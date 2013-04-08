#include <string.h>

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

    data = xrltMalloc(sizeof(xrltNodeData));

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
    xrltRequestsheetPrivate  *priv;
    xmlNodePtr                root;

    ret = (xrltRequestsheetPtr)xrltMalloc(sizeof(xrltRequestsheet) +
                                          sizeof(xrltRequestsheetPrivate));
    if (ret == NULL) {
        xrltTransformError(NULL, NULL, NULL,
                           "xrltRequestsheetCreate: Out of memory\n");
        return NULL;
    }

    memset(ret, 0, sizeof(xrltRequestsheet) + sizeof(xrltRequestsheetPrivate));

    priv = (xrltRequestsheetPrivate *)(ret + 1);

    ret->_private = priv;

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

    priv->pass = XRLT_PASS1;
    if (!xrltElementCompile(ret, root->children)) {
        goto error;
    }

    priv->pass = XRLT_PASS2;
    if (!xrltElementCompile(ret, root->children)) {
        goto error;
    }

    priv->pass = XRLT_COMPILED;

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

    xrltRequestsheetPrivate  *priv = sheet->_private;

    if (priv->funcs != NULL) {
        xmlHashFree(priv->funcs, NULL);
    }

    if (sheet->doc != NULL) {
        xmlFreeDoc(sheet->doc);
    }

    xrltFree(sheet);
}


xrltContextPtr
xrltContextCreate(xrltRequestsheetPtr sheet, xrltHeaderList *header)
{
    xrltContextPtr            ret;
    xrltContextPrivate       *priv;
    xrltRequestsheetPrivate  *spriv;
    xrltNodeDataPtr           n;

    if (sheet == NULL) { return NULL; }

    ret = xrltMalloc(sizeof(xrltContext) + sizeof(xrltContextPrivate));
    if (ret == NULL) {
        xrltTransformError(NULL, NULL, NULL,
                           "xrltContextCreate: Out of memory\n");
        goto error;
    }

    memset(ret, 0, sizeof(xrltContext) + sizeof(xrltContextPrivate));

    priv = (xrltContextPrivate *)(ret + 1);

    ret->sheet = sheet;
    ret->_private = priv;

    if (header != NULL) {
        priv->header.first = header->first;
        priv->header.last = header->last;
        header->first = NULL;
        header->last = NULL;
    }

    priv->responseDoc = xmlNewDoc(NULL);

    if (priv->responseDoc == NULL) {
        xrltTransformError(NULL, NULL, NULL, "Response doc creation failed\n");
        goto error;
    }

    spriv = (xrltRequestsheetPrivate *)sheet->_private;
    n = (xrltNodeDataPtr)spriv->response->_private;

    if (!xrltTransformCallbackQueuePush(&priv->tcb, n->transform, n->data,
                                        (xmlNodePtr)priv->responseDoc, NULL))
    {
        xrltTransformError(NULL, NULL, NULL, "Failed to push callback\n");
        goto error;
    }

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
    xrltContextPrivate  *priv;

    if (ctx == NULL) { return; }

    priv = ctx->_private;

    if (priv != NULL) {
        xrltHeaderListClear(&priv->header);
    }

    if (priv->responseDoc != NULL) {
        xmlDocFormatDump(stdout, priv->responseDoc, 1);
        xmlFreeDoc(priv->responseDoc);
    }

    xrltTransformCallbackQueueClear(&priv->tcb);

    xrltFree(ctx);
}


int
xrltTransform(xrltContextPtr ctx, xrltTransformValue *value)
{
    if (ctx == NULL) { return XRLT_STATUS_ERROR; }

    xrltContextPrivate       *priv = (xrltContextPrivate *)ctx->_private;
    xrltTransformFunction     func;
    void                     *comp;
    xmlNodePtr                insert;
    void                     *data;

    ctx->cur = XRLT_STATUS_UNKNOWN;

    while (xrltTransformCallbackQueueShift(&priv->tcb, &func, &comp,
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

    ctx->cur |= XRLT_STATUS_DONE;

    return ctx->cur;
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
