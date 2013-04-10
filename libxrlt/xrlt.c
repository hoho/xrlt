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
    xmlNodePtr                root;

    ret = (xrltRequestsheetPtr)xrltMalloc(sizeof(xrltRequestsheet));
    if (ret == NULL) {
        xrltTransformError(NULL, NULL, NULL,
                           "xrltRequestsheetCreate: Out of memory\n");
        return NULL;
    }

    memset(ret, 0, sizeof(xrltRequestsheet));

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


xrltContextPtr
xrltContextCreate(xrltRequestsheetPtr sheet, xrltHeaderList *header)
{
    xrltContextPtr            ret;
    xrltNodeDataPtr           n;

    if (sheet == NULL) { return NULL; }

    ret = xrltMalloc(sizeof(xrltContext));

    if (ret == NULL) {
        xrltTransformError(NULL, NULL, NULL,
                           "xrltContextCreate: Out of memory\n");
        goto error;
    }

    memset(ret, 0, sizeof(xrltContext));

    ret->sheet = sheet;

    if (header != NULL) {
        ret->inheader.first = header->first;
        ret->inheader.last = header->last;
        header->first = NULL;
        header->last = NULL;
    }

    ret->responseDoc = xmlNewDoc(NULL);

    if (ret->responseDoc == NULL) {
        xrltTransformError(NULL, NULL, NULL, "Response doc creation failed\n");
        goto error;
    }

    ASSERT_NODE_DATA_GOTO(sheet->response, n);

    if (!xrltTransformCallbackQueuePush(&ret->tcb, n->transform, n->data,
                                        (xmlNodePtr)ret->responseDoc, NULL))
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
    if (ctx == NULL) { return; }

    if (ctx != NULL) {
        xrltHeaderListClear(&ctx->inheader);
    }

    if (ctx->responseDoc != NULL) {
        xmlDocFormatDump(stdout, ctx->responseDoc, 1);
        xmlFreeDoc(ctx->responseDoc);
    }

    xrltTransformCallbackQueueClear(&ctx->tcb);

    xrltFree(ctx);
}


int
xrltTransform(xrltContextPtr ctx, xrltTransformValue *value)
{
    if (ctx == NULL) { return XRLT_STATUS_ERROR; }

    xrltTransformFunction     func;
    void                     *comp;
    xmlNodePtr                insert;
    void                     *data;

    ctx->cur = XRLT_STATUS_UNKNOWN;

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
