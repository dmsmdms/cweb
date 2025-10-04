#include <global.h>

static bool db_job_put(db_t *db, const job_t *job, uint32_t id)
{
    app_t *app = container_of(db, app_t, db);
    uint32_t url_len = strlen(job->url) + 1;
    uint32_t city_len = strlen(job->city) + 1;
    uint32_t title_len = strlen(job->title) + 1;
    uint32_t description_len = strlen(job->description) + 1;
    uint32_t addr_len = job->addr ? strlen(job->addr) + 1 : 0;
    uint32_t total_size = offsetof(db_job_t, data) + url_len + city_len + title_len + description_len + addr_len;
    db_job_t db_job = {
        .salary_min = job->salary_min,
        .salary_max = job->salary_max,
        .source = job->source,
        .currency = job->currency,
        .payment = job->payment,
        .taxes = job->taxes,
        .for_ukraine = job->for_ukraine,
        .for_disabled = job->for_disabled,
    };
    if(total_size > sizeof(db_job.data)) {
        log_error("job size=%u is too long", total_size);
        return false;
    }
    uint32_t offset = 0;
    memcpy(db_job.data + offset, job->url, url_len);
    db_job.url_offset = offset;
    offset += url_len;
    memcpy(db_job.data + offset, job->city, city_len);
    db_job.city_offset = offset;
    offset += city_len;
    memcpy(db_job.data + offset, job->title, title_len);
    db_job.title_offset = offset;
    offset += title_len;
    memcpy(db_job.data + offset, job->description, description_len);
    db_job.description_offset = offset;
    offset += description_len;
    if(addr_len) {
        memcpy(db_job.data + offset, job->addr, addr_len);
        db_job.addr_offset = offset;
        offset += addr_len;
    }
    return db_put_value_by_id(db, DB_TABLE_JOB_LT, id, &db_job, total_size);
}

bool db_job_add(db_t *db, const job_t *job)
{
    app_t *app = container_of(db, app_t, db);
    const db_job_meta_t *meta = db_get_meta(db, DB_TABLE_JOB_LT);
    if(meta == NULL) {
        log_error("job meta not found");
        return false;
    }

    db_job_meta_t new_meta = *meta;
    new_meta.last_id++;
    if(db_job_put(db, job, new_meta.last_id) == false) {
        goto abort;
    }
    if(db_put_meta(db, DB_TABLE_JOB_LT, &new_meta, sizeof(new_meta)) == false) {
        goto abort;
    }
    if(db_put_id_by_url(db, DB_TABLE_JOB_LT_URL, job->url, DB_TABLE_JOB_LT, new_meta.last_id) == false) {
        goto abort;
    }
    return true;
abort:
    return false;
}

bool db_job_get_by_url(const db_t *db, const char *url, job_t *job)
{
    app_t *app = container_of(db, app_t, db);
    const db_key_id_t *key_id = db_get_id_by_url(db, DB_TABLE_JOB_LT_URL, url);
    if(key_id == NULL) {
        return false;
    }
    if(job) {
        const db_job_t *db_job = db_get_value_by_id(db, key_id->table, key_id->id);
        if(db_job == NULL) {
            return false;
        }
        job->url = db_job->data + db_job->url_offset;
        job->city = db_job->data + db_job->city_offset;
        job->title = db_job->data + db_job->title_offset;
        job->description = db_job->data + db_job->description_offset;
        job->addr = db_job->addr_offset ? db_job->data + db_job->addr_offset : NULL;
        job->salary_min = db_job->salary_min;
        job->salary_max = db_job->salary_max;
        job->source = db_job->source;
        job->currency = db_job->currency;
        job->payment = db_job->payment;
        job->taxes = db_job->taxes;
        job->for_ukraine = db_job->for_ukraine;
        job->for_disabled = db_job->for_disabled;
    }
    return true;
}
