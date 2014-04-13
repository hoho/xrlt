/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "response.h"


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
        ctx->varScope = ++ctx->maxVarScope;

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
            // TODO: Gather as many response chunks as possible into one buffer.
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
