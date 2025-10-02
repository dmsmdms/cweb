#include <core/ipc/ipc-server.h>
#include <core/ipc/ipc-priv.h>
#include <core/base/log.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define MAX_CONN 16

typedef struct sipc_conn {
    LIST_ENTRY(sipc_conn) entry;
    ev_io io;
} sipc_conn_t;

typedef LIST_HEAD(sipc_conn_list, sipc_conn) sipc_conn_list_t;

typedef struct {
    sipc_conn_list_t conn_list;
    ev_io io;
    char *sock_path;
    sipc_cmd_handler_t *handlers;
    uint32_t handlers_count;
    uint32_t pad;
    char data[];
} sipc_t;

static sipc_t *sipc_glob = NULL;

static void conn_close(sipc_conn_t *conn)
{
    ev_io_stop(EV_DEFAULT, &conn->io);
    close(conn->io.fd);
    LIST_REMOVE(conn, entry);
    free(conn);
}

static void read_cb(UNUSED struct ev_loop *loop, ev_io *io, int events)
{
    sipc_conn_t *conn = container_of(io, sipc_conn_t, io);
    sipc_t *sipc = io->data;
    if((events & EV_READ) == 0) {
        log_error("unexpected events=%d", events);
        return;
    }

    char buf[IPC_BUF_SIZE];
    ssize_t n = read(io->fd, buf, sizeof(buf));
    if(n < 0) {
        log_error("read(fd=%d) failed - %s", io->fd, strerror(errno));
        conn_close(conn);
        return;
    } else if(n == 0) {
        log_debug("fd=%d closed by peer", io->fd);
        conn_close(conn);
        return;
    }

    ipc_header_t *header = (ipc_header_t *)buf;
    if((size_t)n < sizeof(ipc_header_t)) {
        log_error("incomplete message: n=%zd", n);
        return;
    }
    if((size_t)n != sizeof(ipc_header_t) + header->len) {
        log_error("incomplete message: n=%zd, expected=%zu", n, sizeof(ipc_header_t) + header->len);
        return;
    }
    log_debug("message: cmd=%u, id=%u, len=%u", header->cmd, header->id, header->len);

    // Find handler //
    for(uint32_t i = 0; i < sipc->handlers_count; i++) {
        sipc_cmd_handler_t *handler = &sipc->handlers[i];
        if(handler->cmd == header->cmd) {
            sipc_req_t req = {
                .conn = conn,
                .data = {
                    .data = buf + sizeof(ipc_header_t),
                    .size = header->len,
                },
                .id = header->id,
            };
            if(handler->cb(&req) != SIPC_ERR_OK) {
                sipc_resp(conn, header->id, IPC_CMD_FAIL_SRV, NULL);
            }
            return;
        }
    }
    log_error("unknown cmd=%u", header->cmd);
    sipc_resp(conn, header->id, IPC_CMD_FAIL_SRV, NULL);
}

static void accept_cb(struct ev_loop *loop, ev_io *io, int events)
{
    sipc_t *sipc = io->data;
    if((events & EV_READ) == 0) {
        log_error("unexpected events=%d", events);
        return;
    }

    while(true) {
        int fd = accept(io->fd, NULL, NULL);
        if(fd < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            log_error("accept(%s) failed", sipc->sock_path);
            return;
        }

        sipc_conn_t *conn = calloc(1, sizeof(sipc_conn_t));
        if(conn == NULL) {
            log_error("malloc(sipc_conn_t) failed");
            close(fd);
            return;
        }
        LIST_INSERT_HEAD(&sipc->conn_list, conn, entry);
        conn->io.fd = fd;

        ev_io_init(&conn->io, read_cb, conn->io.fd, EV_READ);
        conn->io.data = sipc;
        ev_io_start(loop, &conn->io);

        log_debug("new fd=%d", conn->io.fd);
    }
}

sipc_err_t sipc_init(const char *sock_path, const sipc_cmd_handler_t *handlers, uint32_t handlers_count)
{
    uint32_t handlers_size = sizeof(sipc_cmd_handler_t) * handlers_count;
    uint32_t sock_path_len = strlen(sock_path) + 1;
    uint32_t tot_size = sizeof(sipc_t) + handlers_size + sock_path_len;
    sipc_t *sipc = malloc(tot_size);
    if(sipc == NULL) {
        log_error("malloc(%u) failed", tot_size);
        return SIPC_ERR_NO_MEM;
    }
    sipc_glob = sipc;
    bzero(sipc, tot_size);
    LIST_INIT(&sipc->conn_list);
    sipc->handlers = (sipc_cmd_handler_t *)sipc->data;
    sipc->handlers_count = handlers_count;
    sipc->sock_path = sipc->data + handlers_size;
    memcpy(sipc->handlers, handlers, handlers_size);
    memcpy(sipc->sock_path, sock_path, sock_path_len);

    // Create socket //
    sipc->io.fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(sipc->io.fd < 0) {
        log_error("socket create failed");
        sipc_deinit();
        return SIPC_ERR_INIT;
    }
    unlink(sipc->sock_path);

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strncpy(addr.sun_path, sipc->sock_path, sizeof(addr.sun_path) - 1);
    if(bind(sipc->io.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("bind(%s) failed", sock_path);
        sipc_deinit();
        return SIPC_ERR_INIT;
    }
    if(listen(sipc->io.fd, MAX_CONN) < 0) {
        log_error("listen failed");
        sipc_deinit();
        return SIPC_ERR_INIT;
    }

    ev_io_init(&sipc->io, accept_cb, sipc->io.fd, EV_READ);
    sipc->io.data = sipc;
    ev_io_start(EV_DEFAULT, &sipc->io);

    return SIPC_ERR_OK;
}

void sipc_deinit(void)
{
    if(sipc_glob == NULL) {
        return;
    }

    while(!LIST_EMPTY(&sipc_glob->conn_list)) {
        sipc_conn_t *conn = LIST_FIRST(&sipc_glob->conn_list);
        conn_close(conn);
    }

    ev_io_stop(EV_DEFAULT, &sipc_glob->io);
    if(sipc_glob->io.fd >= 0) {
        close(sipc_glob->io.fd);
        unlink(sipc_glob->sock_path);
        sipc_glob->io.fd = -1;
    }

    free(sipc_glob);
    sipc_glob = NULL;
}

sipc_err_t sipc_resp(sipc_conn_t *conn, uint32_t id, uint32_t cmd, const buf_t *data)
{
    ipc_header_t header = {
        .cmd = cmd,
        .id = id,
    };
    struct iovec iov[2] = {
        { &header, sizeof(ipc_header_t) },
    };
    uint32_t req_size = sizeof(ipc_header_t);
    uint32_t iov_cnt = 1;
    if(data) {
        header.len = data->size;
        iov[1].iov_base = data->data;
        iov[1].iov_len = data->size;
        req_size += data->size;
        iov_cnt++;
    } else {
        header.len = 0;
    }
    ssize_t n = writev(conn->io.fd, iov, iov_cnt);
    if(n < 0) {
        log_error("writev(fd=%d) failed - %s", conn->io.fd, strerror(errno));
        conn_close(conn);
        return SIPC_ERR_IO;
    }
    if(n != req_size) {
        log_error("incomplete response: n=%zd, expected=%u", n, req_size);
        conn_close(conn);
        return SIPC_ERR_IO;
    }
    log_debug("response: cmd=%u, id=%u, len=%u", cmd, id, header.len);
    return SIPC_ERR_OK;
}
