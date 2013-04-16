#include <string.h>
#include <libxml/xpathInternals.h>

#include "xrlt.h"
#include "transform.h"


static inline void
xrltRemoveBlankNodesAndComments(xmlNodePtr first) {
    xmlNodePtr   tmp;

    while (first != NULL) {
        if (xmlIsBlankNode(first) || first->type == XML_COMMENT_NODE) {
            tmp = first->next;
            xmlUnlinkNode(first);
            xmlFreeNode(first);
            first = tmp;
        } else {
            xrltRemoveBlankNodesAndComments(first->children);
            first = first->next;
        }
    }
}


static void
xrltRegisterNodeFunc(xmlNodePtr node)
{
    xrltNodeDataPtr   data;

    data = (xrltNodeDataPtr)xrltMalloc(sizeof(xrltNodeData));

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

        xrltTransformCallbackQueueClear(&data->tcb);

        xrltFree(data);
    }
}


xrltRequestsheetPtr
xrltRequestsheetCreate(xmlDocPtr doc)
{
    if (doc == NULL) { return NULL; }

    xrltRequestsheetPtr       ret;
    xmlNodePtr                root;

    XRLT_MALLOC(ret, xrltRequestsheetPtr, sizeof(xrltRequestsheet),
                "xrltRequestsheetCreate", NULL);

    root = xmlDocGetRootElement(doc);

    if (root == NULL ||
        !xmlStrEqual(root->name, XRLT_ROOT_NAME) ||
        root->ns == NULL ||
        !xmlStrEqual(root->ns->href, XRLT_NS))
    {
        xrltTransformError(NULL, NULL, root, "Unexpected element\n");
        goto error;
    }

    xrltRemoveBlankNodesAndComments(root);

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

    if (sheet->doc != NULL) {
        xmlFreeDoc(sheet->doc);
    }

    xrltFree(sheet);
}


static xmlXPathObjectPtr
xrltVariableLookupFunc(void *ctxt, const xmlChar *name, const xmlChar *ns_uri)
{
    xmlChar              id[sizeof(size_t) * 2];
    xmlXPathObjectPtr    ret;
    xrltContextPtr       ctx = (xrltContextPtr)ctxt;
    xmlNodePtr           node;
    xrltNodeDataPtr      n;

    if (ctx == NULL) { return NULL; }

    node = ctx->varContext;

    while (TRUE) {
        sprintf((char *)id, "%p", node);

        ret = (xmlXPathObjectPtr)xmlHashLookup2(ctx->xpath->varHash, id, name);

        if (ret != NULL) {
            if (ret->type == XPATH_NODESET) {
                node = xmlXPathNodeSetItem(ret->nodesetval, 0);

                n = (xrltNodeDataPtr)node->_private;

                if (n != NULL && n->count > 0) {
                    ctx->xpathWait = node;

                    return xmlXPathNewNodeSet(NULL);
                }
            }

            return xmlXPathObjectCopy(ret);
        }

        // Try upper scope.
        if (node != NULL) {
            node = node->parent;
        } else {
            break;
        }
    }

    return NULL;
}


xrltContextPtr
xrltContextCreate(xrltRequestsheetPtr sheet)
{
    xrltContextPtr   ret;
    xmlNodePtr       response;

    if (sheet == NULL) { return NULL; }

    XRLT_MALLOC(ret, xrltContextPtr, sizeof(xrltContext),
                "xrltContextCreate", NULL);

    ret->sheet = sheet;

    ret->responseDoc = xmlNewDoc(NULL);

    if (ret->responseDoc == NULL) {
        xrltTransformError(NULL, NULL, NULL, "Response doc creation failed\n");
        goto error;
    }

    response = xmlNewDocNode(ret->responseDoc, NULL,
                             (const xmlChar *)"response", NULL);

    if (response == NULL) {
        xrltTransformError(ret, NULL, NULL,
                           "Failed to create response element\n");
        goto error;
    }

    xmlDocSetRootElement(ret->responseDoc, response);

    NEW_CHILD_GOTO(ret, ret->response, response, "response");
    NEW_CHILD_GOTO(ret, ret->var, response, "var");

    ret->xpathDefault = xmlNewDoc(NULL);

    ret->xpath = xmlXPathNewContext(ret->responseDoc);

    xmlXPathRegisterVariableLookup(ret->xpath, xrltVariableLookupFunc, ret);

    if (ret->xpath == NULL) {
        xrltTransformError(ret, NULL, NULL, "Failed to create XPath context\n");
        goto error;
    }

    TRANSFORM_SUBTREE_GOTO(
        ret, xmlDocGetRootElement(sheet->doc)->children, NULL
    );

    return ret;

  error:
    if (ret != NULL) {
        xrltFree(ret);
    }

    return NULL;
}


void
xrltContextFree(xrltContextPtr ctx)
{
    if (ctx == NULL) { return; }

    xrltInputCallbackPtr     cb, tmp;
    size_t                   i;

    xrltNeedHeaderListClear(&ctx->needHeader);

    xrltSubrequestListClear(&ctx->sr);

    for (i = 0; i < ctx->icb.headerSize; i++) {
        cb = ctx->icb.header[i].first;
        while (cb != NULL) {
            tmp = cb->next;
            xrltFree(cb);
            cb = tmp;
        }
    }
    if (ctx->icb.header != NULL) { xrltFree(ctx->icb.header); }

    for (i = 0; i < ctx->icb.bodySize; i++) {
        cb = ctx->icb.body[i].first;
        while (cb != NULL) {
            tmp = cb->next;
            xrltFree(cb);
            cb = tmp;
        }
    }
    if (ctx->icb.body != NULL) { xrltFree(ctx->icb.body); }

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

    xrltTransformCallbackQueueClear(&ctx->tcb);

    xrltFree(ctx);
}


int
xrltTransform(xrltContextPtr ctx, size_t id, xrltTransformValue *val)
{
    if (ctx == NULL) { return XRLT_STATUS_ERROR; }

    xrltTransformFunction     func;
    void                     *comp;
    xmlNodePtr                insert;
    void                     *data;
    size_t                    len;
    xrltInputCallbackQueue   *q = NULL;
    xrltInputCallbackPtr      cb;
    xrltInputCallbackPtr      prevcb;

    ctx->cur = XRLT_STATUS_UNKNOWN;

    if (val != NULL) {
        if (val->type == XRLT_PROCESS_HEADER) {
            len = ctx->icb.headerSize;

            if (id < len) {
                q = &ctx->icb.header[id];
            }
        } else {
            len = ctx->icb.bodySize;

            if (id < len) {
                q = &ctx->icb.body[id];
            }
        }

        if (q != NULL) {
            prevcb = NULL;
            cb = q->first;

            while (cb != NULL) {
                if (!cb->func(ctx, id, val, cb->data)) {
                    ctx->cur |= XRLT_STATUS_ERROR;
                    return ctx->cur;
                }

                if (val->type == XRLT_PROCESS_HEADER || val->last == TRUE) {
                    // If it's a header callback or it's a last body callback,
                    // then remove it from the queue.
                    if (prevcb == NULL) {
                        q->first = cb->next;
                        if (q->first == NULL) { q->last = NULL; }
                    } else {
                        prevcb->next = cb->next;
                        if (prevcb->next == NULL) { q->last = prevcb; }
                    }

                    xrltFree(cb);
                    cb = prevcb == NULL ? NULL : prevcb->next;
                } else {
                    prevcb = cb;
                    cb = cb->next;
                }
            }
        }
    }

    while (xrltTransformCallbackQueueShift(&ctx->tcb, &func, &comp,
                                           &insert, &data))
    {
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
        xmlDocDump(stdout, ctx->responseDoc);
    } else {
        ctx->cur |= XRLT_STATUS_WAITING;
        printf("aaaaaa %d\n", ((xrltNodeDataPtr)ctx->responseDoc->_private)->count);
    }

    return ctx->cur;
}


xrltBool
xrltXPathEval(xrltContextPtr ctx, xmlNodePtr root, xmlNodePtr insert,
              xmlXPathCompExprPtr expr, xmlXPathObjectPtr *ret)
{
    xmlXPathObjectPtr  r;

    if (root == NULL) {
        ctx->xpath->doc = ctx->xpathDefault;
        ctx->xpath->node = (xmlNodePtr)ctx->xpathDefault;
    } else {
        ctx->xpath->doc = (xmlDocPtr)root;
        ctx->xpath->node = root;
    }

    ctx->varContext = insert;
    ctx->xpathWait = NULL;

    r = xmlXPathCompiledEval(expr, ctx->xpath);

    if (r == NULL) {
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
}


void
xrltCleanup(void)
{
    xrltUnregisterBuiltinElements();
}


xrltBool
xrltInputSubscribe(xrltContextPtr ctx, xrltTransformValueType type,
                   size_t id, xrltInputFunction callback, void *data)
{
    xrltInputCallbackPtr     cb = NULL;
    xrltInputCallbackQueue  *q, *newq;
    size_t                   len;

    XRLT_MALLOC(cb, xrltInputCallbackPtr, sizeof(xrltInputCallback),
                "xrltInputSubscribe", FALSE);

    //cb->type = type;
    //cb->id = id;
    cb->func = callback;
    cb->data = data;

    if (type == XRLT_PROCESS_HEADER) {
        q = ctx->icb.header;
        len = ctx->icb.headerSize;
    } else {
        q = ctx->icb.body;
        len = ctx->icb.bodySize;
    }

    if (id >= len) {
        newq = (xrltInputCallbackQueue *)xrltRealloc(
                q,
                sizeof(xrltInputCallbackQueue) * (id + 10)
        );

        if (newq == NULL) {
            xrltTransformError(ctx, NULL, NULL,
                               "xrltInputSubscribe: Out of memory\n");
            goto error;
        }

        memset(newq + len, 0,
               sizeof(xrltInputCallbackQueue) * (id - len + 10));

        if (type == XRLT_PROCESS_HEADER) {
            ctx->icb.headerSize = id + 10;
            ctx->icb.header = newq;
        } else {
            ctx->icb.bodySize = id + 10;
            ctx->icb.body = newq;
        }

        q = newq;
    }

    if (q[id].first == NULL) {
        q[id].first = cb;
    } else {
        q[id].last->next = cb;
    }
    q[id].last = cb;

    return TRUE;

  error:
    if (cb != NULL) { xrltFree(cb); }
    return FALSE;
}
