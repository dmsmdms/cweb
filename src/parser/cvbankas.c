#include <global.h>

#define UPDATE_INTERVAL_SEC 5

static const gumbo_xpath_t page_count_xpath[] = {
    { GUMBO_TAG_DIV, 1 }, { GUMBO_TAG_DIV, 1 }, { GUMBO_TAG_DIV, 5 }, { GUMBO_TAG_MAIN, 1 }, { GUMBO_TAG_UL, 1 },
    { GUMBO_TAG_LI, 1 },  { GUMBO_TAG_UL, 1 },  { GUMBO_TAG_LI, 7 },  { GUMBO_TAG_A, 1 },
};
static const gumbo_xpath_t job_count_xpath[] = {
    { GUMBO_TAG_DIV, 1 }, { GUMBO_TAG_DIV, 1 },  { GUMBO_TAG_DIV, 2 }, { GUMBO_TAG_DIV, 1 },
    { GUMBO_TAG_DIV, 1 }, { GUMBO_TAG_FORM, 1 }, { GUMBO_TAG_H1, 1 },  { GUMBO_TAG_B, 1 },
};
static const gumbo_xpath_t job_list_xpath[] = {
    { GUMBO_TAG_DIV, 1 }, { GUMBO_TAG_DIV, 1 }, { GUMBO_TAG_DIV, 5 }, { GUMBO_TAG_MAIN, 1 }, { GUMBO_TAG_DIV, 1 },
};
static const gumbo_xpath_t article_vip_xpath[] = {
    { GUMBO_TAG_DIV, 1 },
};
static const gumbo_xpath_t article_url_xpath[] = {
    { GUMBO_TAG_A, 1 },
};

static void page_parse(app_t *app);
static void page_parse_cb(const http_resp_t *resp)
{
    app_t *app = resp->priv_data;
    cvbankas_t *cvb = &app->cvbankas;
    GumboOutput *html = gumbo_parse_wrap(resp->data, resp->size);
    if(html == NULL || html->root == NULL) {
        log_error("parse %s failed", resp->url);
        goto end;
    }

    if(cvb->page_count == 0) {
        cvb->page_count = gumbo_get_int_by_xpath(html->root, page_count_xpath, ARRAY_SIZE(page_count_xpath));
        if(cvb->page_count == 0) {
            log_error("get page_count %s failed", resp->url);
            goto cleanup;
        }
    }
    if(cvb->job_count == 0) {
        cvb->job_count = gumbo_get_int_by_xpath(html->root, job_count_xpath, ARRAY_SIZE(job_count_xpath));
        if(cvb->job_count == 0) {
            log_error("get job_count %s failed", resp->url);
            goto cleanup;
        }
    }

    GumboNode *job_list_node = gumbo_get_element_by_xpath(html->root, job_list_xpath, ARRAY_SIZE(job_list_xpath));
    if(job_list_node == NULL) {
        log_error("get job_list %s failed", resp->url);
        goto cleanup;
    }
    GumboVector *children = &job_list_node->v.element.children;
    for(uint32_t i = 0; i < children->length; i++) {
        GumboNode *child = children->data[i];
        if(child->type != GUMBO_NODE_ELEMENT || child->v.element.tag != GUMBO_TAG_ARTICLE) {
            continue;
        }
        const char *url = gumbo_get_attr_by_xpath(child, article_url_xpath, ARRAY_SIZE(article_url_xpath), "href");
        if(url == NULL) {
            log_error("get article url[%u] on %s failed", i, resp->url);
            continue;
        }
        if(db_job_get_by_url(&app->db, url) == NULL) {
            // Add job
        } else {
            if(gumbo_has_class_by_xpath(child, article_vip_xpath, ARRAY_SIZE(article_vip_xpath),
                                        "jobadlist_article_vip_icon")) {
                cvb->parse_vip_done = true;
            } else {
                cvb->parse_normal_done = true;
            }
            if(cvb->parse_vip_done && cvb->parse_normal_done) {
                goto cleanup;
            }
        }
    }

    if(cvb->current_page < cvb->page_count) {
        gumbo_destroy_wrap(html);
        if(app->is_running) {
            cvb->current_page++;
            page_parse(app);
        }
        return;
    }

cleanup:
    gumbo_destroy_wrap(html);
end:
    cvb->parse_vip_done = true;
    cvb->parse_normal_done = true;
    ev_timer_set(&cvb->update_timer, UPDATE_INTERVAL_SEC, 0);
    ev_timer_start(app->loop, &cvb->update_timer);
}

static void page_parse(app_t *app)
{
    cvbankas_t *cvb = &app->cvbankas;
    char url[64];
    sprintf(url, "https://www.cvbankas.lt/?page=%u", cvb->current_page);
    http_get(&app->http, url, page_parse_cb, app);
}

static void update_cb(struct ev_loop *loop, ev_timer *timer, int events)
{
    app_t *app = timer->data;
    cvbankas_t *cvb = &app->cvbankas;
    if(cvb->parse_vip_done == false || cvb->parse_normal_done == false) {
        log_warn("cvbankas parse not done, skip this round");
        return;
    }
    cvb->current_page = 1;
    cvb->page_count = 0;
    cvb->job_count = 0;
    cvb->parse_vip_done = false;
    cvb->parse_normal_done = false;
    page_parse(app);
}

void cvbankas_init(cvbankas_t *cvb)
{
    app_t *app = container_of(cvb, app_t, cvbankas);
    cvb->parse_vip_done = true;
    cvb->parse_normal_done = true;

    ev_timer_init(&cvb->update_timer, update_cb, 0, 0);
    cvb->update_timer.data = app;
    ev_timer_start(app->loop, &cvb->update_timer);
}

void cvbankas_destroy(cvbankas_t *cvb)
{
    app_t *app = container_of(cvb, app_t, cvbankas);
    ev_timer_stop(app->loop, &cvb->update_timer);
}
