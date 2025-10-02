#include <core/http/http-server.h>
#include <core/http/http-ext.h>
#include <core/base/log.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <malloc.h>
#include <errno.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define MAX_CONN        16
#define MAX_HEADERS     64
#define HEADER_BUF_SIZE (128 * 1024)
#define BODY_BUF_SIZE   (1024 * 1024)

typedef struct shttp_conn {
    LIST_ENTRY(shttp_conn) entry;      ///< Linked list entry for managing multiple connections
    str_buf_t body;                    ///< Buffer to hold the request/responce body
    ev_io io;                          ///< Read/Write events watcher
    shttp_method_t method;             ///< HTTP method of the request
    shttp_content_type_t content_type; ///< HTTP content type of the request
    char *path;                        ///< Request path
} shttp_conn_t;

typedef struct shttp_req_hand {
    LIST_ENTRY(shttp_req_hand) entry; ///< Linked list entry for managing multiple request handlers
    shttp_req_cb_t func;              ///< Request handler callback
    uint32_t len;                     ///< Length of the request path
    char path[];                      ///< Request path
} shttp_req_hand_t;

typedef struct shttp {
    LIST_HEAD(shttp_conn_list, shttp_conn) conn_list;             ///< List of active connections
    LIST_HEAD(shttp_req_hand_list, shttp_req_hand) req_hand_list; ///< List of request handlers
    ev_io io;                                                     ///< Accept events watcher
} shttp_t;

static shttp_t shttp_glob = {
    .io.fd = -1,
};

static const char *content_type_str[] = {
    [SHTTP_CONTENT_TYPE_HTML] = "text/html",
    [SHTTP_CONTENT_TYPE_JSON] = "application/json",
};
STATIC_ASSERT(ARRAY_SIZE(content_type_str) == SHTTP_CONTENT_TYPE_MAX);
static const char *connection_str[] = {
    [SHTTP_CONNECTION_CLOSE] = "close",
    [SHTTP_CONNECTION_KEEPALIVE] = "keep-alive",
};
STATIC_ASSERT(ARRAY_SIZE(connection_str) == SHTTP_CONNECTION_MAX);

static shttp_method_t http_str_method(const char *str)
{
    if(strcmp(str, "GET") == 0) {
        return SHTTP_METHOD_GET;
    } else if(strcmp(str, "POST") == 0) {
        return SHTTP_METHOD_POST;
    }
    return SHTTP_METHOD_MAX;
}

static const char *resp_code_str(shttp_resp_code_t code)
{
    switch(code) {
    case SHTTP_RESP_CODE_200_OK:
        return "OK";
    case SHTTP_RESP_CODE_400_BAD_REQUEST:
        return "Bad Request";
    case SHTTP_RESP_CODE_404_NOT_FOUND:
        return "Not Found";
    case SHTTP_RESP_CODE_500_SERVER_ERROR:
        return "Internal Server Error";
    default:
        return NULL;
    }
}

static void conn_free(shttp_conn_t *conn)
{
    LIST_REMOVE(conn, entry);

    if(ev_is_active(&conn->io)) {
        log_debug("free fd=%d", conn->io.fd);
        ev_io_stop(EV_DEFAULT, &conn->io);
        close(conn->io.fd);
    }

    free(conn->body.data);
    free(conn);
}

static shttp_err_t req_hand_call(const shttp_req_t *req)
{
    shttp_conn_t *conn = req->conn;
    shttp_t *shttp = conn->io.data;
    shttp_req_hand_t *req_hand;
    LIST_FOREACH(req_hand, &shttp->req_hand_list, entry)
    {
        if(strncmp(req->path, req_hand->path, req_hand->len) == 0) {
            return req_hand->func(req);
        }
    }
    log_warn("no handler path=%s", req->path);
    return SHTTP_ERR_NO_HANDLER;
}

static void read_body_cb(UNUSED struct ev_loop *loop, ev_io *io, int events)
{
    shttp_conn_t *conn = container_of(io, shttp_conn_t, io);
    if((events & EV_READ) == 0) {
        log_error("unexpected events=%d fd=%d", events, io->fd);
        conn_free(conn);
        return;
    }

    ssize_t n = read(io->fd, conn->body.data + conn->body.offset, conn->body.size - conn->body.offset);
    if(n <= 0) {
        if(n < 0) {
            log_error("read fd=%d failed - %s", io->fd, strerror(errno));
        }
        conn_free(conn);
        return;
    }

    conn->body.offset += n;
    if(conn->body.offset < conn->body.size) {
        log_warn("read fd=%d incomplete %u/%u", conn->io.fd, conn->body.offset, conn->body.size);
        return;
    }

    shttp_req_t req = {
        .conn = conn,
        .path = conn->path,
        .body.data = conn->body.data,
        .body.len = conn->body.size,
        .method = conn->method,
        .content_type = conn->content_type,
    };
    if(req_hand_call(&req) != SHTTP_ERR_OK) {
        conn_free(conn);
        return;
    }
}

static void read_header_cb(UNUSED struct ev_loop *loop, ev_io *io, int events)
{
    shttp_conn_t *conn = container_of(io, shttp_conn_t, io);
    if((events & EV_READ) == 0) {
        log_error("unexpected events=%d fd=%d", events, io->fd);
        conn_free(conn);
        return;
    }

    char buf[HEADER_BUF_SIZE];
    ssize_t n = read(conn->io.fd, buf, sizeof(buf) - 1);
    if(n <= 0) {
        if(n < 0) {
            log_error("read fd=%d failed - %s", conn->io.fd, strerror(errno));
        }
        conn_free(conn);
        return;
    }
    buf[n] = '\0';

    phr_header_t headers[MAX_HEADERS];
    size_t headers_count = ARRAY_SIZE(headers);
    str_t method, path;
    int minor_version;
    int offset = phr_parse_request(buf, n, (const char **)&method.data, &method.len, (const char **)&path.data,
                                   &path.len, &minor_version, headers, &headers_count, 0);
    if(offset < 0) {
        log_error("parse request fd=%d failed - %s", conn->io.fd, buf);
        conn_free(conn);
        return;
    }
    conn->method = http_str_method(method.data);

    uint32_t content_len = 0;
    http_enum_t content_type_enum = {
        .pval = &conn->content_type,
        .enums = content_type_str,
        .enums_count = ARRAY_SIZE(content_type_str),
    };
    http_header_item_t items[] = {
        { "Content-Length", http_parse_int32, &content_len },
        { "Content-Type", http_parse_enum, &content_type_enum },
    };
    if(http_parse_headers(headers, headers_count, items, ARRAY_SIZE(items)) != HTTP_PARSE_ERR_OK) {
        conn_free(conn);
        return;
    }

    if(content_len > 0) {
        uint32_t path_offset = BODY_BUF_SIZE - (path.len + 1);
        if(content_len >= path_offset) {
            log_error("content_len too large fd=%d len=%u", conn->io.fd, content_len);
            conn_free(conn);
            return;
        }
        if(conn->content_type == SHTTP_CONTENT_TYPE_MAX) {
            log_error("invalid Content-Type fd=%d", conn->io.fd);
            conn_free(conn);
            return;
        }

        n -= offset;
        if(n != content_len) {
            conn->body.data = malloc(BODY_BUF_SIZE);
            if(conn->body.data == NULL) {
                log_error("malloc body failed");
                conn_free(conn);
                return;
            }

            memcpy(conn->body.data, buf + offset, n);
            conn->body.size = content_len;
            conn->body.offset = n;
            conn->io.cb = read_body_cb;

            memcpy(conn->body.data + path_offset, path.data, path.len);
            return;
        }
    }

    method.data[method.len] = '\0';
    path.data[path.len] = '\0';

    shttp_req_t req = {
        .conn = conn,
        .path = path.data,
        .body.data = buf + offset,
        .body.len = content_len,
        .method = conn->method,
        .content_type = conn->content_type,
    };
    if(req_hand_call(&req) != SHTTP_ERR_OK) {
        conn_free(conn);
        return;
    }
}

static void accept_cb(struct ev_loop *loop, ev_io *io, int events)
{
    shttp_t *shttp = io->data;
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
            log_error("accept failed");
            return;
        }

        shttp_conn_t *conn = calloc(1, sizeof(shttp_conn_t));
        if(conn == NULL) {
            log_error("malloc shttp_conn_t failed");
            close(fd);
            return;
        }
        LIST_INSERT_HEAD(&shttp->conn_list, conn, entry);
        conn->io.fd = fd;

        ev_io_init(&conn->io, read_header_cb, conn->io.fd, EV_READ);
        conn->io.data = shttp;
        ev_io_start(loop, &conn->io);

        log_debug("new fd=%d", conn->io.fd);
    }
}

shttp_err_t shttp_init(const char *sock_path)
{
    shttp_glob.io.fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(shttp_glob.io.fd < 0) {
        log_error("socket create");
        shttp_destroy();
        return SHTTP_ERR_SOCKET;
    }
    unlink(sock_path);

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strcpy(addr.sun_path, sock_path);
    if(bind(shttp_glob.io.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("socket bind %s", sock_path);
        shttp_destroy();
        return SHTTP_ERR_SOCKET;
    }

    if(listen(shttp_glob.io.fd, MAX_CONN) < 0) {
        log_error("socket listen %s", sock_path);
        shttp_destroy();
        return SHTTP_ERR_SOCKET;
    }

    ev_io_init(&shttp_glob.io, accept_cb, shttp_glob.io.fd, EV_READ);
    shttp_glob.io.data = &shttp_glob;
    ev_io_start(EV_DEFAULT, &shttp_glob.io);

    return SHTTP_ERR_OK;
}

void shttp_destroy(void)
{
    shttp_conn_t *conn = shttp_glob.conn_list.lh_first;
    while(conn) {
        shttp_conn_t *next = conn->entry.le_next;
        conn_free(conn);
        conn = next;
    }

    shttp_req_hand_t *hand = shttp_glob.req_hand_list.lh_first;
    while(hand) {
        shttp_req_hand_t *next = hand->entry.le_next;
        shttp_del_req_hand(hand);
        hand = next;
    }

    ev_io_stop(EV_DEFAULT, &shttp_glob.io);
    if(shttp_glob.io.fd >= 0) {
        close(shttp_glob.io.fd);
        shttp_glob.io.fd = -1;
    }
}

shttp_req_hand_t *shttp_add_req_hand(const char *path, shttp_req_cb_t cb)
{
    size_t len = strlen(path);
    shttp_req_hand_t *hand = malloc(sizeof(shttp_req_hand_t) + len + 1);
    if(hand == NULL) {
        log_error("malloc shttp_req_hand_t failed");
        return NULL;
    }

    bzero(hand, sizeof(shttp_req_hand_t));
    LIST_INSERT_HEAD(&shttp_glob.req_hand_list, hand, entry);
    strcpy(hand->path, path);
    hand->len = len;
    hand->func = cb;

    return hand;
}

void shttp_del_req_hand(shttp_req_hand_t *hand)
{
    LIST_REMOVE(hand, entry);
    free(hand);
}

static void write_cb(struct ev_loop *loop, ev_io *io, int events)
{
    shttp_conn_t *conn = io->data;
    if((events & EV_WRITE) == 0) {
        log_error("unexpected events=%d fd=%d", events, io->fd);
        conn_free(conn);
        return;
    }

    ssize_t n = write(io->fd, conn->body.data + conn->body.offset, conn->body.size - conn->body.offset);
    if(n < 0) {
        log_error("write fd=%d failed - %s", io->fd, strerror(errno));
        conn_free(conn);
        return;
    }
    conn->body.offset += n;

    if(conn->body.offset == conn->body.size) {
        log_debug("write fd=%d complete", io->fd);
        shttp_buf_free(conn);

        ev_io_stop(loop, &conn->io);
        ev_io_init(&conn->io, read_header_cb, io->fd, EV_READ);
        ev_io_start(loop, &conn->io);
    } else {
        log_warn("write fd=%d incomplete %u/%u", conn->io.fd, conn->body.offset, conn->body.size);
    }
}

shttp_err_t shttp_resp(shttp_conn_t *conn, shttp_resp_code_t code, shttp_content_type_t content_type,
                       shttp_connection_t connection, const str_t *body)
{
    if(body->len >= BODY_BUF_SIZE) {
        log_error("resp too large %zu", body->len);
        shttp_buf_free(conn);
        return SHTTP_ERR_PARAM;
    }
    char hbuf[256];
    uint32_t hlen =
            snprintf(hbuf, sizeof(hbuf) - 1,
                     "HTTP/1.1 %u %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: %s\r\n"
                     "\r\n",
                     code, resp_code_str(code), content_type_str[content_type], body->len, connection_str[connection]);

    struct iovec iov[2] = {
        { hbuf, hlen },
        { body->data, body->len },
    };
    ssize_t n = writev(conn->io.fd, iov, ARRAY_SIZE(iov));
    if(n < 0) {
        log_error("writev fd=%d failed - %s", conn->io.fd, strerror(errno));
        conn_free(conn);
        return SHTTP_ERR_IO;
    } else if(n < hlen) {
        log_error("writev fd=%d header incomplete %zd/%u", conn->io.fd, n, hlen);
        conn_free(conn);
        return SHTTP_ERR_IO;
    } else {
        n -= hlen;
        if((size_t)n != body->len) {
            log_warn("writev fd=%d body incomplete %zd/%zu", conn->io.fd, n, body->len);

            if(conn->body.data == NULL) {
                conn->body.data = malloc(BODY_BUF_SIZE);
                if(conn->body.data == NULL) {
                    log_error("malloc body failed");
                    conn_free(conn);
                    return SHTTP_ERR_MEM_ALLOC;
                }
            }

            size_t len = body->len - n;
            memcpy(conn->body.data, body->data + n, len);
            conn->body.size = len;
            conn->body.offset = 0;

            ev_io_stop(EV_DEFAULT, &conn->io);
            ev_io_init(&conn->io, write_cb, conn->io.fd, EV_WRITE);
            ev_io_start(EV_DEFAULT, &conn->io);
            return SHTTP_ERR_OK;
        }
    }

    conn->io.cb = read_header_cb;
    shttp_buf_free(conn);
    return SHTTP_ERR_OK;
}

void shttp_buf_free(shttp_conn_t *conn)
{
    free(conn->body.data);
    conn->body.data = NULL;
}
