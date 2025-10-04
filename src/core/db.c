#include <global.h>

void db_txn_begin(db_t *db)
{
    int rc = mdb_txn_begin(db->env, NULL, 0, &db->txn);
}

bool db_open(db_t *db, const char *dir)
{
    return 1;
}

void db_close(db_t *db)
{
}
