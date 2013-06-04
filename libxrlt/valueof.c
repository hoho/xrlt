/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "valueof.h"


void *
xrltValueOfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltValueOfData  *ret = NULL;
    xmlChar          *select = NULL;

    if (node->children != NULL) {
        xrltTransformError(NULL, sheet, node, "Element can't have content\n");
        return NULL;
    }

    XRLT_MALLOC(NULL, sheet, node, ret, xrltValueOfData*,
                sizeof(xrltValueOfData), NULL);

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
    xrltValueOfFree(ret);

    return NULL;
}


void
xrltValueOfFree(void *comp)
{
    if (comp != NULL) {
        CLEAR_XRLT_VALUE(((xrltValueOfData *)comp)->select);
        xmlFree(comp);
    }
}


static void
xrltValueOfTransformingFree(void *data)
{
    xrltValueOfTransformingData  *tdata;

    if (data != NULL) {
        tdata = (xrltValueOfTransformingData *)data;

        if (tdata->val != NULL) { xmlFree(tdata->val); }

        xmlFree(data);
    }
}


xrltBool
xrltValueOfTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                     void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr                    node;
    xrltValueOfData              *vcomp = (xrltValueOfData *)comp;
    xrltNodeDataPtr               n;
    xrltValueOfTransformingData  *tdata;

    if (data == NULL) {
        NEW_CHILD(ctx, node, insert, "v-o");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, vcomp->node, tdata, xrltValueOfTransformingData*,
                    sizeof(xrltValueOfTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltValueOfTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "tmp");

        tdata->dataNode = node;

        TRANSFORM_TO_STRING(ctx, node, &vcomp->select, &tdata->val);

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltValueOfTransform, comp, insert,
                          tdata);
    } else {
        tdata = (xrltValueOfTransformingData *)data;

        ASSERT_NODE_DATA(tdata->dataNode, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltValueOfTransform, comp, insert,
                              data);

            return TRUE;
        }

        node = xmlNewText(tdata->val);

        if (node == NULL) {
            ERROR_CREATE_NODE(ctx, NULL, vcomp->node);
            return FALSE;
        }

        if (xmlAddNextSibling(tdata->node, node) == NULL) {
            ERROR_ADD_NODE(ctx, NULL, vcomp->node);
            xmlFreeNode(node);
            return FALSE;
        }

        COUNTER_DECREASE(ctx, tdata->node);

        REMOVE_RESPONSE_NODE(ctx, tdata->node);
    }

    return TRUE;
}
