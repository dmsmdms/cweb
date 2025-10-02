#include <global.h>
// #define NO_INFO
#include <core/base/log-off.h>

#define WS_RECONN_INTERVAL_SEC 5.0
#define WS_HEADER_SIZE         (16 * 1024)
#define WS_BUF_SIZE            (128 * 1024)
#define WS_SSL_FLAGS           (LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK)

static int ws_cb(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

static const struct lws_protocols protocols[] = {
    { "default-proto", ws_cb, 0, WS_BUF_SIZE, 0, NULL, 0 },
    { 0 },
};

static bool ws_connect_real(ws_conn_t *conn)
{
    app_t *app = conn->app;
    ws_client_t *client = &app->ws_client;
    struct lws_client_connect_info info = {
        .context = client->context,
        .address = conn->addr,
        .port = conn->port,
        .path = conn->path,
        .host = lws_canonical_hostname(client->context),
        .origin = "origin",
        .ssl_connection = conn->is_ssl ? WS_SSL_FLAGS : 0,
        .protocol = protocols[0].name,
        .userdata = conn,
    };
    const char *ssl_str = conn->is_ssl ? "wss" : "ws";
    conn->wsi = lws_client_connect_via_info(&info);
    if(conn->wsi == NULL) {
        log_error("ws connect failed to to %s://%s:%u%s", ssl_str, conn->addr, conn->port, conn->path);
        mem_pool_free(&client->conn_pool, conn);
        return false;
    }
    lws_set_opaque_user_data(conn->wsi, conn);
    log_info("ws %p connecting to %s://%s:%u%s", (void *)conn->wsi, ssl_str, conn->addr, conn->port, conn->path);
    return true;
}

static void reconnect_cb(struct ev_loop *loop, ev_timer *timer, int revents)
{
    UNUSED(revents);
    ws_conn_t *conn = timer->data;
    if(ws_connect_real(conn)) {
        ev_timer_stop(loop, timer);
    }
}

static void ws_client_close(ws_conn_t *conn)
{
    app_t *app = conn->app;
    ws_client_t *client = &app->ws_client;
    if(app->is_running && conn->is_active && conn->need_reconnect) {
        ev_timer_init(&conn->timer, reconnect_cb, 0, WS_RECONN_INTERVAL_SEC);
        conn->timer.data = conn;
        ev_timer_start(app->loop, &conn->timer);
    } else {
        ev_timer_stop(app->loop, &conn->timer);
        mem_pool_free(&client->conn_pool, conn);
    }
}

static int ws_cb(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    ws_conn_t *conn = user;
    switch(reason) {
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        app_t *app = conn->app;
        if(conn->is_active) {
            ws_recv_t recv = {
                .app = app,
                .data = in,
                .size = len,
                .priv_data = conn->priv_data,
            };
            conn->recv_cb(&recv);
        } else {
            log_info("ws manually closed on %p", (void *)wsi);
            ws_client_close(conn);
        }
    } break;
    case LWS_CALLBACK_CLIENT_CLOSED: {
        app_t *app = conn->app;
        log_info("ws closed on %p", (void *)wsi);
        ws_client_close(conn);
    } break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
        app_t *app = conn->app;
        log_error("ws error on %p: %s", (void *)wsi, (char *)in);
        ws_client_close(conn);
    } break;
    default:
        break;
    }
    return 0;
}

bool ws_client_init(app_t *app)
{
    ws_client_t *client = &app->ws_client;
    mem_pool_init(app, &client->conn_pool, "ws_conn", sizeof(ws_conn_t));
    struct lws_context_creation_info info = {
        .port = CONTEXT_PORT_NO_LISTEN,
        .protocols = protocols,
        .options = LWS_SERVER_OPTION_LIBEV,
        .foreign_loops = (void **)&app->loop,
        .max_http_header_data = WS_HEADER_SIZE,
    };
#ifndef CONFIG_HTTP_CLIENT
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#endif
    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
    client->context = lws_create_context(&info);
    if(client->context == NULL) {
        log_error("lws_create_context failed");
        return false;
    }
    return true;
}

void ws_client_destroy(app_t *app)
{
    ws_client_t *client = &app->ws_client;
    lws_context_destroy(client->context);
    client->context = NULL;
    mem_pool_destroy(&client->conn_pool);
}

ws_conn_t *ws_connect(app_t *app, const char *addr, const char *path, uint32_t port, bool is_ssl, ws_recv_cb_t cb,
                      void *priv_data)
{
    ws_client_t *client = &app->ws_client;
    ws_conn_t *conn = mem_pool_calloc(&client->conn_pool);
    if(conn == NULL) {
        return NULL;
    }
    conn->app = app;
    conn->recv_cb = cb;
    conn->priv_data = priv_data;
    conn->port = port;
    conn->is_ssl = is_ssl;
    conn->is_active = true;
    conn->need_reconnect = true;
    strcpy(conn->addr, addr);
    strcpy(conn->path, path);
    return ws_connect_real(conn) ? conn : NULL;
}

void ws_disconnect(ws_conn_t *conn)
{
    if(conn) {
        conn->is_active = false;
    }
}
