#include <ipc/ipc-crypto-parser-client.h>

static cipc_t *cipc = NULL;

cipc_err_t cipc_crypto_parser_init(const char *sock_path)
{
    return cipc_init(&cipc, sock_path);
}

void cipc_crypto_parser_deinit(void)
{
    cipc_deinit(cipc);
    cipc = NULL;
}

cipc_err_t cipc_crypto_parser_get_status(cipc_resp_cb_t cb, const buf_t *user_data)
{
    return cipc_send(cipc, IPC_CRYPTO_PARSER_CMD_GET_STATUS, NULL, cb, user_data);
}
