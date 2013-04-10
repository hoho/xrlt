#include "transform.h"
#include "builtins.h"


void *
xrltResponseCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    if (sheet->response == NULL) {
        sheet->response = node;

        return node;
    } else {
        xrltTransformError(NULL, sheet, node,
                           "Duplicate response element\n");
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
    if (ctx == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr           response;

    if (data == NULL) {
        // On the first call, create response parent node and schedule next
        // call.
        response = xmlNewDocNode(ctx->responseDoc, NULL,
                                 (const xmlChar *)"response", NULL);

        if (response == NULL) {
            xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                               "Failed to create response element\n");
            return FALSE;
        }

        xmlDocSetRootElement(ctx->responseDoc, response);

        ctx->response = response;

        if (!xrltElementTransform(ctx, ((xmlNodePtr)comp)->children, response))
        {
            return FALSE;
        }

        // Schedule the next call.
        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltResponseTransform, comp,
                          insert, (void *)0x1);
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
            SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltResponseTransform, comp,
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

    ret = (xrltLogData *)xrltMalloc(sizeof(xrltLogData));

    if (ret == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "xrltLogCompile: Out of memory\n");
        return NULL;
    }

    memset(ret, 0, sizeof(xrltLogData));

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

        ASSERT_NODE_DATA(log, n);

        // Mark node as not ready.
        xrltNotReadyCounterIncrease(ctx, log);

        // Schedule log content transforms.
        if (!xrltElementTransform(ctx, logcomp->node->children, log)) {
            return FALSE;
        }

        // Schedule the next call.
        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltLogTransform, comp, insert,
                          (void *)log);
    } else {
        log = (xmlNodePtr)data;

        ASSERT_NODE_DATA(log, n);

        if (n->count > 0) {
            // The second call.
            xrltNotReadyCounterDecrease(ctx, log);

            if (n->count > 0) {
                // Node is not ready. Schedule the third call of this function
                // inside node's personal ready queue and return.
                SCHEDULE_CALLBACK(ctx, &n->tcb,xrltLogTransform, comp,
                                  insert, (void *)log);

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
xrltFunctionCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltFunctionData         *ret = NULL;

    if (prevcomp == NULL) {
        // First compile pass.
        ret = xrltMalloc(sizeof(xrltFunctionData));

        if (ret == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "xrltFunctionCompile: Out of memory\n");
            return NULL;
        }

        memset(ret, 0, sizeof(xrltFunctionData));

        if (sheet->funcs == NULL) {
            sheet->funcs = xmlHashCreate(20);

            if (sheet->funcs == NULL) {
                xrltTransformError(NULL, sheet, node,
                                   "Functions hash creation failed\n");
                goto error;
            }
        }

        ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

        if (xmlValidateNCName(ret->name, 0)) {
            xrltTransformError(NULL, sheet, node, "Invalid function name\n");
            goto error;
        }

        if (xmlHashAddEntry3(sheet->funcs, ret->name, NULL, NULL, ret)) {
            // Yeah, it could be out of memory error, but it's a duplicate name
            // most likely.
            xrltTransformError(NULL, sheet, node, "Duplicate function name\n");
            goto error;
        }

        ret->children = node->children;
    } else {
        // Second compile pass.
        ret = prevcomp;
    }

    return ret;

  error:
    xrltFunctionFree(ret);

    return NULL;
}


void
xrltFunctionFree(void *comp)
{
    if (comp != NULL) {
        xrltFunctionData  *f = (xrltFunctionData *) comp;

        if (f->name != NULL) { xmlFree(f->name); }

        xrltFree(comp);
    }
}


xrltBool
xrltFunctionTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    return TRUE;
}


void *
xrltApplyCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltApplyData            *ret = NULL;

    if (prevcomp == NULL) {
        // First compile pass.
        ret = xrltMalloc(sizeof(xrltApplyData));

        if (ret == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "xrltApplyCompile: Out of memory\n");
            return NULL;
        }

        memset(ret, 0, sizeof(xrltApplyData));

    } else {
        // Second compile pass.
        ret = prevcomp;
    }

    return ret;
}


void
xrltApplyFree(void *comp)
{
    if (comp != NULL) {
        xrltFree(comp);
    }
}


xrltBool
xrltApplyTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                   void *data)
{
    return TRUE;
}


void *
xrltIfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
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
