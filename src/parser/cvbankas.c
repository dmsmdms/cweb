#include <global.h>

#define UPDATE_INTERVAL_MS 1000

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
    char tmp_buf[2 * 1024 * 1024];
    GumboOutput *html = gumbo_parse_wrap(resp->data, resp->size, tmp_buf, sizeof(tmp_buf));
    if(html == NULL || html->root == NULL) {
        log_error("parse %s failed", resp->url);
        return;
    }

    if(cvb->page_count == 0) {
        cvb->page_count = gumbo_get_int_by_xpath(html->root, page_count_xpath, ARRAY_SIZE(page_count_xpath));
        if(cvb->page_count == 0) {
            log_error("get page_count %s failed", resp->url);
            goto exit;
        }
    }
    if(cvb->job_count == 0) {
        cvb->job_count = gumbo_get_int_by_xpath(html->root, job_count_xpath, ARRAY_SIZE(job_count_xpath));
        if(cvb->job_count == 0) {
            log_error("get job_count %s failed", resp->url);
            goto exit;
        }
    }

    GumboNode *job_list_node = gumbo_get_element_by_xpath(html->root, job_list_xpath, ARRAY_SIZE(job_list_xpath));
    if(job_list_node == NULL) {
        log_error("get job_list %s failed", resp->url);
        goto exit;
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
                goto exit;
            }
        }
    }

    if(cvb->current_page < cvb->page_count) {
        cvb->current_page++;
        page_parse(app);
    }
exit:
    gumbo_destroy_wrap(html, tmp_buf);
}

static void page_parse(app_t *app)
{
    cvbankas_t *cvb = &app->cvbankas;
    char url[64];
    sprintf(url, "https://www.cvbankas.lt/?page=%u", cvb->current_page);
    http_get(&app->http, url, page_parse_cb, app);
}

static void update_cb(const ev_timer_t *timer)
{
    app_t *app = timer->priv_data;
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

void cvbankas_init(cvbankas_t *parser)
{
    app_t *app = container_of(parser, app_t, cvbankas);
    parser->parse_vip_done = true;
    parser->parse_normal_done = true;
    ev_timer_add(&app->loop, &parser->update_timer, update_cb, app, UPDATE_INTERVAL_MS);
}

void cvbankas_destroy(cvbankas_t *parser)
{
    app_t *app = container_of(parser, app_t, cvbankas);
    ev_timer_del(&app->loop, &parser->update_timer);
}
