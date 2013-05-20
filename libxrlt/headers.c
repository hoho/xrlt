/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "headers.h"


void *
xrltResponseHeaderCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                          void *prevcomp)
{
    xrltResponseHeaderData  *ret = NULL;
    xrltBool                 hasxrlt;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltResponseHeaderData*,
                sizeof(xrltResponseHeaderData), NULL);

    if (xmlStrEqual(node->name, (const xmlChar *)"response-cookie")) {
        ret->cookie = TRUE;
    }

    if (ret->cookie) {
        if (!xrltCompileCheckSubnodes(sheet, node,
                                      XRLT_ELEMENT_NAME, XRLT_ELEMENT_VALUE,
                                      XRLT_ELEMENT_PATH, XRLT_ELEMENT_EXPIRES,
                                      XRLT_ELEMENT_DOMAIN, XRLT_ELEMENT_TEST,
                                      &hasxrlt))
        {
            goto error;
        }
    } else {
        if (!xrltCompileCheckSubnodes(sheet, node,
                                      XRLT_ELEMENT_NAME, XRLT_ELEMENT_VALUE,
                                      XRLT_ELEMENT_TEST, NULL, NULL, NULL,
                                      &hasxrlt))
        {
            goto error;
        }
    }

    if (!xrltCompileValue(sheet, node, NULL, NULL, XRLT_ELEMENT_NAME,
                          XRLT_ELEMENT_NAME, TRUE, &ret->name))
    {
        goto error;
    }

    if (!xrltCompileValue(sheet, node, hasxrlt ? NULL : node->children,
                          XRLT_ELEMENT_ATTR_SELECT, NULL, XRLT_ELEMENT_VALUE,
                          TRUE, &ret->val))
    {
        goto error;
    }

    if (!xrltCompileValue(sheet, node, NULL, XRLT_ELEMENT_ATTR_TEST, NULL,
                          XRLT_ELEMENT_TEST, FALSE, &ret->test))
    {
        goto error;
    }

    if (ret->cookie) {
        if (!xrltCompileValue(sheet, node, NULL, NULL, XRLT_ELEMENT_PATH,
                              XRLT_ELEMENT_PATH, TRUE, &ret->path))
        {
            goto error;
        }

        if (!xrltCompileValue(sheet, node, NULL, NULL, XRLT_ELEMENT_DOMAIN,
                              XRLT_ELEMENT_DOMAIN, TRUE, &ret->domain))
        {
            goto error;
        }

        if (!xrltCompileValue(sheet, node, NULL, NULL, XRLT_ELEMENT_EXPIRES,
                              XRLT_ELEMENT_EXPIRES, TRUE, &ret->expires))
        {
            goto error;
        }
    }

    ret->node = node;

    return ret;

  error:
    xrltResponseHeaderFree(ret);

    return NULL;
}


void
xrltResponseHeaderFree(void *comp)
{
    xrltResponseHeaderData  *ret = (xrltResponseHeaderData *)comp;
    if (ret != NULL) {
        CLEAR_XRLT_VALUE(ret->test);
        CLEAR_XRLT_VALUE(ret->name);
        CLEAR_XRLT_VALUE(ret->val);
        CLEAR_XRLT_VALUE(ret->path);
        CLEAR_XRLT_VALUE(ret->domain);
        CLEAR_XRLT_VALUE(ret->expires);

        xmlFree(ret);
    }
}


static void
xrltResponseHeaderTransformingFree(void *data)
{
    xrltResponseHeaderTransformingData  *tdata;

    if (data != NULL) {
        tdata = (xrltResponseHeaderTransformingData *)data;

        if (tdata->name != NULL) { xmlFree(tdata->name); }
        if (tdata->val != NULL) { xmlFree(tdata->val); }
        if (tdata->path != NULL) { xmlFree(tdata->path); }
        if (tdata->domain != NULL) { xmlFree(tdata->domain); }
        if (tdata->expires != NULL) { xmlFree(tdata->expires); }

        xmlFree(data);
    }
}


xrltBool
xrltResponseHeaderTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                            void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr                           node;
    xrltResponseHeaderData              *hcomp = (xrltResponseHeaderData *)comp;
    xrltNodeDataPtr                      n;
    xrltResponseHeaderTransformingData  *tdata;

    if (data == NULL) {
        NEW_CHILD(ctx, node, insert, "h");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, hcomp->node, tdata,
                    xrltResponseHeaderTransformingData*,
                    sizeof(xrltResponseHeaderTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltResponseHeaderTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "tmp");

        tdata->dataNode = node;

        if (hcomp->test.type == XRLT_VALUE_EMPTY) {
            tdata->stage = XRLT_RESPONSE_HEADER_TRANSFORM_NAMEVALUE;

            TRANSFORM_TO_STRING(ctx, node, &hcomp->name, &tdata->name);

            TRANSFORM_TO_STRING(ctx, node, &hcomp->val, &tdata->val);

            TRANSFORM_TO_STRING(ctx, node, &hcomp->path, &tdata->path);

            TRANSFORM_TO_STRING(ctx, node, &hcomp->domain, &tdata->domain);

            TRANSFORM_TO_STRING(ctx, node, &hcomp->expires, &tdata->expires);
        } else {
            tdata->stage = XRLT_RESPONSE_HEADER_TRANSFORM_TEST;

            TRANSFORM_TO_BOOLEAN(ctx, node, &hcomp->test, &tdata->test);
        }

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltResponseHeaderTransform, comp,
                          node, tdata);

        ctx->headerCount++;
    } else {
        tdata = (xrltResponseHeaderTransformingData *)data;

        ASSERT_NODE_DATA(tdata->dataNode, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltResponseHeaderTransform,
                              comp, insert, data);

            return TRUE;
        }

        if (tdata->stage == XRLT_RESPONSE_HEADER_TRANSFORM_TEST) {
            if (tdata->test) {
                tdata->stage = XRLT_RESPONSE_HEADER_TRANSFORM_NAMEVALUE;

                TRANSFORM_TO_STRING(ctx, insert, &hcomp->name, &tdata->name);

                TRANSFORM_TO_STRING(ctx, insert, &hcomp->val, &tdata->val);

                TRANSFORM_TO_STRING(ctx, insert, &hcomp->path, &tdata->path);

                TRANSFORM_TO_STRING(ctx, insert, &hcomp->domain,
                                    &tdata->domain);

                TRANSFORM_TO_STRING(ctx, insert, &hcomp->expires,
                                    &tdata->expires);

                SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltResponseHeaderTransform,
                                  comp, insert, data);
            } else {
                COUNTER_DECREASE(ctx, tdata->node);

                REMOVE_RESPONSE_NODE(ctx, tdata->node);

                ctx->headerCount--;
                if (ctx->headerCount == 0 && ctx->chunk.first != NULL) {
                    ctx->cur |= XRLT_STATUS_CHUNK;
                }
            }

            return TRUE;
        }

        xrltString   name;
        xrltString   val;

        name.len = (size_t)xmlStrlen(tdata->name);
        name.data = name.len > 0 ? (char *)tdata->name : NULL;

        val.len = (size_t)xmlStrlen(tdata->val);
        val.data = val.len > 0 ? (char *)tdata->val : NULL;

        if (!xrltHeaderListPush(&ctx->header, hcomp->cookie, &name, &val)) {
            RAISE_OUT_OF_MEMORY(ctx, NULL, hcomp->node);
            return FALSE;
        }

        ctx->cur |= XRLT_STATUS_HEADER;

        ctx->headerCount--;
        if (ctx->headerCount == 0 && ctx->chunk.first != NULL) {
            ctx->cur |= XRLT_STATUS_CHUNK;
        }

        COUNTER_DECREASE(ctx, tdata->node);

        REMOVE_RESPONSE_NODE(ctx, tdata->node);
    }

    return TRUE;
}


void *
xrltHeaderElementCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                         void *prevcomp)
{
    xrltHeaderElementData  *ret = NULL;
    xrltBool                hasxrlt;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltHeaderElementData*,
                sizeof(xrltHeaderElementData), NULL);

    if (xmlStrEqual(node->name, (const xmlChar *)"cookie")) {
        ret->cookie = TRUE;
    }

    if (!xrltCompileCheckSubnodes(sheet, node, XRLT_ELEMENT_NAME,
                                  XRLT_ELEMENT_VALUE, NULL, NULL, NULL, NULL,
                                  &hasxrlt))
    {
        goto error;
    }

    if (!xrltCompileValue(sheet, node, NULL, NULL, XRLT_ELEMENT_NAME,
                          XRLT_ELEMENT_NAME, TRUE, &ret->name))
    {
        goto error;
    }

    if (!xrltCompileValue(sheet, node, hasxrlt ? NULL : node->children,
                          XRLT_ELEMENT_ATTR_SELECT, NULL, XRLT_ELEMENT_VALUE,
                          TRUE, &ret->val))
    {
        goto error;
    }

    ret->node = node;

    return ret;

  error:
    xrltHeaderElementFree(ret);

    return NULL;
}


void
xrltHeaderElementFree(void *comp)
{
    xrltHeaderElementData  *ret = (xrltHeaderElementData *)comp;
    if (ret != NULL) {
        CLEAR_XRLT_VALUE(ret->name);
        CLEAR_XRLT_VALUE(ret->val);

        xmlFree(ret);
    }
}


static void
xrltHeaderElementTransformingFree(void *data)
{
    if (data != NULL) {
        xrltHeaderElementTransformingData  *tdata;

        tdata = (xrltHeaderElementTransformingData *)data;

        if (tdata->name != NULL) { xmlFree(tdata->name); }
        if (tdata->val != NULL) { xmlFree(tdata->val); }

        xmlFree(data);
    }
}


static xrltBool
xrltNeedHeaderCallback(xrltContextPtr ctx, size_t id, xrltTransformValue *val,
                       void *data)
{
    xrltHeaderElementTransformingData  *tdata;
    tdata = (xrltHeaderElementTransformingData *)data;

    tdata->stage = XRLT_HEADER_ELEMENT_TRANSFORM_VALUE;

    if (val->data.data != NULL) {
        tdata->val = xmlStrndup((xmlChar *)val->data.data, val->data.len);
    } else {
        TRANSFORM_TO_STRING(ctx, tdata->dataNode, &tdata->comp->val,
                            &tdata->val);
    }

    SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltHeaderElementTransform, tdata->comp,
                      NULL, tdata);

    return TRUE;
}


xrltBool
xrltHeaderElementTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                           void *data)
{
    xrltHeaderElementData              *hcomp = (xrltHeaderElementData *)comp;
    xrltHeaderElementTransformingData  *tdata;
    xmlNodePtr                          node;
    xrltNodeDataPtr                     n;
    size_t                              id;
    xrltString                          s;

    if (data == NULL) {
        NEW_CHILD(ctx, node, insert, "h");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, hcomp->node, tdata,
                    xrltHeaderElementTransformingData*,
                    sizeof(xrltHeaderElementTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltHeaderElementTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;
        tdata->comp = hcomp;

        NEW_CHILD(ctx, node, node, "n");

        tdata->dataNode = node;

        TRANSFORM_TO_STRING(ctx, node, &hcomp->name, &tdata->name);

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltHeaderElementTransform, comp,
                          insert, tdata);
    } else {
        tdata = (xrltHeaderElementTransformingData *)data;

        ASSERT_NODE_DATA(tdata->dataNode, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltHeaderElementTransform, comp,
                              insert, tdata);

            return TRUE;
        }

        switch (tdata->stage) {
            case XRLT_HEADER_ELEMENT_TRANSFORM_NAME:
                id = ++ctx->includeId;

                s.data = (char *)tdata->name;
                s.len = (size_t)xmlStrlen(tdata->name);

                if (!xrltNeedHeaderListPush(&ctx->needHeader, id,
                                            hcomp->cookie, &s))
                {
                    RAISE_OUT_OF_MEMORY(ctx, NULL, hcomp->node);
                    return FALSE;
                }

                ctx->cur |= XRLT_STATUS_NEED_HEADER;

                xrltInputSubscribe(ctx, XRLT_PROCESS_HEADER, id,
                                   xrltNeedHeaderCallback, tdata);

                return TRUE;

            case XRLT_HEADER_ELEMENT_TRANSFORM_VALUE:
                if (tdata->val != NULL) {
                    node = xmlNewText(tdata->val);

                    xmlAddNextSibling(tdata->node, node);
                }

                break;
        }

        COUNTER_DECREASE(ctx, tdata->node);

        REMOVE_RESPONSE_NODE(ctx, tdata->node);
    }

    return TRUE;
}
