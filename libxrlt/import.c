/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "import.h"
#include "transform.h"
#include <libxml/uri.h>


static inline xrltBool
xrltProcessImport(xrltRequestsheetPtr sheet, xmlNodePtr node, int level)
{
    xmlChar    *base = NULL;
    xmlChar    *href = NULL;
    xmlChar    *URI = NULL;
    xmlDocPtr   doc = NULL;
    xmlNodePtr  n1, n2, n3;
    xrltBool    ret = FALSE;

    href = xmlGetNsProp(node, (const xmlChar *)"href", NULL);
    if (href == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "%s: No 'href' attribute\n", __func__);
        goto error;
    }

    base = xmlNodeGetBase(node->doc, node);
    URI = xmlBuildURI(href, base);

    if (URI == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Invalid URI reference %s\n", href);
        goto error;
    }

    doc = xmlParseFile((char *)URI);
    if (doc == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to import '%s' requestsheet\n", URI);
        goto error;
    }

    n1 = xmlDocGetRootElement(doc);
    if (n1 == NULL || n1->ns == NULL || !xmlStrEqual(n1->ns->href, XRLT_NS) ||
        !xmlStrEqual(n1->name, XRLT_ROOT_NAME))
    {
        xrltTransformError(NULL, sheet, node,
                           "Bad requestsheet ('%s')\n", URI);
        goto error;
    }

    if (!xrltProcessImports(sheet, n1, level + 1)) {
        goto error;
    }

    n1 = n1->children;
    n3 = node;

    while (n1 != NULL) {
        n2 = xmlDocCopyNode(n1, node->doc, 1);
        if (n2 == NULL) {
            ERROR_CREATE_NODE(NULL, sheet, n1);
            goto error;
        }

        xmlReconciliateNs(node->doc, n2);

        n3 = xmlAddNextSibling(n3, n2);
        if (n3 == NULL) {
            ERROR_ADD_NODE(NULL, sheet, n1);
            xmlFreeNode(n2);
            goto error;
        }

        n1 = n1->next;
    }

    ret = TRUE;

  error:
    if (base != NULL) { xmlFree(base); }
    if (href != NULL) { xmlFree(href); }
    if (URI != NULL) { xmlFree(URI); }
    if (doc != NULL) { xmlFreeDoc(doc); }

    return ret;
}


xrltBool
xrltProcessImports(xrltRequestsheetPtr sheet, xmlNodePtr node, int level)
{
    xmlNodePtr   tmp;

    if (level > XRLT_MAX_IMPORT_DEEPNESS) {
        xrltTransformError(NULL, sheet, node, "Too deep import\n");
        return FALSE;
    }

    node = node->children;

    while (node != NULL) {
        if (node->ns != NULL && xmlStrEqual(node->ns->href, XRLT_NS) &&
            xmlStrEqual(node->name, (const xmlChar *)"import"))
        {
            tmp = node->next;

            if (!xrltProcessImport(sheet, node, level)) {
                return FALSE;
            }

            xmlUnlinkNode(node);
            xmlFreeNode(node);

            node = tmp;
        } else {
            node = node->next;
        }
    }

    return TRUE;
}
