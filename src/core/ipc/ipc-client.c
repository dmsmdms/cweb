#include <core/ipc/ipc-client.h>
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

#define IPC_WAIT_TIMEOUT_SEC 1

typedef struct cipc_req {
    LIST_ENTRY(cipc_req) entries;
    ev_timer timer;
    cipc_resp_cb_t cb;
    uint32_t id;
    uint32_t pad;
    char user_data[];
} cipc_req_t;

typedef LIST_HEAD(cipc_req_head, cipc_req) cipc_req_head_t;

typedef struct cipc {
    cipc_req_head_t reqs;
    ev_io io;
    uint32_t next_req_id;
    char sock_path[];
} cipc_t;

static void cipc_req_free(cipc_req_t *req)
{
    ev_timer_stop(EV_DEFAULT, &req->timer);
    LIST_REMOVE(req, entries);
    free(req);
}

static void cipc_stop(cipc_t *cipc)
{
    while(!LIST_EMPTY(&cipc->reqs)) {
        cipc_req_t *req = LIST_FIRST(&cipc->reqs);
        log_warn("request id=%u not finished", req->id);
        cipc_req_free(req);
    }

    ev_io_stop(EV_DEFAULT, &cipc->io);
    if(cipc->io.fd >= 0) {
        close(cipc->io.fd);
        cipc->io.fd = -1;
    }
}

static void read_cb(UNUSED struct ev_loop *loop, ev_io *io, int events)
{
    if((events & EV_READ) == 0) {
        log_error("unexpected events=%d fd=%d", events, io->fd);
        cipc_stop(io->data);
        return;
    }
    char buf[IPC_BUF_SIZE];
    cipc_t *cipc = io->data;
    ssize_t n = read(io->fd, buf, sizeof(buf));
    if(n < 0) {
        log_error("read(%s) failed - %s", cipc->sock_path, strerror(errno));
        cipc_stop(cipc);
        return;
    }
    if(n == 0) {
        log_debug("IPC server %s closed connection", cipc->sock_path);
        cipc_stop(cipc);
        return;
    }

    ipc_header_t *header = (ipc_header_t *)buf;
    if(((size_t)n < sizeof(ipc_header_t)) || ((size_t)n < sizeof(ipc_header_t) + header->len)) {
        log_error("Incomplete IPC message received: n=%zd", n);
        return;
    }
    log_debug("responce: cmd=%u, len=%u, id=%u", header->cmd, header->len, header->id);

    if(header->id) {
        cipc_req_t *req = cipc->reqs.lh_first;
        while(req) {
            cipc_req_t *next = req->entries.le_next;
            if(req->id == header->id) {
                cipc_resp_t resp = {
                    .err = (header->cmd == IPC_CMD_OK) ? CIPC_ERR_OK : CIPC_ERR_FAIL,
                    .body.data = header->data,
                    .body.size = header->len,
                    .user_data = req->user_data,
                };
                req->cb(&resp);
                cipc_req_free(req);
                return;
            }
            req = next;
        }
        log_warn("no request for id=%u", header->id);
    }
}

static cipc_err_t cipc_connect(cipc_t *cipc)
{
    cipc->io.fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(cipc->io.fd < 0) {
        log_error("socket create failed");
        return CIPC_ERR_CONNECT;
    }

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strncpy(addr.sun_path, cipc->sock_path, sizeof(addr.sun_path) - 1);
    if(connect(cipc->io.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("connect(%s) failed", cipc->sock_path);
        return CIPC_ERR_CONNECT;
    }

    ev_io_init(&cipc->io, read_cb, cipc->io.fd, EV_READ);
    cipc->io.data = cipc;
    ev_io_start(EV_DEFAULT, &cipc->io);

    log_debug("connected to ipc=%s", cipc->sock_path);

    return CIPC_ERR_OK;
}

cipc_err_t cipc_init(cipc_t **pcipc, const char *sock_path)
{
    uint32_t sock_path_len = strlen(sock_path) + 1;
    cipc_t *cipc = malloc(sizeof(cipc_t) + sock_path_len);
    if(cipc == NULL) {
        log_error("malloc(%zu) failed", sizeof(cipc_t) + sock_path_len);
        return CIPC_ERR_NO_MEM;
    }
    bzero(cipc, sizeof(cipc_t));
    memcpy(cipc->sock_path, sock_path, sock_path_len);
    LIST_INIT(&cipc->reqs);
    cipc->next_req_id = 1;

    cipc_err_t err = cipc_connect(cipc);
    if(err != CIPC_ERR_OK) {
        cipc_deinit(cipc);
        return err;
    }
    *pcipc = cipc;
    return err;
}

void cipc_deinit(cipc_t *cipc)
{
    if(cipc == NULL) {
        return;
    }
    cipc_stop(cipc);
    free(cipc);
}

static void timer_cb(UNUSED struct ev_loop *loop, ev_timer *timer, UNUSED int events)
{
    cipc_req_t *req = timer->data;
    log_error("request id=%u timed out", req->id);
    cipc_resp_t resp = {
        .err = CIPC_ERR_TIMEOUT,
        .body.data = NULL,
        .body.size = 0,
        .user_data = req->user_data,
    };
    req->cb(&resp);
    cipc_req_free(req);
}

cipc_err_t cipc_send(cipc_t *cipc, uint32_t cmd, const buf_t *body, cipc_resp_cb_t cb, const buf_t *user_data)
{
    if(cipc->io.fd < 0) {
        cipc_err_t err = cipc_connect(cipc);
        if(err != CIPC_ERR_OK) {
            cipc_resp_t resp = {
                .err = err,
                .body.data = NULL,
                .body.size = 0,
                .user_data = user_data ? user_data->data : NULL,
            };
            cb(&resp);
            cipc_stop(cipc);
            return err;
        }
    }

    uint32_t tot_size = sizeof(cipc_req_t);
    if(user_data) {
        tot_size += user_data->size;
    }
    cipc_req_t *req = malloc(tot_size);
    if(req == NULL) {
        log_error("malloc(%u) failed", tot_size);
        return CIPC_ERR_NO_MEM;
    }
    bzero(req, tot_size);
    LIST_INSERT_HEAD(&cipc->reqs, req, entries);
    req->cb = cb;
    req->id = cipc->next_req_id;
    if(user_data) {
        memcpy(req->user_data, user_data->data, user_data->size);
    }

    ipc_header_t header = {
        .cmd = cmd,
        .id = req->id,
    };
    struct iovec iov[2] = {
        { &header, sizeof(ipc_header_t) },
    };
    uint32_t req_size = sizeof(ipc_header_t);
    uint32_t iov_cnt = 1;
    if(body) {
        header.len = body->size;
        iov[1].iov_base = body->data;
        iov[1].iov_len = body->size;
        req_size += body->size;
        iov_cnt++;
    } else {
        header.len = 0;
    }
    ssize_t n = writev(cipc->io.fd, iov, iov_cnt);
    if(n < 0) {
        log_error("writev(%s) failed - %s", cipc->sock_path, strerror(errno));
        cipc_req_free(req);
        return CIPC_ERR_SEND;
    }
    if(n != req_size) {
        log_error("Incomplete IPC message sent: n=%zd, expected=%u", n, req_size);
        cipc_req_free(req);
        return CIPC_ERR_SEND;
    }

    ev_timer_init(&req->timer, timer_cb, IPC_WAIT_TIMEOUT_SEC, 0);
    req->timer.data = req;
    ev_timer_start(EV_DEFAULT, &req->timer);

    cipc->next_req_id++;
    if(cipc->next_req_id >= UINT16_MAX) {
        cipc->next_req_id = 1;
    }

    log_debug("message: cmd=%u, len=%u, id=%u", cmd, header.len, req->id);
    return CIPC_ERR_OK;
}
