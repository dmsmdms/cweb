#include <ipc/ipc-crypto-parser-server.h>
#include <parser/parser-binance.h>
#include <core/db/db.h>

static sipc_err_t resp_fail(sipc_conn_t *conn, uint32_t id, ipc_crypto_parser_err_t err)
{
    ipc_crypto_parser_fail_t resp = {
        .err = err,
    };
    buf_t buf = {
        .data = &resp,
        .size = sizeof(resp),
    };
    return sipc_resp(conn, id, IPC_CMD_FAIL, &buf);
}

static sipc_err_t get_status(const sipc_req_t *req)
{
    parser_bin_stat_t bin_stat;
    parser_bin_get_stat(&bin_stat);
    db_stat_t db_stat;
    if(db_get_stat(&db_stat) != DB_ERR_OK) {
        return resp_fail(req->conn, req->id, IPC_CRYPTO_PARSER_ERR_DB);
    }
    ipc_crypto_parser_status_t status = {
        .start_ts = bin_stat.start_ts,
        .last_upd_ts = bin_stat.last_upd_ts,
        .db_used_size_kb = db_stat.used_size / 1024,
        .db_tot_size_kb = db_stat.tot_size / 1024,
    };
    buf_t buf = {
        .data = &status,
        .size = sizeof(status),
    };
    return sipc_resp(req->conn, req->id, IPC_CMD_OK, &buf);
}

sipc_err_t sipc_crypto_parser_init(const char *sock_path)
{
    static const sipc_cmd_handler_t handlers[] = {
        { IPC_CRYPTO_PARSER_CMD_GET_STATUS, get_status },
    };
    return sipc_init(sock_path, handlers, ARRAY_SIZE(handlers));
}
