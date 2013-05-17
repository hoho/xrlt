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
    int                      _test;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltResponseHeaderData*,
                sizeof(xrltResponseHeaderData), NULL);

    if (!xrltCompileTestNameValueNode(sheet, node,
                                      XRLT_TESTNAMEVALUE_TEST_ATTR |
                                      XRLT_TESTNAMEVALUE_TEST_NODE |
                                      XRLT_TESTNAMEVALUE_NAME_ATTR |
                                      XRLT_TESTNAMEVALUE_NAME_NODE |
                                      XRLT_TESTNAMEVALUE_VALUE_ATTR |
                                      XRLT_TESTNAMEVALUE_VALUE_NODE,
                                      &_test, &ret->ntest, &ret->xtest,
                                      NULL, NULL, NULL,
                                      &ret->name, &ret->nname, &ret->xname,
                                      &ret->val, &ret->nval, &ret->xval))
    {
        goto error;
    }

    if (xmlStrEqual(node->name, (const xmlChar *)"response-cookie")) {
        ret->cookie = TRUE;
    }

    if (_test > 0) {
        ret->test = _test == 1 ? TRUE : FALSE;
    } else if (ret->ntest == NULL && ret->xtest.expr == NULL) {
        ret->test = TRUE;
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
        if (ret->xtest.expr) { xmlXPathFreeCompExpr(ret->xtest.expr); }

        if (ret->name != NULL) { xmlFree(ret->name); }
        if (ret->xname.expr != NULL) { xmlXPathFreeCompExpr(ret->xname.expr); }

        if (ret->val != NULL) { xmlFree(ret->val); }
        if (ret->xval.expr != NULL) { xmlXPathFreeCompExpr(ret->xval.expr); }

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

        if (hcomp->test) {
            tdata->stage = XRLT_RESPONSE_HEADER_TRANSFORM_NAMEVALUE;

            TRANSFORM_TO_STRING(ctx, node, hcomp->name, hcomp->nname,
                                &hcomp->xname, &tdata->name);

            TRANSFORM_TO_STRING(ctx, node, hcomp->val, hcomp->nval,
                                &hcomp->xval, &tdata->val);
        } else {
            tdata->stage = XRLT_RESPONSE_HEADER_TRANSFORM_TEST;

            TRANSFORM_TO_BOOLEAN(ctx, node, hcomp->test, hcomp->ntest,
                                 &hcomp->xtest, &tdata->test);
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

                TRANSFORM_TO_STRING(ctx, insert, hcomp->name, hcomp->nname,
                                    &hcomp->xname, &tdata->name);

                TRANSFORM_TO_STRING(ctx, insert, hcomp->val, hcomp->nval,
                                    &hcomp->xval, &tdata->val);

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

    XRLT_MALLOC(NULL, sheet, node, ret, xrltHeaderElementData*,
                sizeof(xrltHeaderElementData), NULL);

    if (!xrltCompileTestNameValueNode(sheet, node,
                                      XRLT_TESTNAMEVALUE_NAME_ATTR |
                                      XRLT_TESTNAMEVALUE_NAME_NODE |
                                      XRLT_TESTNAMEVALUE_VALUE_ATTR |
                                      XRLT_TESTNAMEVALUE_VALUE_NODE,
                                      NULL, NULL, NULL,
                                      NULL, NULL, NULL,
                                      &ret->name, &ret->nname, &ret->xname,
                                      &ret->val, &ret->nval, &ret->xval))
    {
        goto error;
    }

    if (xmlStrEqual(node->name, (const xmlChar *)"cookie")) {
        ret->cookie = TRUE;
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
        if (ret->name != NULL) { xmlFree(ret->name); }
        if (ret->xname.expr != NULL) { xmlXPathFreeCompExpr(ret->xname.expr); }

        if (ret->val != NULL) { xmlFree(ret->val); }
        if (ret->xval.expr != NULL) { xmlXPathFreeCompExpr(ret->xval.expr); }

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
        TRANSFORM_TO_STRING(ctx, tdata->dataNode, tdata->comp->val,
                            tdata->comp->nval, &tdata->comp->xval,
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

        TRANSFORM_TO_STRING(ctx, node, hcomp->name, hcomp->nname,
                            &hcomp->xname, &tdata->name);

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
