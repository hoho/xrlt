#include <libxml/tree.h>

#include "transform.h"
#include "builtins.h"


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


inline static size_t
xrltCompiledElementPush(xrltRequestsheetPtr sheet,
                        xrltTransformFunction transform, xrltFreeFunction free,
                        void *data)
{
    xrltRequestsheetPrivate  *priv = (xrltRequestsheetPrivate *)sheet->_private;
    xrltCompiledElementPtr    arr;

    if (priv->compLen >= priv->compSize) {
        if (priv->compLen == 0) {
            // We keep 0 index for non XRLT nodes.
            priv->compLen++;
        }

        arr = xrltRealloc(
            priv->comp,
            sizeof(xrltCompiledElement) * (priv->compSize + 20)
        );

        if (arr == NULL) {
            xrltTransformError(NULL, sheet, NULL,
                               "xrltCompiledElementPush: Out of memory\n");
            return 0;
        }

        memset(arr + sizeof(xrltCompiledElement) * priv->compSize, 0,
               sizeof(xrltCompiledElement) * 20);
        priv->comp = arr;
        priv->compSize += 20;
    }

    priv->comp[priv->compLen].transform = transform;
    priv->comp[priv->compLen].free = free;
    priv->comp[priv->compLen].data = data;

    return priv->compLen++;
}


inline static size_t
xrltTransformingElementPush(xrltContextPtr ctx, int count,
                            xrltFreeFunction free, void *data)
{
    if (ctx == NULL) { return 0; }

    xrltContextPrivate          *priv = (xrltContextPrivate *)ctx->_private;
    xrltTransformingElementPtr   arr;

    if (priv->trLen >= priv->trSize) {
        if (priv->trLen == 0) {
            // We start indexing from 1.
            priv->trLen++;
        }

        arr = xrltRealloc(
                priv->tr,
                sizeof(xrltTransformingElement) * (priv->trSize + 20)
        );

        if (arr == NULL) {
            xrltTransformError(ctx, NULL, NULL,
                               "xrltTransformingElementPush: Out of memory\n");
            return 0;
        }

        memset(arr + sizeof(xrltTransformingElement) * priv->trSize, 0,
               sizeof(xrltTransformingElement) * 20);
        priv->tr = arr;
        priv->trSize += 20;
    }

    priv->tr[priv->trLen].count = count;
    priv->tr[priv->trLen].free = free;
    priv->tr[priv->trLen].data = data;

    return priv->trLen++;
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
    void                     *comp, *prevcomp;
    size_t                    num;

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

        elem = (xrltElementPtr)xmlHashLookup3(
            xrltRegisteredElements, name, _pass, ns
        );

        printf("0000000 %s %d %s %p\n", (char *)first->name, pass, (char *)_pass, elem);

        if (elem == NULL) {
            if (pass == XRLT_PASS1) {
                // Set _private to zero on the first compile pass.
                first->_private = NULL;
            }

            if (pass == XRLT_PASS2 && first->_private == NULL &&
                first->ns != NULL && xmlStrEqual(first->ns->href, XRLT_NS))
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
            num = pass == XRLT_PASS2 ? (size_t)first->_private : 0;
            prevcomp = num > 0 ? priv->comp[num].data : NULL;

            if (elem->compile != NULL) {
                comp = elem->compile(sheet, first, prevcomp);

                if (comp == NULL) {
                    if (num > 0) {
                        // In case of error on the second compile pass, the
                        // data from the first compile pass should be freed
                        // by the compile function, so, we clean element's
                        // data to avoid duplicate freeing.
                        memset(priv->comp + num, 0,
                               sizeof(xrltCompiledElement));
                    }
                    return FALSE;
                }
            } else {
                comp = NULL;
            }

            if (num == 0) {
                num = xrltCompiledElementPush(
                    sheet, elem->transform, elem->free, comp
                );
            } else {
                priv->comp[num].transform = elem->transform;
                priv->comp[num].free = elem->free;
                priv->comp[num].data = comp;
            }

            if (num > 0) {
                first->_private = (void *)num;
            } else {
                if (comp != NULL) { elem->free(comp); }
                return FALSE;
            }

        }

        first = first->next;
    }

    return TRUE;
}


xrltBool
xrltElementTransform(xrltContextPtr ctx, xmlNodePtr first, xmlNodePtr insert)
{
    if (ctx == NULL || insert == NULL) { return FALSE; }

    xrltContextPrivate       *priv = ctx->_private;
    xrltRequestsheetPrivate  *spriv;
    xrltCompiledElementPtr    c;
    size_t                    index;
    xmlNodePtr                newinsert;

    spriv = (xrltRequestsheetPrivate *)ctx->sheet->_private;

    while (first != NULL) {
        index = (size_t)first->_private;

        if (index == 0) {
            newinsert = xmlDocCopyNode(first, priv->responseDoc, 2);

            if (newinsert == NULL) {
                xrltTransformError(ctx, NULL, first,
                                   "Failed to copy element to response doc\n");
                return FALSE;
            }

            if (xmlAddChild(insert, newinsert) == NULL) {
                xmlFreeNode(newinsert);

                xrltTransformError(ctx, NULL, first,
                                   "Failed to add element to response doc\n");
                return FALSE;
            }

            if (!xrltElementTransform(ctx, first->children, newinsert)) {
                return FALSE;
            }
        } else {
            c = &spriv->comp[index];

            if (!xrltTransformCallbackQueuePush(ctx, &priv->tcb, c->transform,
                                                c->data, insert, NULL))
            {
                xrltTransformError(ctx, NULL, first,
                                   "Failed to push callback\n");
                return FALSE;
            }
        }

        first = first->next;
    }

    return TRUE;
}
