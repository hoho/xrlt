/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "log.h"


void *
xrltLogCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltLogData  *ret = NULL;
    xmlChar      *level = NULL;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltLogData*, sizeof(xrltLogData),
                NULL);

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
        xmlFree(comp);
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

        NEW_CHILD(ctx, log, insert, "log");

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
