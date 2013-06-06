/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "copyof.h"


void *
xrltCopyOfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltCopyOfData  *ret = NULL;
    xmlChar         *select = NULL;

    if (node->children != NULL) {
        xrltTransformError(NULL, sheet, node, "Element can't have content\n");
        return NULL;
    }

    XRLT_MALLOC(NULL, sheet, node, ret, xrltCopyOfData *,
                sizeof(xrltCopyOfData), NULL);

    select = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);

    if (select == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to get 'select' attribute\n");
        goto error;
    }

    ret->select.type = XRLT_VALUE_XPATH;
    ret->select.xpathval.src = node;
    ret->select.xpathval.scope = node->parent;
    ret->select.xpathval.expr = xmlXPathCompile(select);

    if (ret->select.xpathval.expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to compile expression\n");
        goto error;
    }

    xmlFree(select);

    ret->node = node;

    return ret;

  error:
    if (select) { xmlFree(select); }
    xrltCopyOfFree(ret);

    return NULL;
}


void
xrltCopyOfFree(void *comp)
{
    if (comp != NULL) {
        CLEAR_XRLT_VALUE(((xrltCopyOfData *)comp)->select);
        xmlFree(comp);
    }
}


static void
xrltCopyOfTransformingFree(void *data)
{
    if (data != NULL) {
        xmlFree(data);
    }
}


DEFINE_TRANSFORM_FUNCTION(
    xrltCopyOfTransform,
    xrltCopyOfData*,
    xrltCopyOfTransformingData*,
    sizeof(xrltCopyOfTransformingData),
    xrltCopyOfTransformingFree,
    ;,
    {
        NEW_CHILD(ctx, tdata->retNode, tdata->node, "r");

        TRANSFORM_XPATH_TO_NODE(ctx, &tcomp->select.xpathval, tdata->retNode);
    },
    {
        WAIT_FOR_NODE(ctx, tdata->retNode, xrltCopyOfTransform);
    }
);
