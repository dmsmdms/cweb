#pragma once

#include <db/db.h>

#define DB_CRYPTO_SYM_NAME_LEN 12

typedef struct {
    char sym_name[DB_CRYPTO_SYM_NAME_LEN];
    uint32_t sym_id;
    bool is_local;
} db_crypto_sym_t;

typedef struct {
    db_crypto_sym_t *data;
    uint32_t count;
} db_crypto_sym_arr_t;

db_err_t db_crypto_sym_add_global(const char *sym_name);

db_err_t db_crypto_sym_del_global(const char *sym_name);

db_err_t db_crypto_sym_get_list(buf_ext_t *buf, db_crypto_sym_arr_t *arr, bool is_local);

db_err_t db_crypto_sym_set_local(const char *sym_name, bool is_local);
