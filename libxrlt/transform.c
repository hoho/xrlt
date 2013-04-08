#include <libxml/tree.h>

#include "transform.h"
#include "builtins.h"
#include "include.h"


static xmlHashTablePtr xrltRegisteredElements = NULL;


static void
xrltRegisteredElementsDeallocator(void *payload, xmlChar *name)
{
    xrltElementPtr   elem = (xrltElementPtr)payload;
    if (elem != NULL) {
        if (elem->passes > 1) {
            elem->passes--;
        } else {
            xrltFree(elem);
        }
    }
}


void
xrltUnregisterBuiltinElements(void)
{
    if (xrltRegisteredElements == NULL) { return; }

    xmlHashFree(xrltRegisteredElements, xrltRegisteredElementsDeallocator);

    xrltRegisteredElements = NULL;
}


static inline xrltBool
xrltRegisterBuiltinElementsIfUnregistered(void)
{
    if (xrltRegisteredElements != NULL) { return TRUE; }

    xrltRegisteredElements = xmlHashCreate(20);

    if (xrltRegisteredElements == NULL) {
        xrltTransformError(
            NULL, NULL, NULL,
            "xrltRegisterBuiltinElementsIfUnregistered: Hash creation failed\n"
        );
        return FALSE;
    }

    xrltBool   ret = TRUE;

    ret &= xrltElementRegister(XRLT_NS, (const xmlChar *)"response",
                               XRLT_REGISTER_TOPLEVEL | XRLT_COMPILE_PASS1,
                               xrltResponseCompile, xrltResponseFree,
                               xrltResponseTransform);

    ret &= xrltElementRegister(XRLT_NS, (const xmlChar *)"include",
                               XRLT_COMPILE_PASS1, xrltIncludeCompile,
                               xrltIncludeFree, xrltIncludeTransform);

    ret &= xrltElementRegister(XRLT_NS, (const xmlChar *)"log",
                               XRLT_COMPILE_PASS1, xrltLogCompile, xrltLogFree,
                               xrltLogTransform);

    ret &= xrltElementRegister(XRLT_NS, (const xmlChar *)"function",
                               XRLT_REGISTER_TOPLEVEL | XRLT_COMPILE_PASS1 |
                               XRLT_COMPILE_PASS2, xrltFunctionCompile,
                               xrltFunctionFree, xrltFunctionTransform);

    ret &= xrltElementRegister(XRLT_NS, (const xmlChar *)"apply",
                               XRLT_COMPILE_PASS1 | XRLT_COMPILE_PASS2,
                               xrltApplyCompile, xrltApplyFree,
                               xrltApplyTransform);

    ret &= xrltElementRegister(XRLT_NS, (const xmlChar *)"if",
                               XRLT_COMPILE_PASS2,
                               xrltIfCompile, xrltIfFree, xrltIfTransform);

    if (!ret) {
        xrltUnregisterBuiltinElements();
        return FALSE;
    } else {
        return TRUE;
    }
}


xrltBool
xrltElementRegister(const xmlChar *ns, const xmlChar *name,
                    int flags, xrltCompileFunction compile,
                    xrltFreeFunction free,
                    xrltTransformFunction transform)
{
    if (name == NULL || transform == NULL) { return FALSE; }
    if (!xrltRegisterBuiltinElementsIfUnregistered()) { return FALSE; }

    xrltElementPtr   elem;
    const xmlChar   *pass1, *pass2;
    xrltBool         ret = TRUE;

    if (flags & XRLT_REGISTER_TOPLEVEL) {
        pass1 = flags & XRLT_COMPILE_PASS1 ? (const xmlChar *)"top1" : NULL;
        pass2 = flags & XRLT_COMPILE_PASS2 ? (const xmlChar *)"top2" : NULL;
    } else {
        pass1 = flags & XRLT_COMPILE_PASS1 ? (const xmlChar *)"1" : NULL;
        pass2 = flags & XRLT_COMPILE_PASS2 ? (const xmlChar *)"2" : NULL;
    }

    if (pass1 == NULL && pass2 == NULL) {
        // We will probably need to throw an error here, but I can't come with
        // a good error message.
        return TRUE;
    }

    elem = xrltMalloc(sizeof(xrltElement));

    if (elem == NULL) {
        xrltTransformError(NULL, NULL, NULL,
                           "xrltElementRegister: Out of memory\n");
        return FALSE;
    }

    memset(elem, 0, sizeof(xrltElement));

    elem->compile = compile;
    elem->free = free;
    elem->transform = transform;
    elem->passes = pass1 != NULL && pass2 != NULL ? 2 : 1;

    if (pass1 != NULL) {
        ret = !xmlHashAddEntry3(xrltRegisteredElements, name, pass1, ns, elem);
    }

    if (pass2 != NULL) {
        ret &= !xmlHashAddEntry3(xrltRegisteredElements, name, pass2, ns, elem);
    }

    if (ret) {
        return TRUE;
    } else {
        xrltTransformError(NULL, NULL, NULL,
                           "Failed to register '%s' element\n", (char *)name);
        return FALSE;
    }
}


xrltBool
xrltElementCompile(xrltRequestsheetPtr sheet, xmlNodePtr first)
{
    if (sheet == NULL || first == NULL) { return FALSE; }
    if (!xrltRegisterBuiltinElementsIfUnregistered()) { return FALSE; }

    xrltRequestsheetPrivate  *priv = sheet->_private;
    xrltCompilePass           pass = priv->pass;
    const xmlChar            *ns;
    const xmlChar            *name;
    const xmlChar            *_pass;
    xrltBool                  toplevel;
    xrltElementPtr            elem;
    void                     *comp;
    xrltNodeDataPtr           n;

    toplevel = first != NULL && first->parent != NULL &&
               xmlStrEqual(first->parent->name, XRLT_ROOT_NAME) &&
               first->parent->ns != NULL &&
               xmlStrEqual(first->parent->ns->href, XRLT_NS)
        ?
        TRUE
        :
        FALSE;

    switch (pass) {
        case XRLT_PASS1:
            _pass = (const xmlChar *)(toplevel ? "top1" : "1");
            break;
        case XRLT_PASS2:
            _pass = (const xmlChar *)(toplevel ? "top2" : "2");
            break;
        case XRLT_COMPILED:
            xrltTransformError(NULL, sheet, first, "Unexpected compile pass\n");
            return FALSE;
    }

    while (first != NULL) {
        ns = first->ns != NULL && first->ns->href != NULL
            ?
            first->ns->href
            :
            NULL;
        name = first->name;

        ASSERT_NODE_DATA(first, n);

        elem = (xrltElementPtr)xmlHashLookup3(
            xrltRegisteredElements, name, _pass, ns
        );

        printf("0000000 %s %d %s %p\n", (char *)first->name, pass, (char *)_pass, elem);

        if (elem == NULL) {
            if (pass == XRLT_PASS2 && !n->xrlt && first->ns != NULL &&
                xmlStrEqual(first->ns->href, XRLT_NS))
            {
                // Don't allow unknown elements from XRLT namespace.
                xrltTransformError(NULL, sheet, first, "Unknown element\n");
                return FALSE;
            }

            if (first->children != NULL &&
                !xrltElementCompile(sheet, first->children))
            {
                return FALSE;
            }
        } else {
            if (elem->compile != NULL) {
                comp = elem->compile(sheet, first, n->data);

                if (comp == NULL) {
                    return FALSE;
                }
            } else {
                comp = NULL;
            }

            n->transform = elem->transform;
            n->free = elem->free;
            n->data = comp;
            n->xrlt = TRUE;
        }

        first = first->next;
    }

    return TRUE;
}


static xrltBool
xrltCopyNonXRLT(xrltContextPtr ctx, void *comp, xmlNodePtr insert, void *data)
{
    if (ctx == NULL || insert == NULL || data == NULL) { return FALSE; }

    xrltContextPrivate  *priv = ctx->_private;
    xmlNodePtr           node = (xmlNodePtr)data;
    xmlNodePtr           newinsert;

    newinsert = xmlDocCopyNode(node, priv->responseDoc, 2);

    if (newinsert == NULL) {
        xrltTransformError(ctx, NULL, node,
                           "Failed to copy element to response doc\n");
        return FALSE;
    }

    if (xmlAddChild(insert, newinsert) == NULL) {
        xmlFreeNode(newinsert);

        xrltTransformError(ctx, NULL, node,
                           "Failed to add element to response doc\n");
        return FALSE;
    }

    if (!xrltElementTransform(ctx, node->children, newinsert)) {
        return FALSE;
    }

    return TRUE;
}


xrltBool
xrltElementTransform(xrltContextPtr ctx, xmlNodePtr first, xmlNodePtr insert)
{
    if (ctx == NULL || insert == NULL) { return FALSE; }

    xrltContextPrivate  *priv = ctx->_private;
    xrltNodeDataPtr      n;
    xrltBool             ret;

    while (first != NULL) {
        ASSERT_NODE_DATA(first, n);

        if (!n->xrlt) {
            ret = xrltTransformCallbackQueuePush(&priv->tcb, xrltCopyNonXRLT,
                                                 NULL, insert, first);
        } else {
            ret = xrltTransformCallbackQueuePush(&priv->tcb, n->transform,
                                                 n->data, insert, NULL);
        }

        if (!ret) {
            xrltTransformError(ctx, NULL, first, "Failed to push callback\n");
            return FALSE;
        }

        first = first->next;
    }

    return TRUE;
}


xrltBool
xrltHasXRLTElement(xmlNodePtr node)
{
    while (node != NULL) {
        if ((node->ns != NULL && xmlStrEqual(node->ns->href, XRLT_NS)) ||
            (!xrltHasXRLTElement(node->children)))
        {
            return FALSE;
        }

        node = node->next;
    }

    return TRUE;
}
