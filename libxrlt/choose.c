/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "choose.h"


void *
xrltChooseCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltChooseData      *ret = NULL;
    xmlChar             *expr = NULL;
    int                  i;
    xmlNodePtr           tmp;
    xrltBool             otherwise = FALSE;
    xrltNodeDataPtr      n;

    if (node->children == NULL) {
        xrltTransformError(NULL, sheet, node, "Element is empty\n");
    }

    i = 0;
    tmp = node->children;

    while (tmp != NULL) {
        i++;
        tmp = tmp->next;
    }

    XRLT_MALLOC(NULL, sheet, node, ret, xrltChooseData*,
                sizeof(xrltChooseData) + sizeof(xrltChooseCaseData) * i, NULL);

    ret->node = node;

    ret->cases = (xrltChooseCaseData *)(ret + 1);
    ret->len = i;

    i = 0;
    tmp = node->children;

    while (tmp != NULL) {
        if (otherwise || tmp->ns == NULL ||
            !xmlStrEqual(tmp->ns->href, XRLT_NS))
        {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
            goto error;
        }

        if (xmlStrEqual(tmp->name, (const xmlChar *)"otherwise")) {
            if (node->children == tmp) {
                ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
                goto error;
            }

            otherwise = TRUE;
        } else if (xmlStrEqual(tmp->name, (const xmlChar *)"when")) {
            expr = xmlGetProp(tmp, XRLT_ELEMENT_ATTR_TEST);

            if (expr == NULL) {
                xrltTransformError(NULL, sheet, tmp, "No 'test' attribute\n");
                goto error;
            }

            ret->cases[i].test.xpathval.expr = xmlXPathCompile(expr);

            xmlFree(expr);

            ret->cases[i].test.type = XRLT_VALUE_XPATH;

            if (ret->cases[i].test.xpathval.expr == NULL) {
                xrltTransformError(NULL, sheet, tmp,
                                   "Failed to compile expression\n");
                goto error;
            }

            ret->cases[i].test.xpathval.src = tmp;
            ret->cases[i].test.xpathval.scope = node->parent;
        } else {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
            goto error;
        }

        ret->cases[i].children = tmp->children;

        ASSERT_NODE_DATA_GOTO(tmp, n);

        n->xrlt = TRUE;

        i++;
        tmp = tmp->next;
    }

    return ret;

  error:
    xrltChooseFree(ret);

    return NULL;
}


void
xrltChooseFree(void *comp)
{
    if (comp != NULL) {
        xrltChooseData  *d;
        int              i;

        d = (xrltChooseData *)comp;

        for (i = 0; i < d->len; i++)  {
            if (d->cases[i].test.xpathval.expr != NULL) {
                xmlXPathFreeCompExpr(d->cases[i].test.xpathval.expr);
            }
        }

        xmlFree(d);
    }

}


static void
xrltChooseTransformingFree(void *data)
{
    if (data != NULL) {
        xmlFree(data);
    }
}


xrltBool
xrltChooseTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                    void *data)
{
    xrltChooseData              *ccomp = (xrltChooseData *)comp;
    xrltChooseTransformingData  *tdata;
    xmlNodePtr                   node;
    xrltNodeDataPtr              n;

    if (data == NULL) {
        if (ccomp->len == 0) {
            return TRUE;
        }

        NEW_CHILD(ctx, node, insert, "c");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, ccomp->node, tdata, xrltChooseTransformingData*,
                    sizeof(xrltChooseTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltChooseTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "tmp");

        tdata->testNode = node;

        TRANSFORM_TO_BOOLEAN(ctx, node, &ccomp->cases[0].test, &tdata->testRet);

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltChooseTransform, comp, insert,
                          tdata);
    } else {
        tdata = (xrltChooseTransformingData *)data;

        node = tdata->retNode == NULL ? tdata->testNode : tdata->retNode;

        ASSERT_NODE_DATA(node, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltChooseTransform, comp, insert,
                              tdata);
            return TRUE;
        }

        if (tdata->retNode == NULL) {
            if (tdata->testRet) {
                NEW_CHILD(ctx, tdata->retNode, tdata->node, "r");

                TRANSFORM_SUBTREE(
                    ctx, ccomp->cases[tdata->pos].children, tdata->retNode
                );

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltChooseTransform, comp, insert, tdata
                );

                return TRUE;
            } else {
                tdata->pos++;

                if (tdata->pos < ccomp->len) {
                    if (ccomp->cases[tdata->pos].test.type == XRLT_VALUE_EMPTY)
                    {
                        tdata->testRet = TRUE;

                        return xrltChooseTransform(ctx, comp, insert, data);
                    } else {
                        TRANSFORM_TO_BOOLEAN(
                            ctx, node, &ccomp->cases[tdata->pos].test,
                            &tdata->testRet
                        );

                        SCHEDULE_CALLBACK(
                            ctx, &ctx->tcb, xrltChooseTransform, comp, insert,
                            tdata
                        );

                        return TRUE;
                    }
                } else {
                    COUNTER_DECREASE(ctx, tdata->node);

                    REMOVE_RESPONSE_NODE(ctx, tdata->node);
                }
            }
        } else {
            COUNTER_DECREASE(ctx, tdata->node);

            REPLACE_RESPONSE_NODE(
                ctx, tdata->node, tdata->retNode->children, ccomp->node
            );
        }
    }

    return TRUE;
}
