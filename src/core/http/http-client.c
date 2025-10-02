#include <global.h>
// #define NO_INFO
// #define NO_DEBUG
#include <core/base/log-off.h>

#define DATA_BUF_SIZE (1024 * 1024)

typedef struct {
    ev_io io;
} http_sock_t;

static void http_req_free(http_req_t *req)
{
    app_t *app = req->app;
    http_client_t *client = &app->http_client;
    LIST_REMOVE(req, entry);
    curl_multi_remove_handle(client->multi, req->curl);
    curl_mime_free(req->mime);
    curl_easy_cleanup(req->curl);
    mem_pool_free(&client->data_pool, req->data);
    mem_pool_free(&client->req_pool, req);
}

static void multi_info_read(app_t *app)
{
    http_client_t *client = &app->http_client;
    while(true) {
        int msgs_in_queue;
        CURLMsg *msg = curl_multi_info_read(client->multi, &msgs_in_queue);
        if(msg == NULL) {
            break;
        }
        if(msg->msg == CURLMSG_DONE) {
            CURL *curl = msg->easy_handle;
            CURLcode res = msg->data.result;
            const char *url = NULL;
            http_req_t *req = NULL;
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
            curl_easy_getinfo(curl, CURLINFO_PRIVATE, &req);

            if(res == CURLE_OK) {
                long resp_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
                if(resp_code >= 200 && resp_code < 300) {
                    log_info("url %s done, resp_code=%ld, data_size=%u", url, resp_code, req->offset);
                    http_resp_t resp = {
                        .app = app,
                        .url = url,
                        .data = req->data,
                        .size = req->offset,
                        .priv_data = req->priv_data,
                    };
                    req->resp_cb(&resp);
                } else {
                    log_warn("url %s done, resp_code=%ld data_size=%u", url, resp_code, req->offset);
                }
            } else {
                log_error("url %s failed - %s", url, curl_easy_strerror(res));
            }
            http_req_free(req);
        }
    }
}

static void sock_cb(struct ev_loop *loop, ev_io *io, int events)
{
    UNUSED(loop);
    app_t *app = io->data;
    http_client_t *client = &app->http_client;
    bool ev_in = !!(events & EV_READ);
    bool ev_out = !!(events & EV_WRITE);
    int action = 0;
    if(ev_in) {
        action |= CURL_POLL_IN;
    }
    if(ev_out) {
        action |= CURL_POLL_OUT;
    }

    CURLMcode rc = curl_multi_socket_action(client->multi, io->fd, action, NULL);
    if(rc != CURLM_OK) {
        log_error("curl_multi_socket_action - %s", curl_multi_strerror(rc));
        return;
    }
    multi_info_read(app);
}

static void sock_free(app_t *app, http_sock_t *sock)
{
    http_client_t *client = &app->http_client;
    ev_io_stop(app->loop, &sock->io);
    mem_pool_free(&client->sock_pool, sock);
}

static int multi_sock_cb(CURL *curl, curl_socket_t sock_fd, int action, void *app_data, void *sock_data)
{
    UNUSED(curl);
    app_t *app = app_data;
    http_client_t *client = &app->http_client;
    http_sock_t *sock = sock_data;

    if(action == CURL_POLL_REMOVE) {
        log_debug("remove fd=%d", sock_fd);
        sock_free(app, sock);
    } else {
        uint32_t events = 0;
        bool ev_in = !!(action & CURL_POLL_IN);
        bool ev_out = !!(action & CURL_POLL_OUT);
        if(ev_in) {
            events |= EV_READ;
        }
        if(ev_out) {
            events |= EV_WRITE;
        }
        if(sock == NULL) {
            log_debug("add fd=%d in=%u out=%u", sock_fd, ev_in, ev_out);
            sock = mem_pool_calloc(&client->sock_pool);
            if(sock == NULL) {
                return -1;
            }
            ev_io_init(&sock->io, sock_cb, sock_fd, events);
            sock->io.data = app;
            ev_io_start(app->loop, &sock->io);

            CURLMcode rc = curl_multi_assign(client->multi, sock_fd, sock);
            if(rc != CURLM_OK) {
                log_error("curl_multi_assign - %s", curl_multi_strerror(rc));
                sock_free(app, sock);
                return -1;
            }
        } else {
            log_debug("update fd=%d in=%u out=%u", sock_fd, ev_in, ev_out);
            ev_io_stop(app->loop, &sock->io);
            ev_io_set(&sock->io, sock_fd, events);
            ev_io_start(app->loop, &sock->io);
        }
    }

    return 0;
}

static void timer_cb(struct ev_loop *loop, ev_timer *timer, int events)
{
    UNUSED(loop);
    UNUSED(events);
    app_t *app = timer->data;
    http_client_t *client = &app->http_client;
    CURLMcode rc = curl_multi_socket_action(client->multi, CURL_SOCKET_TIMEOUT, 0, NULL);
    if(rc != CURLM_OK) {
        log_error("curl_multi_socket_action - %s", curl_multi_strerror(rc));
        return;
    }
    multi_info_read(app);
}

static int multi_timer_cb(CURLM *multi, long timeout_ms, void *app_data)
{
    UNUSED(multi);
    app_t *app = app_data;
    http_client_t *client = &app->http_client;

    ev_timer_stop(app->loop, &client->timer);
    if(timeout_ms >= 0) {
        ev_timer_init(&client->timer, timer_cb, timeout_ms / 1000, 0);
        client->timer.data = app;
        ev_timer_start(app->loop, &client->timer);
    }

    return 0;
}

bool http_client_init(app_t *app)
{
    http_client_t *client = &app->http_client;
    LIST_INIT(&client->req_list);
    mem_pool_init(app, &client->req_pool, "http_client_req", sizeof(http_req_t));
    mem_pool_init(app, &client->data_pool, "http_client_data", DATA_BUF_SIZE);
    mem_pool_init(app, &client->sock_pool, "http_client_sock", sizeof(http_sock_t));
    client->multi = curl_multi_init();
    if(client->multi == NULL) {
        log_error("curl_multi_init failed");
        return false;
    }
    curl_multi_setopt(client->multi, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);
    curl_multi_setopt(client->multi, CURLMOPT_MAX_CONCURRENT_STREAMS, 10L);
    curl_multi_setopt(client->multi, CURLMOPT_SOCKETFUNCTION, multi_sock_cb);
    curl_multi_setopt(client->multi, CURLMOPT_SOCKETDATA, app);
    curl_multi_setopt(client->multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(client->multi, CURLMOPT_TIMERDATA, app);
    return true;
}

void http_client_destroy(app_t *app)
{
    http_client_t *client = &app->http_client;
    http_req_t *req = client->req_list.lh_first;
    while(req) {
        http_req_t *next = req->entry.le_next;
        http_req_free(req);
        req = next;
    }
    ev_timer_stop(app->loop, &client->timer);
    curl_multi_cleanup(client->multi);
    client->multi = NULL;
    mem_pool_destroy(&client->req_pool);
    mem_pool_destroy(&client->data_pool);
    mem_pool_destroy(&client->sock_pool);
}

static size_t recv_cb(void *ptr, size_t size, size_t nmemb, void *req_data)
{
    http_req_t *req = req_data;
    app_t *app = req->app;
    http_client_t *client = &app->http_client;
    size *= nmemb;
    if((req->offset + size + 1) > DATA_BUF_SIZE) {
        log_error("buffer overflow, offset=%u size=%zu", req->offset, size);
        return 0;
    }
    if(req->data == NULL) {
        req->data = mem_pool_alloc(&client->data_pool);
        if(req->data == NULL) {
            return 0;
        }
    }
    memcpy(req->data + req->offset, ptr, size);
    req->offset += size;
    req->data[req->offset] = 0;
    return size;
}

static bool http_req(app_t *app, const char *url, const http_mime_t *mimes, uint32_t mimes_size, http_resp_cb_t cb,
                     void *priv_data, uint32_t data_size)
{
    http_client_t *client = &app->http_client;
    if(data_size >= HTTP_USER_DATA_SIZE) {
        log_error("data_size=%u is too big", data_size);
        return false;
    }
    http_req_t *req = mem_pool_calloc(&client->req_pool);
    if(req == NULL) {
        return false;
    }
    LIST_INSERT_HEAD(&client->req_list, req, entry);
    req->resp_cb = cb;
    req->app = app;

    if(data_size > 0) {
        memcpy(req->user_data, priv_data, data_size);
        req->priv_data = req->user_data;
    } else {
        req->priv_data = priv_data;
    }

    req->curl = curl_easy_init();
    if(req->curl == NULL) {
        log_error("curl_easy_init failed");
        http_req_free(req);
        return false;
    }
    curl_easy_setopt(req->curl, CURLOPT_URL, url);
    curl_easy_setopt(req->curl, CURLOPT_PRIVATE, req);
    curl_easy_setopt(req->curl, CURLOPT_WRITEFUNCTION, recv_cb);
    curl_easy_setopt(req->curl, CURLOPT_WRITEDATA, req);

    if(mimes_size > 0) {
        req->mime = curl_mime_init(req->curl);
        if(req->mime == NULL) {
            log_error("curl_mime_init failed");
            http_req_free(req);
            return false;
        }
        for(uint32_t i = 0; i < mimes_size; i++) {
            curl_mimepart *part = curl_mime_addpart(req->mime);
            if(part == NULL) {
                log_error("curl_mime_addpart failed");
                http_req_free(req);
                return false;
            }
            curl_mime_name(part, mimes[i].name);
            if(mimes[i].type == HTTP_MIME_TYPE_FILE) {
                curl_mime_filedata(part, mimes[i].data);
            } else {
                curl_mime_data(part, mimes[i].data, CURL_ZERO_TERMINATED);
            }
        }
        curl_easy_setopt(req->curl, CURLOPT_MIMEPOST, req->mime);
    }

    CURLMcode rc = curl_multi_add_handle(client->multi, req->curl);
    if(rc != CURLM_OK) {
        log_error("curl_multi_add_handle - %s", curl_multi_strerror(rc));
        http_req_free(req);
        return false;
    }

    log_info("get %s", url);
    return true;
}

bool http_get(app_t *app, const char *url, http_resp_cb_t cb, void *priv_data, uint32_t data_size)
{
    return http_req(app, url, NULL, 0, cb, priv_data, data_size);
}

bool http_post(app_t *app, const char *url, const http_mime_t *mimes, uint32_t mimes_size, http_resp_cb_t cb,
               void *priv_data, uint32_t data_size)
{
    return http_req(app, url, mimes, mimes_size, cb, priv_data, data_size);
}
