#include <core/parser.h>

typedef struct {
    char *buf;
    char *prev_ptr;
    uint32_t size;
    uint32_t capacity;
} alloc_data_t;

static void *gumbo_alloc(void *userdata, size_t size)
{
    alloc_data_t *data = userdata;
    if(data->size + size > data->capacity) {
        return NULL;
    }
    data->prev_ptr = data->buf + data->size;
    data->size += size;
    return data->prev_ptr;
}

static void gumbo_free(void *userdata, void *ptr)
{
    alloc_data_t *data = userdata;
    if(ptr == data->prev_ptr) {
        data->size = data->prev_ptr - data->buf;
        data->prev_ptr = data->buf + data->size;
    }
}

static const GumboOptions options_tmpl = {
    .allocator = gumbo_alloc,
    .deallocator = gumbo_free,
    .tab_stop = 8,
    .max_errors = -1,
    .fragment_context = GUMBO_TAG_BODY,
    .fragment_namespace = GUMBO_NAMESPACE_HTML,
};

GumboOutput *gumbo_parse_wrap(const char *html, uint32_t html_size, void *tmp_buf, uint32_t tmp_buf_size)
{
    alloc_data_t *data = tmp_buf;
    data->buf = tmp_buf;
    data->prev_ptr = NULL;
    data->size = sizeof(alloc_data_t);
    data->capacity = tmp_buf_size;
    GumboOptions options = options_tmpl;
    options.userdata = data;
    return gumbo_parse_with_options(&options, html, html_size);
}

void gumbo_destroy_wrap(GumboOutput *output, void *tmp_buf)
{
    GumboOptions options = options_tmpl;
    options.userdata = tmp_buf;
    gumbo_destroy_output(&options, output);
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
        for(uint32_t i = 0; i < children->length; ++i) {
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

GumboNode *gumbo_get_element_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len)
{
    for(uint32_t i = 0; i < xpath_len; i++) {
        node = gumbo_get_nth_child_with_tag(node, xpath[i].tag, xpath[i].index);
        if(node == NULL) {
            return NULL;
        }
    }
    return node;
}

int gumbo_get_int_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len)
{
    char buf[32];
    node = gumbo_get_element_by_xpath(node, xpath, xpath_len);
    if(node == NULL) {
        return 0;
    }
    gumbo_get_inner_text(node, buf, sizeof(buf));
    return atoi(buf);
}

const char *gumbo_get_attr_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len,
                                    const char *attr_name)
{
    node = gumbo_get_element_by_xpath(node, xpath, xpath_len);
    if(node == NULL || node->type != GUMBO_NODE_ELEMENT) {
        return NULL;
    }
    GumboAttribute *attr = gumbo_get_attribute(&node->v.element.attributes, attr_name);
    if(attr == NULL) {
        return NULL;
    }
    return attr->value;
}

static bool gumbo_has_class(GumboNode *node, const char *class_name)
{
    if(node->type != GUMBO_NODE_ELEMENT) {
        return false;
    }
    GumboAttribute *class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
    if(class_attr == NULL) {
        return false;
    }
    return strstr(class_attr->value, class_name) ? true : false;
}

bool gumbo_has_class_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len, const char *class_name)
{
    node = gumbo_get_element_by_xpath(node, xpath, xpath_len);
    if(node == NULL) {
        return false;
    }
    return gumbo_has_class(node, class_name);
}
