#include "transform.h"
#include "builtins.h"
#include "xrlterror.h"


void *
xrltResponseCompile(xrltRequestsheetPtr sheet, xmlNodePtr node)
{
    xrltRequestsheetPrivate  *priv = (xrltRequestsheetPrivate *)sheet->_private;

    if (priv->response == NULL) {
        priv->response = node;

        if (node->children != NULL &&
            !xrltElementCompile(sheet, node->children))
        {
            return NULL;
        }

        return node;
    } else {
        xrltTransformError(NULL, sheet, node,
                           "Duplicate response node\n");
        return NULL;
    }
}


void
xrltResponseFree(void *data)
{
    (void)data;
}

xrltBool
xrltResponseTransform(xrltContextPtr ctx, void *data)
{
    return TRUE;
}



void *
xrltIfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node)
{
    xrltIfData  *ret = NULL;
    xmlChar     *expr = NULL;

    expr = xmlGetProp(node, XRLT_ELEMENT_ATTR_TEST);

    if (expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to get 'test' attribute\n");
        goto error;
    }

    ret = (xrltIfData *)xrltMalloc(sizeof(xrltIfData));

    if (ret == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "xrltIfCompile: Out of memory\n");
        goto error;
    }

    memset(ret, 0, sizeof(xrltIfData));

    ret->test = xmlXPathCompile(expr);

    if (ret->test == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to compile expression\n");
        goto error;
    }

    if (node->children != NULL &&
        !xrltElementCompile(sheet, node->children))
    {
        goto error;
    }

    xmlFree(expr);

    ret->children = node->children;

    return ret;

  error:
    if (expr) { xmlFree(expr); }
    xrltIfFree(ret);

    return NULL;
}


void
xrltIfFree(void *data)
{
    if (data != NULL) {
        xmlXPathFreeCompExpr(((xrltIfData *)data)->test);
        xrltFree(data);
    }
}


xrltBool
xrltIfTransform(xrltContextPtr ctx, void *data)
{
    return TRUE;
}
