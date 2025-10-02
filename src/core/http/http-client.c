#include <core/http/http-client.h>
#include <core/base/log.h>
#include <curl/curl.h>
#include <sys/queue.h>
#include <malloc.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define BODY_BUF_SIZE (1024 * 1024)

typedef struct chttp_req {
    LIST_ENTRY(chttp_req) entry;
    CURL *curl;
    curl_mime *mime;
    chttp_resp_cb_t resp_cb;
    str_t body;
    char user_data[];
} chttp_req_t;

static LIST_HEAD(req_list, chttp_req) req_list = LIST_HEAD_INITIALIZER(req_list);
static ev_timer timer = { 0 };
static CURLM *multi = NULL;

static void req_free(chttp_req_t *req)
{
    LIST_REMOVE(req, entry);

    curl_multi_remove_handle(multi, req->curl);
    curl_mime_free(req->mime);
    curl_easy_cleanup(req->curl);

    free(req->body.data);
    free(req);
}

static void multi_info_read(void)
{
    while(true) {
        int msgs_in_queue;
        CURLMsg *msg = curl_multi_info_read(multi, &msgs_in_queue);
        if(msg == NULL) {
            break;
        }
        if(msg->msg == CURLMSG_DONE) {
            CURL *curl = msg->easy_handle;
            CURLcode res = msg->data.result;
            const char *url = NULL;
            chttp_req_t *req = NULL;
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
            curl_easy_getinfo(curl, CURLINFO_PRIVATE, &req);
            if(url == NULL || req == NULL) {
                log_error("invalid req=%p or url=%s", (void *)req, url);
                continue;
            }

            chttp_resp_t resp = {
                .err = CHTTP_ERR_OK,
                .url = url,
                .body = req->body,
                .user_data = req->user_data,
            };
            if(res == CURLE_OK) {
                long resp_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
                if(resp_code >= 200 && resp_code < 300) {
                    log_info("url %s done, resp_code=%ld, body_len=%zu", url, resp_code, req->body.len);
                } else {
                    log_warn("url %s done, resp_code=%ld body_len=%zu", url, resp_code, req->body.len);
                    resp.err = CHTTP_ERR_RESP;
                }
            } else {
                log_error("url %s failed - %s", url, curl_easy_strerror(res));
                resp.err = CHTTP_ERR_CURL;
            }
            req->resp_cb(&resp);
            req_free(req);
        }
    }
}

static void sock_cb(UNUSED struct ev_loop *loop, ev_io *io, int events)
{
    bool ev_in = !!(events & EV_READ);
    bool ev_out = !!(events & EV_WRITE);
    uint32_t action = 0;
    if(ev_in) {
        action |= CURL_POLL_IN;
    }
    if(ev_out) {
        action |= CURL_POLL_OUT;
    }

    CURLMcode rc = curl_multi_socket_action(multi, io->fd, action, NULL);
    if(rc != CURLM_OK) {
        log_error("curl_multi_socket_action - %s", curl_multi_strerror(rc));
        return;
    }
    multi_info_read();
}

static void sock_free(ev_io *io)
{
    ev_io_stop(EV_DEFAULT, io);
    free(io);
}

static int multi_sock_cb(UNUSED CURL *curl, curl_socket_t sock_fd, int action, UNUSED void *app_data, void *sock_data)
{
    ev_io *io = sock_data;
    if(action == CURL_POLL_REMOVE) {
        log_debug("remove fd=%d", sock_fd);
        if(io) {
            sock_free(io);
        }
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

        if(io == NULL) {
            log_debug("add fd=%d in=%u out=%u", sock_fd, ev_in, ev_out);
            io = malloc(sizeof(ev_io));
            if(io == NULL) {
                log_error("malloc ev_io failed");
                return -1;
            }
            ev_io_init(io, sock_cb, sock_fd, events);
            ev_io_start(EV_DEFAULT, io);

            CURLMcode rc = curl_multi_assign(multi, sock_fd, io);
            if(rc != CURLM_OK) {
                log_error("curl_multi_assign - %s", curl_multi_strerror(rc));
                sock_free(io);
                return -1;
            }
        } else {
            log_debug("update fd=%d in=%u out=%u", sock_fd, ev_in, ev_out);
            ev_io_stop(EV_DEFAULT, io);
            ev_io_set(io, sock_fd, events);
            ev_io_start(EV_DEFAULT, io);
        }
    }
    return 0;
}

static void timer_cb(UNUSED struct ev_loop *loop, UNUSED ev_timer *ev_timer, UNUSED int events)
{
    CURLMcode rc = curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, NULL);
    if(rc != CURLM_OK) {
        log_error("curl_multi_socket_action - %s", curl_multi_strerror(rc));
        return;
    }
    multi_info_read();
}

static int multi_timer_cb(UNUSED CURLM *curlm, long timeout_ms, UNUSED void *app_data)
{
    ev_timer_stop(EV_DEFAULT, &timer);
    if(timeout_ms >= 0) {
        ev_timer_init(&timer, timer_cb, timeout_ms / 1000, 0);
        ev_timer_start(EV_DEFAULT, &timer);
    }
    return 0;
}

chttp_err_t chttp_init(void)
{
    multi = curl_multi_init();
    if(multi == NULL) {
        log_error("curl_multi_init failed");
        return CHTTP_ERR_CURL;
    }
    curl_multi_setopt(multi, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);
    curl_multi_setopt(multi, CURLMOPT_MAX_CONCURRENT_STREAMS, 10L);
    curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, multi_sock_cb);
    curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    return CHTTP_ERR_OK;
}

void chttp_destroy(void)
{
    chttp_req_t *req = req_list.lh_first;
    while(req) {
        chttp_req_t *next = req->entry.le_next;
        req_free(req);
        req = next;
    }

    ev_timer_stop(EV_DEFAULT, &timer);
    curl_multi_cleanup(multi);
    multi = NULL;
}

static size_t recv_cb(void *ptr, size_t size, size_t nmemb, void *req_data)
{
    chttp_req_t *req = req_data;
    size *= nmemb;
    if(req_data == NULL) {
        log_error("req_data in NULL");
        return 0;
    }
    if((req->body.len + size + 1) > BODY_BUF_SIZE) {
        log_error("buffer overflow, body_len=%zu size=%zu", req->body.len, size);
        return 0;
    }

    if(req->body.data == NULL) {
        req->body.data = malloc(BODY_BUF_SIZE);
        if(req->body.data == NULL) {
            log_error("malloc body buffer failed");
            return 0;
        }
    }

    memcpy(req->body.data + req->body.len, ptr, size);
    req->body.len += size;
    req->body.data[req->body.len] = '\0';
    return size;
}

static chttp_err_t http_req(const char *url, const chttp_mime_t *mimes, uint32_t mimes_count, chttp_resp_cb_t cb,
                            const buf_t *user_data)
{
    size_t req_size = sizeof(chttp_req_t);
    if(user_data) {
        req_size += user_data->size;
    }
    chttp_req_t *req = malloc(req_size);
    if(req == NULL) {
        log_error("calloc chttp_req_t failed, size=%zu", req_size);
        return CHTTP_ERR_MEM_ALLOC;
    }
    bzero(req, sizeof(chttp_req_t));
    LIST_INSERT_HEAD(&req_list, req, entry);
    if(user_data) {
        memcpy(req->user_data, user_data->data, user_data->size);
    }
    req->resp_cb = cb;

    req->curl = curl_easy_init();
    if(req->curl == NULL) {
        log_error("curl_easy_init failed");
        req_free(req);
        return CHTTP_ERR_CURL;
    }
    curl_easy_setopt(req->curl, CURLOPT_URL, url);
    curl_easy_setopt(req->curl, CURLOPT_PRIVATE, req);
    curl_easy_setopt(req->curl, CURLOPT_WRITEFUNCTION, recv_cb);
    curl_easy_setopt(req->curl, CURLOPT_WRITEDATA, req);

    if(mimes_count > 0) {
        req->mime = curl_mime_init(req->curl);
        if(req->mime == NULL) {
            log_error("curl_mime_init failed");
            req_free(req);
            return CHTTP_ERR_CURL;
        }

        for(uint32_t i = 0; i < mimes_count; i++) {
            curl_mimepart *part = curl_mime_addpart(req->mime);
            if(part == NULL) {
                log_error("curl_mime_addpart failed");
                req_free(req);
                return CHTTP_ERR_CURL;
            }

            const chttp_mime_t *mime = &mimes[i];
            curl_mime_name(part, mime->name);
            if(mime->type == CHTTP_MIME_TYPE_FILE) {
                curl_mime_filedata(part, mime->data);
            } else {
                curl_mime_data(part, mime->data, CURL_ZERO_TERMINATED);
            }
        }
        curl_easy_setopt(req->curl, CURLOPT_MIMEPOST, req->mime);
    }

    CURLMcode rc = curl_multi_add_handle(multi, req->curl);
    if(rc != CURLM_OK) {
        log_error("curl_multi_add_handle - %s", curl_multi_strerror(rc));
        req_free(req);
        return CHTTP_ERR_CURL;
    }

    log_info("get %s", url);
    return CHTTP_ERR_OK;
}

chttp_err_t chttp_get(const char *url, chttp_resp_cb_t cb, const buf_t *user_data)
{
    return http_req(url, NULL, 0, cb, user_data);
}

chttp_err_t chttp_post(const char *url, const chttp_mime_t *mimes, uint32_t mimes_count, chttp_resp_cb_t cb,
                       const buf_t *user_data)
{
    return http_req(url, mimes, mimes_count, cb, user_data);
}
