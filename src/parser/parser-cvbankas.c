#include <global.h>

static const gumbo_xpath_t page_count_xpath[] = {
    XP_TAG(GUMBO_TAG_DIV, 1),  XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_DIV, 5),
    XP_TAG(GUMBO_TAG_MAIN, 1), XP_TAG(GUMBO_TAG_UL, 1),  XP_TAG(GUMBO_TAG_LI, 1),
    XP_TAG(GUMBO_TAG_UL, 1),   XP_TAG(GUMBO_TAG_LI, 7),  XP_TAG(GUMBO_TAG_A, 1),
};
static const gumbo_xpath_t job_count_xpath[] = {
    XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_DIV, 1),  XP_TAG(GUMBO_TAG_DIV, 2), XP_TAG(GUMBO_TAG_DIV, 1),
    XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_FORM, 1), XP_TAG(GUMBO_TAG_H1, 1),  XP_TAG(GUMBO_TAG_B, 1),
};
static const gumbo_xpath_t job_list_xpath[] = {
    XP_TAG(GUMBO_TAG_DIV, 1),  XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_DIV, 5),
    XP_TAG(GUMBO_TAG_MAIN, 1), XP_TAG(GUMBO_TAG_DIV, 1),
};
static const gumbo_xpath_t article_vip_xpath[] = {
    XP_TAG(GUMBO_TAG_DIV, 1),
};
static const gumbo_xpath_t article_url_xpath[] = {
    XP_TAG(GUMBO_TAG_A, 1),
};

static void timer_renew(app_t *app)
{
    parser_cvb_t *cvb = &app->parser_cvb;
    if(app->is_running && cvb->parse_vip_done && cvb->parse_normal_done && cvb->parsed_jobs == cvb->new_jobs) {
        ev_timer_set(&cvb->update_timer, app->cfg.parser_cvb_upd_sec, 0);
        ev_timer_start(app->loop, &cvb->update_timer);
    }
}

static void job_parse_done(app_t *app, GumboOutput *html)
{
    parser_cvb_t *cvb = &app->parser_cvb;
    if(html) {
        gumbo_destroy_wrap(app, html);
    }
    cvb->parsed_jobs++;
    timer_renew(app);
}

static void job_parse_cb(const http_resp_t *resp)
{
    app_t *app = resp->app;
    GumboOutput *html = gumbo_parse_wrap(app, resp->data, resp->size);
    if(html == NULL || html->root == NULL) {
        log_error("parse %s failed", resp->url);
        job_parse_done(app, html);
        return;
    }
    parser_cvb_job_parse(app, html->root, resp->url);
    job_parse_done(app, html);
}

static void page_parse_done(app_t *app, GumboOutput *html)
{
    parser_cvb_t *cvb = &app->parser_cvb;
    if(html) {
        gumbo_destroy_wrap(app, html);
    }
    cvb->parse_vip_done = true;
    cvb->parse_normal_done = true;
    timer_renew(app);
}

static void page_parse(app_t *app);
static void page_parse_cb(const http_resp_t *resp)
{
    app_t *app = resp->app;
    parser_cvb_t *cvb = &app->parser_cvb;
    GumboOutput *html = gumbo_parse_wrap(app, resp->data, resp->size);
    if(html == NULL || html->root == NULL) {
        log_error("parse %s failed", resp->url);
        page_parse_done(app, html);
        return;
    }

    if(cvb->page_count == 0) {
        cvb->page_count = gumbo_get_int_by_xpath(html->root, page_count_xpath, ARRAY_SIZE(page_count_xpath));
        if(cvb->page_count == 0) {
            log_error("get page_count %s failed", resp->url);
            page_parse_done(app, html);
            return;
        }
    }
    if(cvb->job_count == 0) {
        cvb->job_count = gumbo_get_int_by_xpath(html->root, job_count_xpath, ARRAY_SIZE(job_count_xpath));
        if(cvb->job_count == 0) {
            log_error("get job_count %s failed", resp->url);
            page_parse_done(app, html);
            return;
        }
    }

    GumboNode *job_list_node = gumbo_get_element_by_xpath(html->root, job_list_xpath, ARRAY_SIZE(job_list_xpath));
    if(job_list_node == NULL) {
        log_error("get job_list %s failed", resp->url);
        page_parse_done(app, html);
        return;
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
        if(!db_job_get_by_url(app, url, NULL)) {
            cvb->new_jobs++;
            http_get(app, url, job_parse_cb, NULL, 0);
        } else {
            if(db_job_get_count(app) >= cvb->job_count) {
                if(gumbo_has_class_by_xpath(child, article_vip_xpath, ARRAY_SIZE(article_vip_xpath),
                                            "jobadlist_article_vip_icon")) {
                    cvb->parse_vip_done = true;
                } else {
                    cvb->parse_normal_done = true;
                }
            }
            if(cvb->parse_vip_done && cvb->parse_normal_done) {
                page_parse_done(app, html);
                return;
            }
        }
    }

    if(cvb->current_page < cvb->page_count) {
        if(app->is_running) {
            gumbo_destroy_wrap(app, html);
            cvb->current_page++;
            page_parse(app);
            return;
        }
    }
    page_parse_done(app, html);
}

static void page_parse(app_t *app)
{
    parser_cvb_t *cvb = &app->parser_cvb;
    char url[64];
    sprintf(url, "https://www.cvbankas.lt/?page=%u", cvb->current_page);
    http_get(app, url, page_parse_cb, NULL, 0);
}

static void update_cb(struct ev_loop *loop, ev_timer *timer, int events)
{
    UNUSED(loop);
    UNUSED(events);
    app_t *app = timer->data;
    parser_cvb_t *cvb = &app->parser_cvb;
    if(!cvb->parse_vip_done || !cvb->parse_normal_done || cvb->parsed_jobs != cvb->new_jobs) {
        log_warn("cvbankas parse not done, skip this round");
        return;
    }
    cvb->current_page = 1;
    cvb->page_count = 0;
    cvb->job_count = 0;
    cvb->new_jobs = 0;
    cvb->parsed_jobs = 0;
    cvb->parse_vip_done = false;
    cvb->parse_normal_done = false;
    page_parse(app);
}

void parser_cvb_init(app_t *app)
{
    parser_cvb_t *cvb = &app->parser_cvb;
    cvb->parse_vip_done = true;
    cvb->parse_normal_done = true;

    ev_timer_init(&cvb->update_timer, update_cb, 0, 0);
    cvb->update_timer.data = app;
    ev_timer_start(app->loop, &cvb->update_timer);
}

void parser_cvb_destroy(app_t *app)
{
    parser_cvb_t *cvb = &app->parser_cvb;
    ev_timer_stop(app->loop, &cvb->update_timer);
}
