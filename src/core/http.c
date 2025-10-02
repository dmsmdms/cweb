#include <global.h>

#define GET_BUF_SIZE (1024 * 1024)

typedef struct {
    http_resp_cb_t resp_cb;
    void *priv_data;
    char *data;
    uint32_t offset;
    uint32_t size;
} http_req_t;

typedef struct {
    ev_io_t io;
} http_sock_t;

static void multi_info_read(app_t *app)
{
    http_t *http = &app->http;
    while(true) {
        int msgs_in_queue;
        CURLMsg *msg = curl_multi_info_read(http->multi, &msgs_in_queue);
        if(msg == NULL) {
            break;
        }
        if(msg->msg == CURLMSG_DONE) {
            CURL *curl = msg->easy_handle;
            CURLcode res = msg->data.result;
            char *url = NULL;
            http_req_t *req = NULL;
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
            curl_easy_getinfo(curl, CURLINFO_PRIVATE, &req);

            if(res == CURLE_OK) {
                long resp_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
                if(resp_code >= 200 && resp_code < 300) {
                    log_info("url %s done, resp_code=%ld, data_size=%u", url, resp_code, req->size);
                    http_resp_t resp = {
                        .url = url,
                        .data = req->data,
                        .size = req->size,
                        .priv_data = req->priv_data,
                    };
                    req->resp_cb(&resp);
                } else {
                    log_warn("url %s done, resp_code=%ld data_size=%u", url, resp_code, req->size);
                }
            } else {
                log_error("url %s failed, error=%s", url, curl_easy_strerror(res));
            }

            curl_multi_remove_handle(http->multi, curl);
            curl_easy_cleanup(curl);
            free(req->data);
            mem_pool_free(&http->req_pool, req, sizeof(http_req_t));
        }
    }
}

static void sock_cb(const ev_io_t *io, uint32_t events)
{
    app_t *app = io->priv_data;
    http_t *http = &app->http;
    bool ev_in = !!(events & EPOLLIN);
    bool ev_out = !!(events & EPOLLOUT);
    int action = 0;
    if(ev_in) {
        action |= CURL_POLL_IN;
    }
    if(ev_out) {
        action |= CURL_POLL_OUT;
    }

    CURLMcode rc = curl_multi_socket_action(http->multi, io->fd, action, NULL);
    if(rc != CURLM_OK) {
        log_error("curl error %s", curl_multi_strerror(rc));
        return;
    }
    multi_info_read(app);
}

static int multi_sock_cb(CURL *curl, curl_socket_t sock_fd, int action, void *app_data, void *sock_data)
{
    app_t *app = app_data;
    http_t *http = &app->http;
    http_sock_t *sock = sock_data;

    if(action == CURL_POLL_REMOVE) {
        log_debug("remove fd=%d", sock_fd);
        goto exit;
    } else {
        uint32_t events = 0;
        bool ev_in = !!(action & CURL_POLL_IN);
        bool ev_out = !!(action & CURL_POLL_OUT);
        if(ev_in) {
            events |= EPOLLIN;
        }
        if(ev_out) {
            events |= EPOLLOUT;
        }
        if(sock == NULL) {
            log_debug("add fd=%d in=%u out=%u", sock_fd, ev_in, ev_out);
            sock = mem_pool_alloc(&http->sock_pool, sizeof(http_sock_t));
            ev_io_add(&app->loop, &sock->io, sock_cb, app, sock_fd, events);

            CURLMcode rc = curl_multi_assign(http->multi, sock_fd, sock);
            if(rc != CURLM_OK) {
                log_error("curl error %s", curl_multi_strerror(rc));
                goto exit;
            }
        } else {
            log_debug("update fd=%d in=%u out=%u", sock_fd, ev_in, ev_out);
            ev_io_mod(&app->loop, &sock->io, events);
        }
    }

    return 0;
exit:
    ev_io_del(&app->loop, &sock->io);
    mem_pool_free(&http->sock_pool, sock, sizeof(http_sock_t));
    return 0;
}

static void timer_cb(const ev_timer_t *timer)
{
    app_t *app = timer->priv_data;
    http_t *http = &app->http;
    CURLMcode rc = curl_multi_socket_action(http->multi, CURL_SOCKET_TIMEOUT, 0, NULL);
    if(rc != CURLM_OK) {
        log_error("curl error %s", curl_multi_strerror(rc));
        return;
    }
    multi_info_read(app);
}

static int multi_timer_cb(CURLM *multi, long timeout_ms, void *app_data)
{
    app_t *app = app_data;
    http_t *http = &app->http;

    if(timeout_ms >= 0) {
        if(http->timer.cb) {
            ev_timer_mod(&app->loop, &http->timer, timeout_ms);
        } else {
            ev_timer_add(&app->loop, &http->timer, timer_cb, app, timeout_ms);
        }
    } else {
        ev_timer_del(&app->loop, &http->timer);
    }

    return 0;
}

void http_init(http_t *http)
{
    app_t *app = container_of(http, app_t, http);
    http->multi = curl_multi_init();
    curl_multi_setopt(http->multi, CURLMOPT_SOCKETFUNCTION, multi_sock_cb);
    curl_multi_setopt(http->multi, CURLMOPT_SOCKETDATA, app);
    curl_multi_setopt(http->multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(http->multi, CURLMOPT_TIMERDATA, app);
}

void http_destroy(http_t *http)
{
    app_t *app = container_of(http, app_t, http);
    if(http->timer.cb) {
        ev_timer_del(&app->loop, &http->timer);
    }
    curl_multi_cleanup(http->multi);
}

static uint32_t recv_cb(void *ptr, uint32_t size, uint32_t nmemb, void *req_data)
{
    http_req_t *req = req_data;
    size *= nmemb;
    uint32_t full_size = req->offset + size + 1;

    if(full_size > req->size) {
        uint32_t new_size = req->size ? (req->size << 1) : GET_BUF_SIZE;
        while(new_size < full_size) {
            new_size <<= 1;
        }
        req->data = realloc(req->data, new_size);
        req->size = new_size;
    }

    memcpy(req->data + req->offset, ptr, size);
    req->offset += size;
    req->data[req->offset] = 0;

    return size;
}

void http_get(http_t *http, const char *url, http_resp_cb_t cb, void *priv_data)
{
    app_t *app = container_of(http, app_t, http);
    http_req_t *req = mem_pool_alloc(&http->req_pool, sizeof(http_req_t));
    req->data = NULL;
    req->size = 0;
    req->offset = 0;
    req->resp_cb = cb;
    req->priv_data = priv_data;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, req);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, recv_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);
    log_info("get %s", url);

    CURLMcode rc = curl_multi_add_handle(http->multi, curl);
    if(rc != CURLM_OK) {
        log_error("curl error %s", curl_multi_strerror(rc));
        goto exit;
    }
    return;
exit:
    curl_easy_cleanup(curl);
    mem_pool_free(&http->req_pool, req, sizeof(http_req_t));
    return;
}
