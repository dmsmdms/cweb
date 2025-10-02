#include <global.h>
// #define NO_DEBUG
#include <core/base/log-off.h>

static const gumbo_xpath_t job_content_xpath[] = {
    XP_TAG(GUMBO_TAG_DIV, 1),     XP_TAG(GUMBO_TAG_DIV, 2), XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_MAIN, 1),
    XP_TAG(GUMBO_TAG_ARTICLE, 1), XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_DIV, 1), XP_ID("jobad_content_main"),
};
static const gumbo_xpath_t job_header_xpath[] = {
    XP_TAG(GUMBO_TAG_HEADER, 1),
};
static const gumbo_xpath_t job_city_xpath[] = {
    XP_ID("jobad_location"),
    XP_TAG(GUMBO_TAG_SPAN, 1),
    XP_TAG(GUMBO_TAG_A, 1),
    XP_TAG(GUMBO_TAG_SPAN, 1),
};
static const gumbo_xpath_t job_title_xpath[] = {
    XP_TAG(GUMBO_TAG_H1, 1),
};
static const gumbo_xpath_t job_desc_xpath[] = {
    XP_TAG(GUMBO_TAG_SECTION, 1),
};
static const gumbo_xpath_t job_salary_xpath[] = {
    XP_TAG(GUMBO_TAG_DIV, 1),
    XP_TAG(GUMBO_TAG_SPAN, 1),
    XP_TAG(GUMBO_TAG_SPAN, 1),
    XP_TAG(GUMBO_TAG_SPAN, 1),
};
static const gumbo_xpath_t job_addr_xpath[] = {
    XP_TAG(GUMBO_TAG_DIV, 1),     XP_TAG(GUMBO_TAG_DIV, 2), XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_MAIN, 1),
    XP_TAG(GUMBO_TAG_ARTICLE, 1), XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_SECTION, 1),
    XP_TAG(GUMBO_TAG_ASIDE, 1),   XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_DIV, 1), XP_TAG(GUMBO_TAG_DIV, 2),
};
static const gumbo_xpath_t job_sal_val_xpath[] = {
    XP_TAG(GUMBO_TAG_SPAN, 1),
    XP_TAG(GUMBO_TAG_SPAN, 1),
};
static const gumbo_xpath_t job_cur_pay_xpath[] = {
    XP_TAG(GUMBO_TAG_SPAN, 1),
    XP_TAG(GUMBO_TAG_SPAN, 2),
};
static const gumbo_xpath_t job_taxes_xpath[] = {
    XP_TAG(GUMBO_TAG_SPAN, 2),
};
static const char *job_sal_from[] = { "Nuo", "From", "От" };
static const char *job_sal_to[] = { "Iki", "Upto" };
static const char *job_cur_eur[] = { "€" };
static const char *job_pay_month[] = { "mėn", "mon", "мес" };
static const char *job_pay_day[] = { "d.", "day" };
static const char *job_pay_hour[] = { "val", "hour" };
static const char *job_tax_gross[] = { "Neatskaičius mokesčių", "Gross", "Брутто" };
static const char *job_tax_net[] = { "Į rankas", "Net", "Нетто" };

static bool sal_val_parse(char *buf, job_t *job)
{
    char *end1 = NULL, *end2 = NULL;
    str_remove_char(buf, ' ');
    str_replace_char(buf, ',', '.');
    if(str_remove_str_arr(buf, job_sal_from, ARRAY_SIZE(job_sal_from))) {
        job->salary_min = 100.f * strtof(buf, &end1);
        job->salary_max = 0;
    } else if(str_remove_str_arr(buf, job_sal_to, ARRAY_SIZE(job_sal_to))) {
        job->salary_min = 0;
        job->salary_max = 100.f * strtof(buf, &end2);
    } else {
        char *sep = strchr(buf, '-');
        if(sep) {
            *sep++ = '\0';
            job->salary_min = 100.f * strtof(buf, &end1);
            job->salary_max = 100.f * strtof(sep, &end2);
        } else {
            job->salary_min = 100.f * strtof(buf, &end1);
            job->salary_max = job->salary_min;
        }
    }
    if((end1 && *end1 != '\0') || (end2 && *end2 != '\0')) {
        return false;
    }
    return true;
}

static bool cur_pay_parse(char *buf, job_t *job)
{
    char *sep = strchr(buf, '/');
    if(sep == NULL) {
        return false;
    }
    *sep++ = '\0';
    if(str_in_arr(buf, job_cur_eur, ARRAY_SIZE(job_cur_eur))) {
        job->currency = JOB_CURRENCY_EUR;
    } else {
        return false;
    }
    if(str_in_arr(sep, job_pay_month, ARRAY_SIZE(job_pay_month))) {
        job->payment = JOB_PAYMENT_MONTHLY;
    } else if(str_in_arr(sep, job_pay_day, ARRAY_SIZE(job_pay_day))) {
        job->payment = JOB_PAYMENT_DAYLY;
    } else if(str_in_arr(sep, job_pay_hour, ARRAY_SIZE(job_pay_hour))) {
        job->payment = JOB_PAYMENT_HOURLY;
    } else {
        return false;
    }
    return true;
}

static bool tax_parse(char *buf, job_t *job)
{
    if(str_in_arr(buf, job_tax_gross, ARRAY_SIZE(job_tax_gross))) {
        job->taxes = JOB_TAXES_GROSS;
    } else if(str_in_arr(buf, job_tax_net, ARRAY_SIZE(job_tax_net))) {
        job->taxes = JOB_TAXES_NET;
    } else {
        return false;
    }
    return true;
}

static bool salary_parse(app_t *app, GumboNode *node, job_t *job)
{
    char sal_val[64];
    if(!gumbo_get_str_by_xpath(node, job_sal_val_xpath, ARRAY_SIZE(job_sal_val_xpath), sal_val, sizeof(sal_val))) {
        log_error("get salary %s failed", job->url);
        return false;
    }
    if(!sal_val_parse(sal_val, job)) {
        log_error("parse salary='%s' on %s failed", sal_val, job->url);
        return false;
    }
    char cur_pay[64];
    if(!gumbo_get_str_by_xpath(node, job_cur_pay_xpath, ARRAY_SIZE(job_cur_pay_xpath), cur_pay, sizeof(cur_pay))) {
        log_error("get currency/payment %s failed", job->url);
        return false;
    }
    if(!cur_pay_parse(cur_pay, job)) {
        log_error("parse currency/payment='%s' on %s failed", cur_pay, job->url);
        return false;
    }
    char taxes[64];
    if(!gumbo_get_str_by_xpath(node, job_taxes_xpath, ARRAY_SIZE(job_taxes_xpath), taxes, sizeof(taxes))) {
        log_error("get taxes %s failed", job->url);
        return false;
    }
    if(!tax_parse(taxes, job)) {
        log_error("parse taxes='%s' on %s failed", taxes, job->url);
        return false;
    }
    return true;
}

void parser_cvb_job_parse(app_t *app, GumboNode *root, const char *url)
{
    GumboNode *content = gumbo_get_element_by_xpath(root, job_content_xpath, ARRAY_SIZE(job_content_xpath));
    if(content == NULL) {
        log_error("get job content %s failed", url);
        return;
    }
    GumboNode *header = gumbo_get_element_by_xpath(content, job_header_xpath, ARRAY_SIZE(job_header_xpath));
    if(header == NULL) {
        log_error("get job header %s failed", url);
        return;
    }
    char title[1024];
    if(!gumbo_get_str_by_xpath(header, job_title_xpath, ARRAY_SIZE(job_title_xpath), title, sizeof(title))) {
        log_error("get title %s failed", url);
        return;
    }
    char desc[16 * 1024];
    if(!gumbo_get_str_by_xpath(content, job_desc_xpath, ARRAY_SIZE(job_desc_xpath), desc, sizeof(desc))) {
        log_error("get description %s failed", url);
        return;
    }
    job_t job = {
        .url = url,
        .title = title,
        .description = desc,
        .source = JOB_SOURCE_CVBANKAS,
        .for_ukraine = !!gumbo_get_child_by_class(header, "ukrainians_welcome_tag"),
        .for_disabled = !!gumbo_get_child_by_class(header, "special_needs_adapted_tag"),
    };
    GumboNode *salary_node = gumbo_get_element_by_xpath(header, job_salary_xpath, ARRAY_SIZE(job_salary_xpath));
    if(salary_node) {
        if(!salary_parse(app, salary_node, &job)) {
            return;
        }
    } else {
        log_debug("job %s has no salary info", url);
    }
    char city[512];
    if(gumbo_get_str_by_xpath(header, job_city_xpath, ARRAY_SIZE(job_city_xpath), city, sizeof(city))) {
        job.city = city;
    } else {
        log_debug("job %s has no city info", url);
    }
    char addr[1024];
    if(gumbo_get_str_by_xpath(root, job_addr_xpath, ARRAY_SIZE(job_addr_xpath), addr, sizeof(addr))) {
        job.addr = addr;
    } else {
        log_debug("job %s has no address info", url);
    }
    if(!db_job_add(app, &job)) {
        log_error("db add job %s failed", url);
    }
}
