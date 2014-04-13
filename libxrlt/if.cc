/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "if.h"


void *
xrltIfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltIfData  *ret = NULL;
    xmlChar     *expr = NULL;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltIfData*, sizeof(xrltIfData), NULL);

    expr = xmlGetProp(node, XRLT_ELEMENT_ATTR_TEST);

    if (expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "No 'test' attribute\n");
        goto error;
    }

    ret->node = node;

    ret->test.xpathval.expr = xmlXPathCompile(expr);
    ret->test.type = XRLT_VALUE_XPATH;

    ret->test.xpathval.src = node;
    ret->test.xpathval.scope = node->parent;

    if (ret->test.xpathval.expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to compile expression\n");
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
xrltIfFree(void *comp)
{
    if (comp != NULL) {
        xmlXPathFreeCompExpr(((xrltIfData *)comp)->test.xpathval.expr);
        xmlFree(comp);
    }
}


static void xrltIfTransformingFree(void *data)
{
    if (data != NULL) {
        xmlFree(data);
    }
}


DEFINE_TRANSFORM_FUNCTION(
    xrltIfTransform,
    xrltIfData*,
    xrltIfTransformingData*,
    sizeof(xrltIfTransformingData),
    xrltIfTransformingFree,
    ;,
    {
        NEW_CHILD(ctx, tdata->testNode, tdata->node, "t");

        TRANSFORM_TO_BOOLEAN(ctx, tdata->testNode, &tcomp->test, &tdata->test);
    },
    {
        if (tdata->testNode != NULL) {
            WAIT_FOR_NODE(ctx, tdata->testNode, xrltIfTransform);

            tdata->testNode = NULL;

            if (tdata->test) {
                NEW_CHILD(ctx, tdata->retNode, tdata->node, "r");

                TRANSFORM_SUBTREE(ctx, tcomp->children, tdata->retNode);

                CALL_AGAIN(ctx, xrltIfTransform);
            }
        }

        if (tdata->retNode != NULL) {
            WAIT_FOR_NODE(ctx, tdata->retNode, xrltIfTransform);
        }
    }
);
