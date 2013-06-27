/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include <string.h>
#include <libxml/xpathInternals.h>
#include <libxslt/transform.h>

#include "xrlt.h"
#include "transform.h"
#include "include.h"
#include "import.h"
#include "xpathfuncs.h"

#ifndef __XRLT_NO_JAVASCRIPT__
    #include "js.h"
#endif


static xrltBool
xrltRemoveBlankNodesAndComments(xrltRequestsheetPtr sheet, xmlNodePtr first,
                                xrltBool inResponse) {
    xmlNodePtr   tmp, tmp2, tmp3, textNode;
    xrltBool     isResponse;

    while (first != NULL) {
        isResponse = FALSE;

        if (xrltIsXRLTNamespace(first)) {
            if (inResponse &&
                xmlStrEqual(first->name, (const xmlChar *)"text"))
            {
                tmp = first->children;

                while (tmp != NULL) {
                    if (tmp->type != XML_TEXT_NODE &&
                        tmp->type != XML_CDATA_SECTION_NODE)
                    {
                        ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
                        return FALSE;
                    }

                    tmp = tmp->next;
                }

                textNode = first;
                tmp = first;
                tmp2 = tmp->children;

                first = first->next;

                while (tmp2 != NULL) {
                    tmp3 = tmp2->next;

                    tmp = xmlAddNextSibling(tmp, tmp2);

                    if (tmp == NULL) {
                        ERROR_ADD_NODE(NULL, sheet, tmp2);
                        return FALSE;
                    }

                    tmp2 = tmp3;
                }

                xmlUnlinkNode(textNode);
                xmlFreeNode(textNode);

                continue;
            }

            if (xmlStrEqual(first->name, (const xmlChar *)"response")) {
                isResponse = TRUE;
            }
        }

        if (xmlIsBlankNode(first) || first->type == XML_COMMENT_NODE) {
            tmp = first->next;
            xmlUnlinkNode(first);
            xmlFreeNode(first);
            first = tmp;
        } else {
            if (!xrltRemoveBlankNodesAndComments(sheet, first->children,
                                                 inResponse || isResponse))
            {
                return FALSE;
            }

            first = first->next;
        }
    }

    return TRUE;
}


static void
xrltRegisterNodeFunc(xmlNodePtr node)
{
    xrltNodeDataPtr   data;

    data = (xrltNodeDataPtr)xmlMalloc(sizeof(xrltNodeData));

    if (data != NULL) {
        memset(data, 0, sizeof(xrltNodeData));
    }

    node->_private = data;
}


static void
xrltDeregisterNodeFunc(xmlNodePtr node)
{
    xrltNodeDataPtr   data = (xrltNodeDataPtr)node->_private;

    if (data != NULL) {
        if (data->data != NULL && data->free != NULL) {
            data->free(data->data);
        }

        if (data->tcb.first != NULL) {
            xrltTransformCallbackQueueClear(&data->tcb);
        }

        xmlFree(data);
    }
}


xrltRequestsheetPtr
xrltRequestsheetCreate(xmlDocPtr doc)
{
    if (doc == NULL) { return NULL; }

    xrltRequestsheetPtr       ret;
    xmlNodePtr                root;

    XRLT_MALLOC(NULL, NULL, NULL, ret, xrltRequestsheetPtr,
                sizeof(xrltRequestsheet), NULL);

    root = xmlDocGetRootElement(doc);

    if (root == NULL ||
        !xmlStrEqual(root->name, XRLT_ROOT_NAME) ||
        root->ns == NULL ||
        !xmlStrEqual(root->ns->href, XRLT_NS))
    {
        ERROR_UNEXPECTED_ELEMENT(NULL, NULL, root);

        goto error;
    }

    if (!xrltProcessImports(ret, root, 1)) {
        goto error;
    }

    xmlReconciliateNs(doc, root);

    if (!xrltRemoveBlankNodesAndComments(ret, root, FALSE)) {
        goto error;
    }

    ret->pass = XRLT_PASS1;
    if (!xrltElementCompile(ret, root->children)) {
        goto error;
    }

    ret->pass = XRLT_PASS2;
    if (!xrltElementCompile(ret, root->children)) {
        goto error;
    }

    ret->pass = XRLT_COMPILED;

    ret->doc = doc;

    return ret;

  error:
    xrltRequestsheetFree(ret);

    return NULL;
}


void
xrltRequestsheetFree(xrltRequestsheetPtr sheet)
{
    if (sheet == NULL) { return; }

    if (sheet->funcs != NULL) {
        xmlHashFree(sheet->funcs, NULL);
    }

    if (sheet->transforms != NULL) {
        xmlHashFree(sheet->transforms, NULL);
    }

    if (sheet->doc != NULL) {
        xmlFreeDoc(sheet->doc);
    }

#ifndef __XRLT_NO_JAVASCRIPT__
    if (sheet->js != NULL) {
        xrltJSContextFree((xrltJSContextPtr)sheet->js);
    }
#endif

    xmlFree(sheet);
}


xrltContextPtr
xrltContextCreate(xrltRequestsheetPtr sheet, xmlChar **params)
{
    xrltContextPtr                ret;
    xmlNodePtr                    response;
    xrltIncludeTransformingData  *data;
    xrltNodeDataPtr               n;

    if (sheet == NULL) { return NULL; }

    XRLT_MALLOC(NULL, NULL, NULL, ret, xrltContextPtr, sizeof(xrltContext),
                NULL);

    ret->sheet = sheet;

    ret->responseDoc = xmlNewDoc(NULL);

    if (ret->responseDoc == NULL) {
        ERROR_CREATE_NODE(NULL, NULL, NULL);

        goto error;
    }

    response = xmlNewDocNode(ret->responseDoc, NULL,
                             (const xmlChar *)"response", NULL);

    if (response == NULL) {
        ERROR_CREATE_NODE(ret, NULL, NULL);

        goto error;
    }

    xmlDocSetRootElement(ret->responseDoc, response);

    NEW_CHILD_GOTO(ret, ret->response, response, "response");
    NEW_CHILD_GOTO(ret, ret->var, response, "var");

    ret->xpathDefault = xmlNewDoc(NULL);

    ret->xpath = xmlXPathNewContext(ret->responseDoc);

    xmlXPathRegisterVariableLookup(ret->xpath, xrltVariableLookupFunc, ret);

    if (ret->xpath == NULL) {
        xrltTransformError(ret, NULL, NULL,
                           "Failed to create XPath context\n");
        goto error;
    }

    if (ret->xpath->varHash == NULL) {
        ret->xpath->varHash = xmlHashCreate(5);
        if (ret->xpath->varHash == NULL) {
            xrltTransformError(ret, NULL, NULL,
                               "Variable hash creation failed\n");
            goto error;
        }
    }

    ret->xpath->extra = ret;

    if (!xrltRegisterFunctions(ret->xpath)) {
        xrltTransformError(ret, NULL, NULL, "Failed to register functions\n");
        goto error;
    }

    ret->sheetNode = xmlDocGetRootElement(sheet->doc);

    ASSERT_NODE_DATA_GOTO(response, n);

    data = (xrltIncludeTransformingData*)xmlMalloc(
        sizeof(xrltIncludeTransformingData)
    );

    if (data == NULL) {
        ERROR_OUT_OF_MEMORY(ret, NULL, NULL);
        goto error;
    }

    memset(data, 0, sizeof(xrltIncludeTransformingData));

    ret->headersData = data;

    data->srcNode = sheet->querystringNode;
    data->comp = (xrltCompiledIncludeData *)sheet->querystringComp;
    data->status = 200;

    n->sr = data;

    NEW_CHILD_GOTO(ret, data->node, response, "req");
    NEW_CHILD_GOTO(ret, data->hnode, response, "h");
    NEW_CHILD_GOTO(ret, data->cnode, response, "c");

    if (!xrltRequestInputTransform(ret, NULL, NULL, data)) {
        goto error;
    }

    if (sheet->bodyNode != NULL) {
        data = (xrltIncludeTransformingData*)xmlMalloc(
            sizeof(xrltIncludeTransformingData)
        );

        if (data == NULL) {
            ERROR_OUT_OF_MEMORY(ret, NULL, NULL);
            goto error;
        }

        memset(data, 0, sizeof(xrltIncludeTransformingData));

        ret->bodyData = data;

        data->srcNode = sheet->bodyNode;
        data->comp = (xrltCompiledIncludeData *)sheet->bodyComp;
        data->status = 200;

        if (!xrltRequestInputTransform(ret, NULL, NULL, data)) {
            goto error;
        }
    }

    TRANSFORM_SUBTREE_GOTO(ret, ret->sheetNode->children, NULL);

    if (params != NULL) {
        ret->params = xmlHashCreate(10);

        if (ret->params == NULL) {
            ERROR_OUT_OF_MEMORY(ret, NULL, NULL);
            goto error;
        }

        int       i = 0;
        xmlChar  *name;
        xmlChar  *val;

        while (params[i] != NULL) {
            name = params[i++];
            val = params[i++];

            if (xmlHashAddEntry3(ret->params, name, NULL, NULL, val)) {
                xrltTransformError(ret, NULL, NULL,
                                   "Failed to process params\n");
                goto error;
            }
        }
    }

    return ret;

  error:
    if (ret != NULL) {
        xmlFree(ret);
    }

    return NULL;
}


void
xrltContextFree(xrltContextPtr ctx)
{
    if (ctx == NULL) { return; }

    xrltInputCallbackPtr     cb, tmp;
    size_t                   i;

    xrltSubrequestListClear(&ctx->sr);

    for (i = 0; i < ctx->icb.size; i++) {
        cb = ctx->icb.q[i].first;
        while (cb != NULL) {
            tmp = cb->next;
            xmlFree(cb);
            cb = tmp;
        }
    }
    if (ctx->icb.q != NULL) { xmlFree(ctx->icb.q); }

    if (ctx->xpath != NULL) { xmlXPathFreeContext(ctx->xpath); }

    if (ctx->xpathDefault != NULL) {
        xmlFreeDoc(ctx->xpathDefault);
    }

    if (ctx->var != NULL) {
        xmlNodePtr   n = ctx->var->children;
        xmlDocPtr    d;
        while (n != NULL) {
            d = (xmlDocPtr)n;
            n = n->next;
            xmlUnlinkNode((xmlNodePtr)d);
            xmlFreeDoc(d);
        }
    }

    if (ctx->responseDoc != NULL) {
        xmlFreeDoc(ctx->responseDoc);
    }

    if (ctx->tcb.first != NULL) {
        xrltTransformCallbackQueueClear(&ctx->tcb);
    }

    if (ctx->querystring.data != NULL) {
        xmlFree(ctx->querystring.data);
    }

    if (ctx->headersData != NULL) {
        xrltIncludeTransformingFree(ctx->headersData);
    }

    if (ctx->bodyData != NULL) {
        xrltIncludeTransformingFree(ctx->bodyData);
    }

    if (ctx->bodyBuf != NULL) {
        xmlBufferFree(ctx->bodyBuf);
    }

    if (ctx->params != NULL) {
        xmlHashFree(ctx->params, NULL);
    }

    xmlFree(ctx);
}


int
xrltTransform(xrltContextPtr ctx, size_t id, xrltTransformValue *val)
{
    if (ctx == NULL || val == NULL) { return XRLT_STATUS_ERROR; }

    xrltTransformFunction     func;
    void                     *comp;
    xmlNodePtr                insert;
    size_t                    varScope;
    xmlNodePtr                xpathContext;
    int                       xpathContextSize;
    int                       xpathProximityPosition;
    void                     *data;
    size_t                    len;
    xrltInputCallbackQueue   *q = NULL;
    xrltInputCallbackPtr      cb;
    xrltInputCallbackPtr      prevcb;

    ctx->cur = XRLT_STATUS_UNKNOWN;

    if (val->type != XRLT_TRANSFORM_VALUE_EMPTY) {
        len = ctx->icb.size;

        if (id == 0) {
            switch (val->type) {
                case XRLT_TRANSFORM_VALUE_BODY:
                    if (ctx->bodyData != NULL) {
                        if (!xrltRequestInputTransform(ctx, val, NULL,
                                                       ctx->bodyData))
                        {
                            ctx->cur |= XRLT_STATUS_ERROR;

                            return ctx->cur;
                        }
                    }
                    break;

                case XRLT_TRANSFORM_VALUE_HEADER:
                case XRLT_TRANSFORM_VALUE_COOKIE:
                case XRLT_TRANSFORM_VALUE_STATUS:
                case XRLT_TRANSFORM_VALUE_QUERYSTRING:
                    if (!xrltRequestInputTransform(ctx, val, NULL,
                                                   ctx->headersData))
                    {
                        ctx->cur |= XRLT_STATUS_ERROR;

                        return ctx->cur;
                    }
                    break;

                case XRLT_TRANSFORM_VALUE_ERROR:
                case XRLT_TRANSFORM_VALUE_EMPTY:
                    break;
            }
        } else if (id < len) {
            q = &ctx->icb.q[id];
        } else {
            xrltTransformError(ctx, NULL, NULL,
                               "Identifier is out of bounds (%zd)\n", id);
            ctx->cur |= XRLT_STATUS_ERROR;

            return ctx->cur;
        }

        if (q != NULL) {
            prevcb = NULL;
            cb = q->first;

            while (cb != NULL) {
                ctx->varScope = cb->varScope;

                if (!cb->func(ctx, val, cb->data)) {
                    ctx->cur |= XRLT_STATUS_ERROR;
                    return ctx->cur;
                }

                if (val->type == XRLT_TRANSFORM_VALUE_BODY &&
                    val->bodyval.last == TRUE)
                {
                    // If it's the last header or the last body chunk,
                    // remove it from the queue.
                    if (prevcb == NULL) {
                        q->first = cb->next;
                        if (q->first == NULL) { q->last = NULL; }
                    } else {
                        prevcb->next = cb->next;
                        if (prevcb->next == NULL) { q->last = prevcb; }
                    }

                    xmlFree(cb);
                    cb = prevcb == NULL ? NULL : prevcb->next;
                } else {
                    prevcb = cb;
                    cb = cb->next;
                }
            }
        }

        if (ctx->cur != XRLT_STATUS_UNKNOWN) {
            return ctx->cur;
        }
    }

    while (xrltTransformCallbackQueueShift(&ctx->tcb, &func, &comp,
                                           &insert, &varScope, &xpathContext,
                                           &xpathContextSize,
                                           &xpathProximityPosition, &data))
    {
        ctx->insert = insert;
        ctx->varScope = varScope;
        ctx->xpathContext = xpathContext;
        ctx->xpathContextSize = xpathContextSize;
        ctx->xpathProximityPosition = xpathProximityPosition;

        if (!func(ctx, comp, insert, data)) {
            ctx->cur |= XRLT_STATUS_ERROR;
            return ctx->cur;
        }

        if (ctx->cur != XRLT_STATUS_UNKNOWN) {
            // There is something to send.
            return ctx->cur;
        }
    }

    if (((xrltNodeDataPtr)ctx->responseDoc->_private)->count == 0) {
        ctx->cur |= XRLT_STATUS_DONE;

        //xmlDocFormatDump(stderr, ctx->responseDoc, 1);
    } else {
        ctx->cur |= XRLT_STATUS_WAITING;
    }

    return ctx->cur;
}


xrltBool
xrltXPathEval(xrltContextPtr ctx, xmlNodePtr insert, xrltXPathExpr *expr,
              xmlXPathObjectPtr *ret)
{
    xmlXPathObjectPtr  r;
    xmlNodePtr         node = insert;
    xrltNodeDataPtr    n;

    do {
        ASSERT_NODE_DATA(node, n);
        node = node->parent;
    } while (node != NULL && n->root == NULL);

    if (n->root == NULL)  {
        ctx->xpath->doc = ctx->xpathDefault;
        ctx->xpath->node = (xmlNodePtr)ctx->xpathDefault;
    } else {
        ctx->xpath->doc = n->root;
        ctx->xpath->node = (xmlNodePtr)n->root;
    }

    if (ctx->xpathContext != NULL) {
        ctx->xpath->node = ctx->xpathContext;
        ctx->xpath->contextSize = ctx->xpathContextSize;
        ctx->xpath->proximityPosition = ctx->xpathProximityPosition;
    }

    ctx->varContext = expr->scope;
    ctx->xpathWait = NULL;

    r = xmlXPathCompiledEval(expr->expr, ctx->xpath);

    if (r == NULL) {
        xrltTransformError(ctx, NULL, expr->src,
                           "Failed to evaluate expression\n");
        return FALSE;
    }

    if (ctx->xpathWait != NULL) {
        xmlXPathFreeObject(r);
        *ret = NULL;
    } else {
        *ret = r;
    }

    return TRUE;
}


void
xrltInit(void)
{
    xmlRegisterNodeDefault(xrltRegisterNodeFunc);
    xmlDeregisterNodeDefault(xrltDeregisterNodeFunc);
#ifndef __XRLT_NO_JAVASCRIPT__
    xrltJSInit();
#endif
}


void
xrltCleanup(void)
{
    xrltUnregisterBuiltinElements();
#ifndef __XRLT_NO_JAVASCRIPT__
    xrltJSFree();
#endif
    xsltCleanupGlobals();
    xmlCleanupParser();
}


size_t
xrltInputSubscribe(xrltContextPtr ctx, xrltInputFunction callback,
                   void *payload)
{
    xrltInputCallbackPtr     cb = NULL;
    xrltInputCallbackQueue  *q, *newq;
    size_t                   len;
    size_t                   id;

    XRLT_MALLOC(ctx, NULL, NULL, cb, xrltInputCallbackPtr,
                sizeof(xrltInputCallback), FALSE);

    cb->func = callback;
    cb->varScope = ctx->varScope;
    cb->data = payload;

    q = ctx->icb.q;
    len = ctx->icb.size;

    id = ctx->includeId + 1;

    if (id >= len) {
        newq = (xrltInputCallbackQueue *)xmlRealloc(
            q, sizeof(xrltInputCallbackQueue) * (id + 10)
        );

        if (newq == NULL) {
            ERROR_OUT_OF_MEMORY(ctx, NULL, NULL);
            goto error;
        }

        memset(newq + len, 0,
               sizeof(xrltInputCallbackQueue) * (id - len + 10));

        ctx->icb.size = id + 10;
        ctx->icb.q = newq;

        q = newq;
    }

    ctx->includeId = id;

    if (q[id].first == NULL) {
        q[id].first = cb;
    } else {
        q[id].last->next = cb;
    }
    q[id].last = cb;

    return id;

  error:
    if (cb != NULL) { xmlFree(cb); }
    return 0;
}
