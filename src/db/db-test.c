#include <common.h>

typedef struct {
    uint16_t table_id;
    uint16_t sym_id;
    uint32_t timestamp;
} db_table_key_t;
STATIC_ASSERT(sizeof(db_table_key_t) == 8);

typedef struct {
    float price;
    float volume;
    float liq_ask;
    float liq_bid;
    uint8_t whales;
    uint8_t pad[3];
} db_table_val_t;
STATIC_ASSERT(sizeof(db_table_val_t) == 20);

typedef struct {
    char name[14];
    uint8_t offset;
    uint8_t size;
} db_table_item_t;
STATIC_ASSERT(sizeof(db_table_item_t) == 16);

#define DB_TABLE_ITEM(type, item) { #item, offsetof(type, item), sizeof(((type *)0)->item) }

static const db_table_item_t key_items[] = {
    DB_TABLE_ITEM(db_table_key_t, table_id),
    DB_TABLE_ITEM(db_table_key_t, sym_id),
    DB_TABLE_ITEM(db_table_key_t, timestamp),
};
static const db_table_item_t val_items[] = {
    DB_TABLE_ITEM(db_table_val_t, price),   DB_TABLE_ITEM(db_table_val_t, volume),
    DB_TABLE_ITEM(db_table_val_t, liq_ask), DB_TABLE_ITEM(db_table_val_t, liq_bid),
    DB_TABLE_ITEM(db_table_val_t, whales),
};
