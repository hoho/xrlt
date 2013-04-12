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
xrltContextCreate(xrltRequestsheetPtr sheet, xrltHeaderList *header)
{
    xrltContextPtr    ret;
    xmlNodePtr        response;

    if (sheet == NULL) { return NULL; }

    XRLT_MALLOC(ret, xrltContextPtr, sizeof(xrltContext),
                "xrltContextCreate", NULL);

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

    if (ctx != NULL) {
        xrltHeaderListClear(&ctx->inheader);
    }

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
