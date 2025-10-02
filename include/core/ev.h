#pragma once

#include <common.h>
#include <sys/epoll.h>

/**
 * @brief Timer info prototype
 */
typedef struct ev_timer ev_timer_t;

/**
 * @brief IO event info prototype
 */
typedef struct ev_io ev_io_t;

/**
 * @brief Timer callback
 * @param timer - [in] Pointer to timer
 */
typedef void (*ev_timer_cb_t)(const ev_timer_t *timer);

/**
 * @brief IO event callback
 * @param io - [in] Pointer to event
 * @param events - [in] Bitmask of events (EPOLLIN, EPOLLOUT, etc.)
 */
typedef void (*ev_io_cb_t)(const ev_io_t *io, uint32_t events);

/**
 * @brief Timer info
 */
struct ev_timer {
    LIST_ENTRY(ev_timer) entries;
    ev_timer_cb_t cb;
    void *priv_data;
    time_t next_call_ms;
    time_t interval_ms;
};

/**
 * @brief IO event info
 */
struct ev_io {
    LIST_ENTRY(ev_io) entries;
    ev_io_cb_t cb;
    void *priv_data;
    uint32_t events;
    int fd;
};

/**
 * @brief Timers list
 */
typedef LIST_HEAD(ev_timer_list, ev_timer) ev_timer_list_t;

/**
 * @brief IO events list
 */
typedef LIST_HEAD(ev_io_list, ev_io) ev_io_list_t;

/**
 * @brief Event loop info
 */
typedef struct {
    ev_timer_list_t timer_list;
    ev_io_list_t io_list;
    time_t next_call_ms;
    int epoll_fd;
    bool is_running;
    bool is_inited;
} ev_loop_t;

/**
 * @brief Initialize event loop
 * @param loop - [out] Pointer to event loop
 * @returns true on success, false on failure
 */
bool ev_init(ev_loop_t *loop);

/**
 * @brief Run event loop
 * @param loop - [in] Pointer to event loop
 */
void ev_run(ev_loop_t *loop);

/**
 * @brief Deinitialize event loop
 * @param loop - [in] Pointer to event loop
 */
void ev_deinit(ev_loop_t *loop);

/**
 * @brief Add timer to event loop
 * @param loop - [in] Pointer to event loop
 * @param timer - [out] Pointer to timer
 * @param cb - [in] Timer callback
 * @param priv_data - [in] Private data for callback
 * @param interval_ms - [in] Interval in milliseconds
 * @returns true on success, false on failure
 */
bool ev_timer_add(ev_loop_t *loop, ev_timer_t *timer, ev_timer_cb_t cb, void *priv_data, time_t interval_ms);

/**
 * @brief Modify timer in event loop
 * @param loop - [in] Pointer to event loop
 * @param timer - [in] Pointer to timer
 * @param interval_ms - [in] New interval in milliseconds
 * @returns true on success, false on failure
 */
bool ev_timer_mod(ev_loop_t *loop, ev_timer_t *timer, time_t interval_ms);

/**
 * @brief Remove timer from event loop
 * @param loop - [in] Pointer to event loop
 * @param timer - [in] Pointer to timer
 */
void ev_timer_del(ev_loop_t *loop, ev_timer_t *timer);

/**
 * @brief Add IO event to event loop
 * @param loop - [in] Pointer to event loop
 * @param io - [in] Pointer to IO event
 * @param cb - [in] IO event callback
 * @param priv_data - [in] Private data for callback
 * @param fd - [in] File descriptor to monitor
 * @param events - [in] Bitmask of events (EPOLLIN, EPOLLOUT, etc.)
 * @returns true on success, false on failure
 */
bool ev_io_add(ev_loop_t *loop, ev_io_t *io, ev_io_cb_t cb, void *priv_data, int fd, uint32_t events);

/**
 * @brief Modify IO event in event loop
 * @param loop - [in] Pointer to event loop
 * @param io - [in] Pointer to IO event
 * @param events - [in] New bitmask of events (EPOLLIN, EPOLLOUT, etc.)
 * @returns true on success, false on failure
 */
bool ev_io_mod(ev_loop_t *loop, ev_io_t *io, uint32_t events);

/**
 * @brief Remove IO event from event loop
 * @param loop - [in] Pointer to event loop
 * @param io - [in] Pointer to IO event
 */
void ev_io_del(ev_loop_t *loop, ev_io_t *io);
