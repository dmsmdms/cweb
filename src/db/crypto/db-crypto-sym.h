#pragma once

#include <db/db-table.h>

#define DB_CRYPTO_SYM_NAME_LEN 12
#define DB_CRYPTO_SYM_ARR_SIZE 1024

typedef struct {
    db_table_id_t table_id;
} db_crypto_sym_meta_key_t;

typedef struct {
    uint32_t last_sym_id;
} db_crypto_sym_meta_val_t;

typedef struct {
    db_table_id_t table_id;
    char sym_name[DB_CRYPTO_SYM_NAME_LEN];
} db_crypto_sym_key_t;

typedef struct {
    uint32_t sym_id;
    bool is_local;
} db_crypto_sym_val_t;

typedef struct {
    char sym_name[DB_CRYPTO_SYM_NAME_LEN];
    db_crypto_sym_val_t val;
} db_crypto_sym_t;

db_err_t db_crypto_sym_add_global(const char *sym_name);

db_err_t db_crypto_sym_del_global(const char *sym_name);

db_err_t db_crypto_sym_set_local(const char *sym_name, bool is_local);

db_err_t db_crypto_sym_get_list(db_crypto_sym_t *arr, uint32_t arr_size, uint32_t *sym_count);
