#include <global.h>

#define MAX_EVENTS 64

static time_t get_current_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

bool ev_init(ev_loop_t *loop)
{
    app_t *app = container_of(loop, app_t, loop);
    if(loop->is_inited) {
        log_error("event loop already initialized");
        return false;
    }

    loop->epoll_fd = epoll_create1(0);
    if(loop->epoll_fd < 0) {
        log_error("epoll_create1 failed: %s", strerror(errno));
        return false;
    }

    LIST_INIT(&loop->timer_list);
    LIST_INIT(&loop->io_list);
    loop->next_call_ms = INT64_MAX;
    loop->is_inited = true;

    return true;
}

void ev_run(ev_loop_t *loop)
{
    app_t *app = container_of(loop, app_t, loop);
    loop->is_running = true;
    while(loop->is_running) {
        int timeout_ms = -1;
        if(loop->next_call_ms != INT64_MAX) {
            time_t now_ms = get_current_ms();
            if(loop->next_call_ms <= now_ms) {
                timeout_ms = 0;
            } else {
                timeout_ms = loop->next_call_ms - now_ms;
            }
        }

        struct epoll_event events[MAX_EVENTS];
        int n = epoll_wait(loop->epoll_fd, events, MAX_EVENTS, timeout_ms);
        if(n < 0) {
            if(errno == EINTR) {
                continue;
            }
            log_error("epoll_wait failed: %s", strerror(errno));
            break;
        }

        for(int i = 0; i < n; i++) {
            ev_io_t *io = events[i].data.ptr;
            io->cb(io, events[i].events);
        }

        ev_timer_t *timer;
        time_t now_ms = get_current_ms();
        LIST_FOREACH(timer, &loop->timer_list, entries)
        {
            if(timer->cb && (timer->next_call_ms <= now_ms)) {
                timer->cb(timer);
                timer->next_call_ms = now_ms + timer->interval_ms;
            }
        }
    }
}

void ev_deinit(ev_loop_t *loop)
{
    app_t *app = container_of(loop, app_t, loop);
    if(loop->is_inited == false) {
        log_error("event loop not initialized");
        return;
    }

    if(loop->epoll_fd >= 0) {
        close(loop->epoll_fd);
    }

    bzero(loop, sizeof(ev_loop_t));
}

static void recalc_next_call(ev_loop_t *loop)
{
    ev_timer_t *timer;
    loop->next_call_ms = INT64_MAX;
    LIST_FOREACH(timer, &loop->timer_list, entries)
    {
        if(timer->next_call_ms < loop->next_call_ms) {
            loop->next_call_ms = timer->next_call_ms;
        }
    }
}

bool ev_timer_add(ev_loop_t *loop, ev_timer_t *timer, ev_timer_cb_t cb, void *priv_data, time_t interval_ms)
{
    app_t *app = container_of(loop, app_t, loop);
    if(timer->cb) {
        log_error("timer already added");
        return false;
    }

    LIST_INSERT_HEAD(&loop->timer_list, timer, entries);
    timer->cb = cb;
    timer->priv_data = priv_data;
    timer->interval_ms = interval_ms;
    timer->next_call_ms = get_current_ms() + interval_ms;
    recalc_next_call(loop);

    return true;
}

bool ev_timer_mod(ev_loop_t *loop, ev_timer_t *timer, time_t interval_ms)
{
    app_t *app = container_of(loop, app_t, loop);
    if(timer->cb == NULL) {
        log_error("timer not added");
        return false;
    }

    timer->interval_ms = interval_ms;
    timer->next_call_ms = get_current_ms() + interval_ms;
    recalc_next_call(loop);

    return true;
}

void ev_timer_del(ev_loop_t *loop, ev_timer_t *timer)
{
    app_t *app = container_of(loop, app_t, loop);
    if(timer->cb == NULL) {
        log_error("timer not added");
        return;
    }

    LIST_REMOVE(timer, entries);
    recalc_next_call(loop);
    bzero(timer, sizeof(ev_timer_t));
}

bool ev_io_add(ev_loop_t *loop, ev_io_t *io, ev_io_cb_t cb, void *priv_data, int fd, uint32_t events)
{
    app_t *app = container_of(loop, app_t, loop);
    if(io->cb) {
        log_error("io already added");
        return false;
    }

    struct epoll_event ev = {
        .events = events,
        .data.ptr = io,
    };
    if(epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        log_error("epoll_ctl ADD failed: %s", strerror(errno));
        return false;
    }

    LIST_INSERT_HEAD(&loop->io_list, io, entries);
    io->cb = cb;
    io->priv_data = priv_data;
    io->fd = fd;
    io->events = events;

    return true;
}

bool ev_io_mod(ev_loop_t *loop, ev_io_t *io, uint32_t events)
{
    app_t *app = container_of(loop, app_t, loop);
    if(io->cb == NULL) {
        log_error("io not added");
        return false;
    }
    if(events == io->events) {
        return true;
    }

    struct epoll_event ev = {
        .events = events,
        .data.ptr = io,
    };
    if(epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, io->fd, &ev) < 0) {
        log_error("epoll_ctl MOD failed: %s", strerror(errno));
        return false;
    }
    io->events = events;

    return true;
}

void ev_io_del(ev_loop_t *loop, ev_io_t *io)
{
    app_t *app = container_of(loop, app_t, loop);
    if(io->cb == NULL) {
        log_error("io not added");
        return;
    }

    if(epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, io->fd, NULL) < 0) {
        log_error("epoll_ctl DEL failed: %s", strerror(errno));
    }

    LIST_REMOVE(io, entries);
    bzero(io, sizeof(ev_io_t));
}
