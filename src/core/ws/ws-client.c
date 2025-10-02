#include <core/ws/ws-client.h>
#include <core/base/log.h>
#include <libwebsockets.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define WS_RECONN_INTERVAL_SEC (5.0)
#define WS_HEADER_SIZE         (16 * 1024)
#define WS_BUF_SIZE            (128 * 1024)
#define WS_SSL_FLAGS           (LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK)

typedef struct cws_conn {
    struct lws *wsi;
    ev_timer timer;
    cws_recv_cb_t recv_cb;
    char *path;
    char *addr;
    void *user_data;
    uint32_t port;
    bool is_ssl;
    bool is_active;
    bool need_reconnect;
    char data[];
} cws_conn_t;

static int default_cb(struct lws *wsi, enum lws_callback_reasons reason, void *user_data, void *data, size_t len);
static const struct lws_protocols protocols[] = {
    { "default-proto", default_cb, 0, WS_BUF_SIZE, 0, NULL, 0 },
    { 0 },
};
static struct lws_context *context = NULL;

static cws_err_t connect_real(cws_conn_t *conn)
{
    const char *ssl_str = conn->is_ssl ? "wss" : "ws";
    struct lws_client_connect_info info = {
        .context = context,
        .address = conn->addr,
        .port = conn->port,
        .path = conn->path,
        .host = lws_canonical_hostname(context),
        .origin = "origin",
        .ssl_connection = conn->is_ssl ? WS_SSL_FLAGS : 0,
        .protocol = protocols[0].name,
        .userdata = conn,
    };

    conn->wsi = lws_client_connect_via_info(&info);
    if(conn->wsi == NULL) {
        log_error("ws connect failed to %s://%s:%u%s", ssl_str, conn->addr, conn->port, conn->path);
        return CWS_ERR_CONNECT;
    }

    lws_set_opaque_user_data(conn->wsi, conn);
    log_info("ws %p connecting to %s://%s:%u%s", (void *)conn->wsi, ssl_str, conn->addr, conn->port, conn->path);

    return CWS_ERR_OK;
}

static void reconnect_cb(struct ev_loop *loop, ev_timer *timer, UNUSED int revents)
{
    cws_conn_t *conn = timer->data;
    if(connect_real(conn) == CWS_ERR_OK) {
        ev_timer_stop(loop, timer);
    }
}

static void client_close(cws_conn_t *conn)
{
    if(app_is_running && conn->is_active && conn->need_reconnect) {
        ev_timer_init(&conn->timer, reconnect_cb, 0, WS_RECONN_INTERVAL_SEC);
        conn->timer.data = conn;
        ev_timer_start(EV_DEFAULT, &conn->timer);
    } else {
        ev_timer_stop(EV_DEFAULT, &conn->timer);
        free(conn);
    }
}

static int default_cb(struct lws *wsi, enum lws_callback_reasons reason, void *user_data, void *data, size_t len)
{
    cws_conn_t *conn = user_data;
    switch(reason) {
    case LWS_CALLBACK_CLIENT_RECEIVE:
        if(conn->is_active) {
            cws_recv_t recv = {
                .err = CWS_ERR_OK,
                .body.data = data,
                .body.len = len,
                .user_data = conn->user_data,
            };
            conn->recv_cb(&recv);
        } else {
            log_info("ws manually closed on %p", (void *)wsi);
            client_close(conn);
        }
        break;
    case LWS_CALLBACK_CLIENT_CLOSED:
        log_info("ws closed on %p", (void *)wsi);
        client_close(conn);
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        log_error("ws error on %p: %s", (void *)wsi, (char *)data);
        client_close(conn);
        break;
    default:
        break;
    }
    return 0;
}

cws_err_t cws_init(void)
{
    struct ev_loop *loop = EV_DEFAULT;
    struct lws_context_creation_info info = {
        .port = CONTEXT_PORT_NO_LISTEN,
        .protocols = protocols,
        .options = LWS_SERVER_OPTION_LIBEV | LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
        .foreign_loops = (void **)&loop,
        .max_http_header_data = WS_HEADER_SIZE,
    };
    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

    context = lws_create_context(&info);
    if(context == NULL) {
        log_error("lws_create_context failed");
        return CWS_ERR_CONTEXT;
    }

    return CWS_ERR_OK;
}

void cws_destroy(void)
{
    lws_context_destroy(context);
    context = NULL;
}

cws_err_t cws_connect(cws_conn_t **pconn, const char *addr, const char *path, uint32_t port, bool is_ssl,
                      cws_recv_cb_t cb, const buf_t *user_data)
{
    size_t addr_size = strlen(addr) + 1;
    size_t path_size = strlen(path) + 1;
    size_t conn_size = sizeof(cws_conn_t) + addr_size + path_size;
    if(user_data) {
        conn_size += user_data->size;
    }
    cws_conn_t *conn = malloc(conn_size);
    if(conn == NULL) {
        return CWS_ERR_MEM_ALLOC;
    }
    bzero(conn, sizeof(cws_conn_t));

    uint32_t offset = 0;
    conn->recv_cb = cb;
    conn->port = port;
    conn->is_ssl = is_ssl;
    conn->is_active = true;
    conn->need_reconnect = true;

    conn->addr = conn->data + offset;
    offset += addr_size;
    memcpy(conn->addr, addr, addr_size);

    conn->path = conn->data + offset;
    offset += path_size;
    memcpy(conn->path, path, path_size);

    if(user_data) {
        conn->user_data = conn->data + offset;
        memcpy(conn->user_data, user_data->data, user_data->size);
    }

    cws_err_t res = connect_real(conn);
    if(res != CWS_ERR_OK) {
        free(conn);
        return res;
    }

    *pconn = conn;
    return CWS_ERR_OK;
}

void cws_disconnect(cws_conn_t *conn)
{
    if(conn) {
        conn->is_active = false;
    }
}
