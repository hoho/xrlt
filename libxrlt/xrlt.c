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
    size_t                    i;

    if (priv->comp != NULL) {
        for (i = 1; i < priv->compLen; i++) {
            if (priv->comp[i].data != NULL) {
                priv->comp[i].free(priv->comp[i].data);
            }
        }

        xrltFree(priv->comp);
    }

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
    xrltCompiledElementPtr    r;

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
    r = &spriv->comp[(size_t)spriv->response->_private];

    if (!xrltTransformCallbackQueuePush(ret, &priv->tcb, r->transform, r->data,
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

    xrltTransformCallbackQueueClear(ctx, &priv->tcb);

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
    int                       ret = XRLT_STATUS_UNKNOWN;

    while (xrltTransformCallbackQueueShift(ctx, &priv->tcb, &func, &comp,
           &insert, &data))
    {
        if (!func(ctx, comp, insert, data)) {
            ret |= XRLT_STATUS_ERROR;
            break;
        }
    }

    ret |= XRLT_STATUS_DONE;

    return ret;
}


void
xrltCleanup(void)
{
    xrltUnregisterBuiltinElements();
}
