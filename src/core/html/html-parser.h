#pragma once

#include <common.h>
#include <gumbo.h>

#define XP_TAG(tag, index) { NULL, tag, index }
#define XP_ID(id)          { id, 0, 0 }

/**
 * @brief Parser structure with a temporary buffer to reduce memory allocation overhead
 * @warning Don't parse HTML concurrently with the same parser instance
 */
typedef struct {
    GumboOptions options; //!< Gumbo parser options
    uint32_t mem_offset;  ///< Current memory offset in the temporary buffer
} html_parser_t;

/**
 * @brief Structure to represent an XPath-like query for Gumbo nodes
 */
typedef struct {
    const char *id_name; ///< If set, match the id name instead of index
    GumboTag tag;        ///< Gumbo tag to match
    uint32_t index;      ///< Index of the tag (start from 1)
} gumbo_xpath_t;

/**
 * @brief Initialize the HTML parser structure
 * @param app - [in] Pointer to the application instance
 */
void html_parser_init(app_t *app);

/**
 * @brief Parse HTML with a temporary buffer to reduce memory allocation overhead
 * @param app - [in] Pointer to the application instance
 * @param html - [in] HTML string to parse
 * @param html_size - [in] Size of the HTML string
 * @return Pointer to GumboOutput structure containing the HTML tree or NULL on failure
 */
GumboOutput *gumbo_parse_wrap(app_t *app, const char *html, uint32_t html_size);

/**
 * @brief Destroy GumboOutput structure and free associated memory
 * @param app - [in] Pointer to the application instance
 * @param output - [in] Pointer to GumboOutput structure to destroy
 */
void gumbo_destroy_wrap(app_t *app, GumboOutput *output);

/**
 * @brief Get the first GumboNode that matches the given XPath-like query
 * @param node - [in] Pointer to the root GumboNode to start the search
 * @param xpath - [in] Array of gumbo_xpath_t representing the XPath-like query
 * @param xpath_len - [in] Length of the xpath array
 * @return Pointer to the matching GumboNode or NULL if not found
 */
GumboNode *gumbo_get_element_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len);

/**
 * @brief Get child node by ID
 * @param node - [in] Pointer to the GumboNode
 * @param id_name - [in] ID name to search for
 * @return Pointer to the matching GumboNode or NULL if not found
 */
// GumboNode *gumbo_get_child_by_id(GumboNode *node, const char *id_name);

/**
 * @brief Get child node by class name
 * @param node - [in] Pointer to the GumboNode
 * @param class_name - [in] Class name to search for
 * @return Pointer to the matching GumboNode or NULL if not found
 */
GumboNode *gumbo_get_child_by_class(GumboNode *node, const char *class_name);

/**
 * @brief Get inner text from a GumboNode specified by an XPath-like query
 * @param node - [in] Pointer to the root GumboNode to start the search
 * @param xpath - [in] Array of gumbo_xpath_t representing the XPath-like query
 * @param xpath_len - [in] Length of the xpath array
 * @param buf - [out] Buffer to store the extracted text
 * @param buf_size - [in] Size of the buffer
 * @return true on success, false on failure
 */
bool gumbo_get_str_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len, char *buf,
                            uint32_t buf_size);

/**
 * @brief Get integer value from a GumboNode specified by an XPath-like query
 * @param node - [in] Pointer to the root GumboNode to start the search
 * @param xpath - [in] Array of gumbo_xpath_t representing the XPath-like query
 * @param xpath_len - [in] Length of the xpath array
 * @return Integer value extracted from the node, or 0 if not found or conversion fails
 */
int gumbo_get_int_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len);

/**
 * @brief Get the value of a specific attribute from a GumboNode specified by an XPath-like query
 * @param node - [in] Pointer to the root GumboNode to start the search
 * @param xpath - [in] Array of gumbo_xpath_t representing the XPath-like query
 * @param xpath_len - [in] Length of the xpath array
 * @param attr_name - [in] Name of the attribute to retrieve
 * @return Pointer to the attribute value string, or NULL if not found
 */
const char *gumbo_get_attr_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len,
                                    const char *attr_name);

/**
 * @brief Check if a GumboNode specified by an XPath-like query has a specific class name
 * @param node - [in] Pointer to the root GumboNode to start the search
 * @param xpath - [in] Array of gumbo_xpath_t representing the XPath-like query
 * @param xpath_len - [in] Length of the xpath array
 * @param class_name - [in] Class name to check for
 * @return true if the node has the specified class name, false otherwise
 */
bool gumbo_has_class_by_xpath(GumboNode *node, const gumbo_xpath_t *xpath, uint32_t xpath_len, const char *class_name);
