#include <global.h>
#include <sys/uio.h>
#include <sys/un.h>

#define HTTP_MAX_CONN 16
#define HTTP_BUF_SIZE (1024 * 1024)

typedef struct {
    str_t path;
    http_srv_req_cb_t func;
} http_path_cb_t;

static const http_path_cb_t http_path_cb[] = {
#ifdef CONFIG_APP_JOBLY_API
    { STR("/jobly/file"), api_jobly_file_cb },
#endif
#ifdef CONFIG_APP_CRYPTO_API
    { STR("/crypto/api"), api_crypto_cb },
#endif
};
static const char *http_resp_code_str[] = {
    [HTTP_RESP_CODE_200_OK] = "200 OK",
    [HTTP_RESP_CODE_400_BAD_REQUEST] = "400 Bad Request",
    [HTTP_RESP_CODE_404_NOT_FOUND] = "404 Not Found",
    [HTTP_RESP_CODE_500_SERVER_ERROR] = "500 Internal Server Error",
};
static const char *http_content_type_str[] = {
    [HTTP_CONTENT_TYPE_HTML] = "text/html",
    [HTTP_CONTENT_TYPE_JSON] = "application/json",
};
static const char *http_connection_str[] = {
    [HTTP_CONNECTION_CLOSE] = "close",
    [HTTP_CONNECTION_KEEPALIVE] = "keep-alive",
};

static void http_conn_free(http_srv_conn_t *conn)
{
    app_t *app = conn->app;
    http_server_t *server = &app->http_server;
    LIST_REMOVE(conn, entry);
    if(ev_is_active(&conn->io)) {
        log_debug("free fd=%d", conn->io.fd);
        ev_io_stop(app->loop, &conn->io);
        close(conn->io.fd);
    }
    mem_pool_free(&server->conns_pool, conn);
}

static void http_send_resp(const http_srv_conn_t *conn, const http_srv_resp_t *resp)
{
    const app_t *app = conn->app;
    const char *status = http_resp_code_str[resp->code];
    const char *content_type = http_content_type_str[resp->content_type];
    const char *connection = http_connection_str[resp->connection];
    char buf[128];
    uint32_t buf_len = sprintf(buf,
                               "HTTP/1.1 %s\r\n"
                               "Content-Type: %s\r\n"
                               "Content-Length: %zu\r\n"
                               "Connection: %s\r\n"
                               "\r\n",
                               status, content_type, resp->content.len, connection);
    struct iovec iov[2] = {
        { buf, buf_len },
        { resp->content.data, resp->content.len },
    };
    ssize_t total_size = buf_len + resp->content.len;
    ssize_t n = writev(conn->io.fd, iov, ARRAY_SIZE(iov));
    if(n < 0) {
        log_error("writev fd=%d failed - %s", conn->io.fd, strerror(errno));
    } else if(n != total_size) {
        log_warn("writev fd=%d partial %zd/%zd", conn->io.fd, n, total_size);
    }
}

static void http_req_abort(http_srv_conn_t *conn, uint32_t mem_offset)
{
    mem_put_offset(conn->app, mem_offset);
    http_conn_free(conn);
}

static void read_cb(struct ev_loop *loop, ev_io *io, int events)
{
    UNUSED(loop);
    UNUSED(events);
    http_srv_conn_t *conn = io->data;
    app_t *app = conn->app;
    uint32_t mem_offset = mem_get_offset(app);
    char *buf = mem_alloc(app, __func__, HTTP_BUF_SIZE);
    if(buf == NULL) {
        http_req_abort(conn, mem_offset);
        return;
    }
    ssize_t n = read(io->fd, buf, HTTP_BUF_SIZE);
    if(n <= 0) {
        if(n < 0) {
            log_error("read fd=%d failed - %s", io->fd, strerror(errno));
        }
        http_req_abort(conn, mem_offset);
        return;
    }

    str_t method;
    str_t path;
    phr_header_t headers[64];
    size_t num_headers = ARRAY_SIZE(headers);
    int minor_version;
    int offset = phr_parse_request(buf, n, (const char **)&method.data, &method.len, (const char **)&path.data,
                                   &path.len, &minor_version, headers, &num_headers, 0);
    if(offset < 0) {
        log_error("parse request fd=%d failed", io->fd);
        http_req_abort(conn, mem_offset);
        return;
    }
    http_srv_req_t req = {
        .app = app,
        .path = path.data,
        .content_type = HTTP_CONTENT_TYPE_MAX,
    };
    http_enum_t content_type_enum = {
        .pval = &req.content_type,
        .enums = http_content_type_str,
        .enums_count = ARRAY_SIZE(http_content_type_str),
    };
    http_header_item_t items[] = {
        { "Content-Length", http_parse_int32, &req.size },
        { "Content-Type", http_parse_enum, &content_type_enum },
    };
    if(!http_parse_headers(app, headers, num_headers, items, ARRAY_SIZE(items))) {
        http_req_abort(conn, mem_offset);
        return;
    }
    if(req.size != (n - offset)) {
        log_error("incomplete body fd=%d size=%zu expect=%u", io->fd, n - offset, req.size);
        http_req_abort(conn, mem_offset);
        return;
    }
    char *data = buf + offset;
    req.data = data;
    method.data[method.len] = '\0';
    path.data[path.len] = '\0';
    data[req.size] = '\0';
    uint32_t req_print_size = req.size < 256 ? req.size : 256;
    log_info("request %s %s %.*s", method.data, path.data, req_print_size, data);

    http_srv_resp_t resp = {
        .code = HTTP_RESP_CODE_500_SERVER_ERROR,
        .content_type = HTTP_CONTENT_TYPE_HTML,
        .connection = HTTP_CONNECTION_KEEPALIVE,
    };
    for(uint32_t i = 0; i < ARRAY_SIZE(http_path_cb); i++) {
        const http_path_cb_t *cb = &http_path_cb[i];
        if(strncmp(path.data, cb->path.data, cb->path.len) == 0) {
            req.path += cb->path.len;
            cb->func(&req, &resp);
            break;
        }
    }
    http_send_resp(conn, &resp);
    mem_put_offset(app, mem_offset);
}

static void accept_cb(struct ev_loop *loop, ev_io *io, int events)
{
    UNUSED(events);
    app_t *app = io->data;
    http_server_t *server = &app->http_server;
    http_srv_conn_t *conn = mem_pool_calloc(&server->conns_pool);
    if(conn == NULL) {
        return;
    }
    LIST_INSERT_HEAD(&server->conn_list, conn, entry);
    conn->app = app;

    conn->io.fd = accept(io->fd, NULL, NULL);
    if(conn->io.fd < 0) {
        log_error("accept failed");
        mem_pool_free(&server->conns_pool, conn);
        return;
    }

    ev_io_init(&conn->io, read_cb, conn->io.fd, EV_READ);
    conn->io.data = conn;
    ev_io_start(loop, &conn->io);
    log_debug("new fd=%d", conn->io.fd);
}

bool http_server_init(app_t *app)
{
    http_server_t *server = &app->http_server;
    const config_t *cfg = &app->cfg;
    mem_pool_init(app, &server->conns_pool, "http_server_conns", sizeof(http_srv_conn_t));
    LIST_INIT(&server->conn_list);

    server->io.fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server->io.fd < 0) {
        log_error("socket create");
        return false;
    }
    unlink(cfg->http_sock);

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strcpy(addr.sun_path, cfg->http_sock);
    if(bind(server->io.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("socket bind %s", cfg->http_sock);
        close(server->io.fd);
        return false;
    }
    if(listen(server->io.fd, HTTP_MAX_CONN) < 0) {
        log_error("socket listen %s", cfg->http_sock);
        close(server->io.fd);
        unlink(cfg->http_sock);
        return false;
    }

    ev_io_init(&server->io, accept_cb, server->io.fd, EV_READ);
    server->io.data = app;
    ev_io_start(app->loop, &server->io);
    return true;
}

void http_server_destroy(app_t *app)
{
    http_server_t *server = &app->http_server;
    http_srv_conn_t *conn = server->conn_list.lh_first;
    while(conn) {
        http_srv_conn_t *next = conn->entry.le_next;
        http_conn_free(conn);
        conn = next;
    }
    LIST_INIT(&server->conn_list);
    mem_pool_destroy(&server->conns_pool);

    if(ev_is_active(&server->io)) {
        ev_io_stop(app->loop, &server->io);
        close(server->io.fd);
        unlink(app->cfg.http_sock);
    }
}
