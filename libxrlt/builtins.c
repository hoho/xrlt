#include "transform.h"
#include "builtins.h"


void *
xrltResponseCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltRequestsheetPrivate  *priv = (xrltRequestsheetPrivate *)sheet->_private;

    if (priv->response == NULL) {
        priv->response = node;

        if (node->children != NULL &&
            !xrltElementCompile(sheet, node->children))
        {
            return NULL;
        }

        return node;
    } else {
        xrltTransformError(NULL, sheet, node,
                           "Duplicate response element\n");
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
    if (ctx == NULL || insert == NULL) { return FALSE; }

    xrltContextPrivate  *priv = ctx->_private;
    xmlNodePtr           response;

    if (data == NULL) {
        // On the first call, create response parent node and schedule next
        // call.
        response = xmlNewDocNode(priv->responseDoc, NULL,
                                 (const xmlChar *)"response", NULL);

        if (response == NULL) {
            xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                               "Failed to create response element\n");
            return FALSE;
        }

        xmlDocSetRootElement(priv->responseDoc, response);

        xrltElementTransform(ctx, ((xmlNodePtr)comp)->children, response);

        // Schedule the next call.
        if (!xrltTransformCallbackQueuePush(ctx, &priv->tcb,
                                            xrltResponseTransform, comp,
                                            response, (void *)0x1))
        {
            xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                               "Failed to push callback\n");
            return FALSE;
        }
    } else {
        // On the second call, check if something is ready to be sent and send
        // it if it is.
        xmlNodePtr   ret = (xmlNodePtr)data;

        response = insert;

        if (ret == (void *)0x1) {
            // It's the second call.
            ret = response->children;
        }

        while (ret != NULL && ret->_private == NULL) {
            // Send response chunk out.
            ret = ret->next;
        }

        if (ret != NULL) {
            // We still have some data that's not ready, schedule the next call.
            if (!xrltTransformCallbackQueuePush(ctx, &priv->tcb,
                                                xrltResponseTransform, comp,
                                                response, ret))
            {
                xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                                   "Failed to push callback\n");
                return FALSE;
            }
        }
    }
    printf("tratra\n");
    return TRUE;
}


void *
xrltFunctionCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltRequestsheetPrivate  *priv = (xrltRequestsheetPrivate *)sheet->_private;
    xrltFunctionData         *ret = NULL;

    if (prevcomp == NULL) {
        // First compile pass.
        ret = xrltMalloc(sizeof(xrltFunctionData));

        if (ret == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "xrltFunctionCompile: Out of memory\n");
            return NULL;
        }

        memset(ret, 0, sizeof(xrltFunctionData));

        if (priv->funcs == NULL) {
            priv->funcs = xmlHashCreate(20);

            if (priv->funcs == NULL) {
                xrltTransformError(NULL, sheet, node,
                                   "Functions hash creation failed\n");
                goto error;
            }
        }

        ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

        if (xmlValidateNCName(ret->name, 0)) {
            xrltTransformError(NULL, sheet, node, "Invalid function name\n");
            goto error;
        }

        if (xmlHashAddEntry3(priv->funcs, ret->name, NULL, NULL, ret)) {
            // Yeah, it could be out of memory error, but it's a duplicate name
            // most likely.
            xrltTransformError(NULL, sheet, node, "Duplicate function name\n");
            goto error;
        }

        ret->children = node->children;
    } else {
        // Second compile pass.
        ret = prevcomp;
    }

    if (ret->children != NULL &&
        !xrltElementCompile(sheet, ret->children))
    {
        goto error;
    }

    return ret;

  error:
    xrltFunctionFree(ret);

    return NULL;
}


void
xrltFunctionFree(void *comp)
{
    if (comp != NULL) {
        xrltFunctionData  *f = (xrltFunctionData *) comp;

        if (f->name != NULL) { xmlFree(f->name); }

        xrltFree(comp);
    }
}

xrltBool
xrltFunctionTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    return TRUE;
}


void *
xrltApplyCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltRequestsheetPrivate  *priv = (xrltRequestsheetPrivate *)sheet->_private;
    xrltApplyData            *ret = NULL;

    if (prevcomp == NULL) {
        // First compile pass.
        ret = xrltMalloc(sizeof(xrltApplyData));

        if (ret == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "xrltApplyCompile: Out of memory\n");
            return NULL;
        }

        memset(ret, 0, sizeof(xrltApplyData));

    } else {
        // Second compile pass.
        ret = prevcomp;
    }

    return ret;
}


void
xrltApplyFree(void *comp)
{
    if (comp != NULL) {
        xrltFree(comp);
    }
}

xrltBool
xrltApplyTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                   void *data)
{
    return TRUE;
}


void *
xrltIfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltIfData  *ret = NULL;
    xmlChar     *expr = NULL;

    expr = xmlGetProp(node, XRLT_ELEMENT_ATTR_TEST);

    if (expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to get 'test' attribute\n");
        goto error;
    }

    ret = (xrltIfData *)xrltMalloc(sizeof(xrltIfData));

    if (ret == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "xrltIfCompile: Out of memory\n");
        goto error;
    }

    memset(ret, 0, sizeof(xrltIfData));

    ret->test = xmlXPathCompile(expr);

    if (ret->test == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to compile expression\n");
        goto error;
    }

    if (node->children != NULL &&
        !xrltElementCompile(sheet, node->children))
    {
        goto error;
    }

    xmlFree(expr);

    ret->children = node->children;

    return ret;

  error:
    if (expr) { xmlFree(expr); }
    xrltIfFree(ret);

    return NULL;
}


void
xrltIfFree(void *comp)
{
    if (comp != NULL) {
        xmlXPathFreeCompExpr(((xrltIfData *)comp)->test);
        xrltFree(comp);
    }
}


xrltBool
xrltIfTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert, void *data)
{
    return TRUE;
}
