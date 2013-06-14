/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "foreach.h"


void *
xrltForEachCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltForEachData  *ret = NULL;
    xmlChar          *select = NULL;

    if (node->children == NULL) {
        xrltTransformError(NULL, sheet, node, "Element is empty\n");
        return NULL;
    }

    XRLT_MALLOC(NULL, sheet, node, ret, xrltForEachData *,
                sizeof(xrltForEachData), NULL);

    select = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);

    if (select == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to get 'select' attribute\n");
        goto error;
    }

    ret->select.type = XRLT_VALUE_XPATH;
    ret->select.xpathval.src = node;
    ret->select.xpathval.scope = node;
    ret->select.xpathval.expr = xmlXPathCompile(select);

    if (ret->select.xpathval.expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to compile expression\n");
        goto error;
    }

    xmlFree(select);

    ret->node = node;
    ret->children = node->children;

    return ret;

  error:
    if (select) { xmlFree(select); }
    xrltForEachFree(ret);

    return NULL;
}


void
xrltForEachFree(void *comp)
{
    if (comp != NULL) {
        CLEAR_XRLT_VALUE(((xrltForEachData *)comp)->select);
        xmlFree(comp);
    }
}


static void
xrltForEachTransformingFree(void *data)
{
    if (data != NULL) {
        xrltForEachTransformingData *tdata = \
            (xrltForEachTransformingData *)data;

        if (tdata->val != NULL) {
            xmlXPathFreeNodeSet(tdata->val);
        }

        xmlFree(data);
    }
}


#define MOVE_NODESET_TO_TDATA                                                 \
    if (val->type != XPATH_NODESET) {                                         \
        xrltTransformError(ctx, NULL, tcomp->node,                            \
                "The 'select' expression does not evaluate to a node set\n"); \
        xmlXPathFreeObject(val);                                              \
        return FALSE;                                                         \
    }                                                                         \
    tdata->val = val->nodesetval;                                             \
    val->nodesetval = NULL;                                                   \
    xmlXPathFreeObject(val);                                                  \


DEFINE_TRANSFORM_FUNCTION(
    xrltForEachTransform,
    xrltForEachData*,
    xrltForEachTransformingData*,
    sizeof(xrltForEachTransformingData),
    xrltForEachTransformingFree,
    xmlXPathObjectPtr   val;,
    {
        ASSERT_NODE_DATA(insert, nodeData);

        nodeData->parentScope = ctx->varScope;

        if (!xrltXPathEval(ctx, insert, &tcomp->select.xpathval, &val)) {
            return FALSE;
        }

        if (val == NULL) {
            tdata->xpathEvaluation = TRUE;

            WAIT_FOR_NODE(ctx, ctx->xpathWait, xrltForEachTransform);
        } else {
            MOVE_NODESET_TO_TDATA;

            return xrltForEachTransform(ctx, comp, insert, tdata);
        }
    },
    {
        if (tdata->xpathEvaluation) {
            if (!xrltXPathEval(ctx, insert, &tcomp->select.xpathval, &val)) {
                return FALSE;
            }

            if (val == NULL) {
                WAIT_FOR_NODE(ctx, ctx->xpathWait, xrltForEachTransform);
            } else {
                MOVE_NODESET_TO_TDATA;

                tdata->xpathEvaluation = FALSE;

                return xrltForEachTransform(ctx, comp, insert, tdata);
            }
        } else {
            if (tdata->val != NULL && tdata->val->nodeNr > 0) {
                if (tdata->retNode == NULL) {
                    int   i;
                    int   oldContextSize;
                    int   oldProximityPosition;

                    NEW_CHILD(ctx, tdata->retNode, tdata->node, "r");

                    tmpNode = ctx->xpathContext;
                    oldContextSize = ctx->xpathContextSize;
                    oldProximityPosition = ctx->xpathProximityPosition;

                    ctx->xpathContextSize = tdata->val->nodeNr;

                    for (i = 0; i < tdata->val->nodeNr; i++) {
                        ctx->varScope = ++ctx->maxVarScope;

                        ctx->xpathContext = tdata->val->nodeTab[i];
                        ctx->xpathProximityPosition = i + 1;

                        TRANSFORM_SUBTREE(ctx, tcomp->children, tdata->retNode);
                    }

                    ctx->xpathContext = tmpNode;
                    ctx->xpathContextSize = oldContextSize;
                    ctx->xpathProximityPosition = oldProximityPosition;

                    SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltForEachTransform,
                                      comp, insert, data);
                    return TRUE;
                } else {
                    WAIT_FOR_NODE(ctx, tdata->retNode, xrltForEachTransform);
                }
            }
        }
    }
);
