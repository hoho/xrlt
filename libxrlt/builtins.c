#include "transform.h"
#include "builtins.h"


void *
xrltResponseCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    if (sheet->response == NULL) {
        sheet->response = node;
        return node;
    } else {
        xrltTransformError(NULL, sheet, node, "Duplicate response element\n");
        return NULL;
    }
}


void
xrltResponseFree(void *comp)
{
    (void)comp;
}


xrltBool
xrltResponseTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    if (ctx == NULL) { return FALSE; }

    xmlNodePtr           response;

    if (data == NULL) {
        TRANSFORM_SUBTREE(ctx, ((xmlNodePtr)comp)->children, ctx->response);

        // Schedule the next call.
        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltResponseTransform, comp, NULL, (void *)0x1
        );
    } else {
        // On the second call, check if something is ready to be sent and send
        // it if it is.
        xrltNodeDataPtr   n;
        xrltString        chunk;
        xrltBool          pushed;

        response = ctx->response;

        if (data == (void *)0x1) {
            // It's the second call.
            ctx->responseCur = response->children;
        }

        while (ctx->responseCur != NULL) {
            ASSERT_NODE_DATA(ctx->responseCur, n);

            if (n->count > 0) {
                break;
            }

            // Send response chunk out.
            chunk.data = (char *)xmlXPathCastNodeToString(ctx->responseCur);

            if (chunk.data != NULL) {
                chunk.len = strlen(chunk.data);

                pushed = chunk.len > 0
                    ?
                    xrltChunkListPush(&ctx->chunk, &chunk)
                    :
                    TRUE;

                xmlFree(chunk.data);

                if (!pushed) {
                    xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                                       "Failed to push response chunk\n");
                    return FALSE;
                }

                if (chunk.len > 0) {
                    ctx->cur |= XRLT_STATUS_CHUNK;
                }
            }

            ctx->responseCur = ctx->responseCur->next;
        }

        if (ctx->responseCur != NULL) {
            // We still have some data that's not ready, schedule the next call.
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltResponseTransform, comp,
                              response, (void *)0x2);
        }
    }

    return TRUE;
}


void *
xrltLogCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltLogData  *ret = NULL;
    xmlChar      *level = NULL;

    XRLT_MALLOC(ret, xrltLogData*, sizeof(xrltLogData), "xrltLogCompile", NULL);

    level = xmlGetProp(node, (const xmlChar *)"level");

    if (level == NULL) {
        ret->type = XRLT_INFO;
    } else {
        if (xmlStrEqual(level, (const xmlChar *)"warning")) {
            ret->type = XRLT_WARNING;
        } else if (xmlStrEqual(level, (const xmlChar *)"error")) {
            ret->type = XRLT_ERROR;
        } else if (xmlStrEqual(level, (const xmlChar *)"info")) {
            ret->type = XRLT_INFO;
        } else if (xmlStrEqual(level, (const xmlChar *)"debug")) {
            ret->type = XRLT_DEBUG;
        } else {
            xrltTransformError(NULL, sheet, node,
                               "Unknown 'level' attribute value\n");
            xmlFree(level);
            xrltLogFree(ret);
            return NULL;
        }

        xmlFree(level);
    }

    ret->node = node;

    return ret;
}


void
xrltLogFree(void *comp)
{
    if (comp != NULL) {
        xrltFree(comp);
    }
}


xrltBool
xrltLogTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert, void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr           log;
    xrltLogData         *logcomp = (xrltLogData *)comp;
    xrltNodeDataPtr      n;

    if (data == NULL) {
        // On the first call, create log parent node.

        if (logcomp->node->children == NULL) {
            // Just skip empty log element.
            return TRUE;
        }

        log = xmlNewChild(insert, NULL, (const xmlChar *)"log", NULL);

        if (log == NULL) {
            xrltTransformError(ctx, NULL, logcomp->node,
                               "Failed to create log element\n");
            return FALSE;
        }

        // Schedule log content transforms.
        TRANSFORM_SUBTREE(ctx, logcomp->node->children, log);

        COUNTER_INCREASE(ctx, log);

        // Schedule the next call.
        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltLogTransform, comp, insert, (void *)log
        );
    } else {
        log = (xmlNodePtr)data;

        ASSERT_NODE_DATA(log, n);

        if (n->count > 0) {
            COUNTER_DECREASE(ctx, log);

            if (n->count > 0) {
                // Node is not ready. Schedule the third call of this function
                // inside node's personal ready queue and return.
                SCHEDULE_CALLBACK(
                    ctx, &n->tcb, xrltLogTransform, comp, insert, (void *)log
                );

                return TRUE;
            }
        }

        // Node is ready, sent it's contents out.
        xrltString   msg;
        xrltBool     pushed;

        msg.data = (char *)xmlXPathCastNodeToString(log);

        REMOVE_RESPONSE_NODE(ctx, log);

        if (msg.data != NULL) {
            msg.len = strlen(msg.data);

            pushed = msg.len > 0
                ?
                xrltLogListPush(&ctx->log, logcomp->type, &msg)
                :
                TRUE;

            xmlFree(msg.data);

            if (!pushed) {
                xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                                   "Failed to push log message\n");
                return FALSE;
            }

            if (msg.len > 0) {
                ctx->cur |= XRLT_STATUS_LOG;
            }
        }
    }

    return TRUE;
}


void *
xrltIfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltIfData  *ret = NULL;
    xmlChar     *expr = NULL;

    XRLT_MALLOC(ret, xrltIfData*, sizeof(xrltIfData), "xrltIfCompile", NULL);

    expr = xmlGetProp(node, XRLT_ELEMENT_ATTR_TEST);

    if (expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to get 'test' attribute\n");
        goto error;
    }

    ret->test = xmlXPathCompile(expr);

    if (ret->test == NULL) {
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
        xmlXPathFreeCompExpr(((xrltIfData *)comp)->test);
        xrltFree(comp);
    }
}


xrltBool
xrltIfTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert, void *data)
{
    return TRUE;
}


void *
xrltValueOfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltValueOfData  *ret = NULL;
    xmlChar          *select = NULL;

    if (node->children != NULL) {
        xrltTransformError(NULL, sheet, node, "Element can't have content\n");
        return NULL;
    }

    XRLT_MALLOC(ret, xrltValueOfData*, sizeof(xrltValueOfData),
                "xrltValueOfCompile", NULL);

    select = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);

    if (select == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to get 'select' attribute\n");
        goto error;
    }

    ret->select = xmlXPathCompile(select);

    if (ret->select == NULL) {
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
        xmlXPathFreeCompExpr(((xrltValueOfData *)comp)->select);
        xrltFree(comp);
    }
}


static void
xrltValueOfTransformingFree(void *data)
{
    xrltValueOfTransformingData  *tdata;

    if (data != NULL) {
        tdata = (xrltValueOfTransformingData *)data;

        if (tdata->val != NULL) { xmlFree(tdata->val); }

        xrltFree(data);
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

        XRLT_MALLOC(tdata, xrltValueOfTransformingData*,
                    sizeof(xrltValueOfTransformingData),
                    "xrltValueOfTransform", FALSE);

        n->data = tdata;
        n->free = xrltValueOfTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "tmp");

        tdata->dataNode = node;

        TRANSFORM_TO_STRING(ctx, node, NULL, NULL, vcomp->select, &tdata->val);

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
            xrltTransformError(ctx, NULL, vcomp->node,
                               "Failed to create response node\n");
            return FALSE;
        }

        if (xmlAddNextSibling(tdata->node, node) == NULL) {
            xrltTransformError(ctx, NULL, vcomp->node,
                               "Failed to append response node\n");
            xmlFreeNode(node);
            return FALSE;
        }

        COUNTER_DECREASE(ctx, tdata->node);

        REMOVE_RESPONSE_NODE(ctx, tdata->node);
    }

    return TRUE;
}
