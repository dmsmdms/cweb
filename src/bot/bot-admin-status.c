#include <bot/bot-admin-status.h>
#include <ipc/ipc-crypto-parser-client.h>
#include <core/telebot/telebot-method.h>
#include <core/base/str.h>
#include <core/base/log.h>
#include <db/db-bot.h>
#include <time.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define BOT_MSG_SIZE_MAX  1024
#define BOT_ALERT_SEC_MAX 60

typedef struct {
    uint64_t chat_id;
} status_priv_data_t;

typedef struct {
#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
    time_t crypto_parser_last_upd_ts;
#endif
} status_t;

static status_t status = { 0 };

static void send_status_msg(const status_priv_data_t *data, const str_buf_t *buf)
{
    if(buf->data[0] == '\0') {
        return;
    }
    if(buf->offset >= buf->size) {
        log_error("msg size overflow");
        return;
    }
    if(data->chat_id == UINT64_MAX) {
        char buf_mem[BOT_CHAT_ARR_BUF_SIZE];
        buf_ext_t buf_ext;
        buf_init_ext(&buf_ext, buf_mem, sizeof(buf_mem));
        bot_chat_arr_t arr;
        if(db_bot_chat_arr_get(&arr, &buf_ext) != DB_ERR_OK) {
            return;
        }
        for(uint32_t i = 0; i < arr.count; i++) {
            telebot_send_message(arr.data[i].id, buf->data);
        }
    } else {
        telebot_send_message(data->chat_id, buf->data);
    }
}

#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
static void crypto_parser_get_status_cb(const cipc_resp_t *resp)
{
    const status_priv_data_t *data = resp->user_data;
    char buf_mem[BOT_MSG_SIZE_MAX];
    str_buf_t buf;
    str_buf_init(&buf, buf_mem, sizeof(buf_mem));

    if(resp->err == CIPC_ERR_OK) {
        time_t min_ts = time(NULL) - BOT_ALERT_SEC_MAX;
        ipc_crypto_parser_status_t *res = resp->body.data;
        bool is_outdated = (res->last_upd_ts < (uint64_t)min_ts);
        status.crypto_parser_last_upd_ts = res->last_upd_ts;

        if(data->chat_id != UINT64_MAX || is_outdated) {
            buf_puts(&buf, "Crypto Parser Status:\n");
            buf_strtime(&buf, "Start Time: %Y-%m-%d %H:%M:%S\n", res->start_ts);
            buf_strtime(&buf, "Last Update Time: %Y-%m-%d %H:%M:%S\n", res->last_upd_ts);
            buf_printf(&buf, "DB Used Size: %u.%03uMB\n", res->db_used_size_kb / 1024, res->db_used_size_kb % 1024);
            buf_printf(&buf, "DB Total Size: %u.%03uMB\n", res->db_tot_size_kb / 1024, res->db_tot_size_kb % 1024);
            if(is_outdated) {
                buf_puts(&buf, "WARNING: Crypto Parser status is outdated!\n");
            }
        }
    } else {
        if(resp->body.size) {
            static const char *const fail_str[] = {
                [IPC_CRYPTO_PARSER_ERR_DB] = "DB",
            };
            ipc_crypto_parser_fail_t *fail = resp->body.data;
            buf_printf(&buf, "Crypto Parser Error: %s", fail_str[fail->err]);
        } else {
            buf_puts(&buf, "Failed to get Crypto Parser status");
        }
    }

    buf_putc(&buf, '\0');
    send_status_msg(data, &buf);
}

bot_admin_err_t bot_admin_send_crypto_parser_status(uint64_t chat_id)
{
    status_priv_data_t data = {
        .chat_id = chat_id,
    };
    buf_t buf_data = {
        .data = &data,
        .size = sizeof(data),
    };
    if(cipc_crypto_parser_get_status(crypto_parser_get_status_cb, &buf_data) != CIPC_ERR_OK) {
        return BOT_ADMIN_ERR_IPC;
    }
    return BOT_ADMIN_ERR_OK;
}
#endif

void bot_admin_status_upd(UNUSED struct ev_loop *loop, UNUSED ev_timer *timer, UNUSED int events)
{
    status_priv_data_t data = {
        .chat_id = UINT64_MAX,
    };
    buf_t buf_data = {
        .data = &data,
        .size = sizeof(data),
    };
    time_t min_ts = time(NULL) - BOT_ALERT_SEC_MAX;
#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
    if(status.crypto_parser_last_upd_ts < min_ts) {
        cipc_crypto_parser_get_status(crypto_parser_get_status_cb, &buf_data);
    }
#endif
}
