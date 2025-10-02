#include <global.h>

static void *parser_alloc(void *userdata, size_t size)
{
    app_t *app = userdata;
    return mem_alloc(app, __func__, size);
}

static void parser_free(void *userdata, void *ptr)
{
    UNUSED(userdata);
    UNUSED(ptr);
}

void html_parser_init(app_t *app)
{
    html_parser_t *parser = &app->html_parser;
    parser->options = kGumboDefaultOptions;
    parser->options.allocator = parser_alloc;
    parser->options.deallocator = parser_free;
    parser->options.userdata = app;
    parser->options.fragment_context = GUMBO_TAG_BODY;
    parser->options.fragment_namespace = GUMBO_NAMESPACE_HTML;
}

GumboOutput *gumbo_parse_wrap(app_t *app, const char *html, uint32_t html_size)
{
    html_parser_t *parser = &app->html_parser;
    parser->mem_offset = mem_get_offset(app);
    return gumbo_parse_with_options(&parser->options, html, html_size);
}

void gumbo_destroy_wrap(app_t *app, GumboOutput *output)
{
    html_parser_t *parser = &app->html_parser;
    gumbo_destroy_output(&parser->options, output);
    mem_put_offset(app, parser->mem_offset);
}

static uint32_t gumbo_get_inner_text(GumboNode *node, char *buf, uint32_t buf_size)
{
    buf[0] = 0;
    if(node == NULL) {
        return 0;
    }
    if(node->type == GUMBO_NODE_TEXT) {
        strncpy(buf, node->v.text.text, buf_size - 1);
        return strlen(node->v.text.text);
    } else if(node->type == GUMBO_NODE_ELEMENT) {
        uint32_t len = 0;
        GumboVector *children = &node->v.element.children;
        for(uint32_t i = 0; i < children->length; i++) {
            len += gumbo_get_inner_text(children->data[i], buf + len, buf_size - len);
        }
        return len;
    }
    return 0;
}

static GumboNode *gumbo_get_nth_child_with_tag(GumboNode *parent, GumboTag tag, int index)
{
    if(parent->type != GUMBO_NODE_ELEMENT) {
        return NULL;
    }
    int count = 1;
    GumboVector *children = &parent->v.element.children;
    for(uint32_t i = 0; i < children->length; i++) {
        GumboNode *child = children->data[i];
        if(child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == tag) {
            if(count == index) {
                return child;
            }
            count++;
        }
    }
    return NULL;
}

static bool gumbo_has_class(GumboNode *node, const char *class_name)
{
    if(node->type != GUMBO_NODE_ELEMENT) {
        return false;
    }
    GumboAttribute *class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
    if(class_attr == NULL || class_attr->value == NULL) {
        return false;
    }
    return strstr(class_attr->value, class_name) ? true : false;
}

static bool gumbo_has_id(GumboNode *node, const char *id_name)
{
    if(node->type != GUMBO_NODE_ELEMENT) {
        return false;
    }
    const GumboAttribute *id_attr = gumbo_get_attribute(&node->v.element.attributes, "id");
    if(id_attr == NULL || id_attr->value == NULL) {
        return false;
    }
    return strcmp(id_attr->value, id_name) == 0;
}

static GumboNode *gumbo_get_child_by_id(GumboNode *node, const char *id_name)
{
    if(node->type != GUMBO_NODE_ELEMENT) {
        return NULL;
    }
    GumboVector *children = &node->v.element.children;
    for(uint32_t i = 0; i < children->length; i++) {
        GumboNode *child = children->data[i];
        if(gumbo_has_id(child, id_name)) {
            return child;
        }
    }
    return NULL;
}

GumboNode *gumbo_get_element_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len)
{
    for(uint32_t i = 0; i < xpath_len; i++) {
        const gumbo_xpath_t *xp = &xpath[i];
        if(xp->id_name) {
            node = gumbo_get_child_by_id(node, xp->id_name);
        } else {
            node = gumbo_get_nth_child_with_tag(node, xp->tag, xp->index);
        }
        if(node == NULL) {
            return NULL;
        }
    }
    return node;
}

GumboNode *gumbo_get_child_by_class(GumboNode *node, const char *class_name)
{
    if(node->type != GUMBO_NODE_ELEMENT) {
        return NULL;
    }
    GumboVector *children = &node->v.element.children;
    for(uint32_t i = 0; i < children->length; i++) {
        GumboNode *child = children->data[i];
        if(gumbo_has_class(child, class_name)) {
            return child;
        }
    }
    return NULL;
}

bool gumbo_get_str_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len, char *buf,
                            uint32_t buf_size)
{
    node = gumbo_get_element_by_xpath(node, xpath, xpath_len);
    if(node == NULL) {
        return false;
    }
    gumbo_get_inner_text(node, buf, buf_size);
    return true;
}

int gumbo_get_int_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len)
{
    char buf[32];
    if(!gumbo_get_str_by_xpath(node, xpath, xpath_len, buf, sizeof(buf))) {
        return 0;
    }
    str_remove_char(buf, ' ');
    return atoi(buf);
}

const char *gumbo_get_attr_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len,
                                    const char *attr_name)
{
    node = gumbo_get_element_by_xpath(node, xpath, xpath_len);
    if(node == NULL || node->type != GUMBO_NODE_ELEMENT) {
        return NULL;
    }
    const GumboAttribute *attr = gumbo_get_attribute(&node->v.element.attributes, attr_name);
    if(attr == NULL) {
        return NULL;
    }
    return attr->value;
}

bool gumbo_has_class_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len, const char *class_name)
{
    node = gumbo_get_element_by_xpath(node, xpath, xpath_len);
    if(node == NULL) {
        return false;
    }
    return gumbo_has_class(node, class_name);
}
