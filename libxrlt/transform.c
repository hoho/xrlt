#include <libxml/tree.h>

#include "transform.h"
#include "builtins.h"
#include "xrlterror.h"


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

    xrltBool   ret = FALSE;

    ret |= xrltElementRegister(XRLT_NS, (const xmlChar *)"response",
                               XRLT_REGISTER_TOPLEVEL | XRLT_COMPILE_PASS1,
                               xrltResponseCompile, xrltResponseFree,
                               xrltResponseTransform);

    ret |= xrltElementRegister(XRLT_NS, (const xmlChar *)"if",
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

    if (priv->compiledLen >= priv->compiledSize) {
        if (priv->compiledLen == 0) {
            // We keep 0 index for non XRLT nodes.
            priv->compiledLen++;
        }

        arr = xrltRealloc(
            priv->compiled,
            sizeof(xrltCompiledElement) * (priv->compiledSize + 20)
        );

        if (arr == NULL) {
            xrltTransformError(NULL, sheet, NULL,
                               "xrltCompiledElementPush: Out of memory\n");
            return 0;
        }

        memset(arr + sizeof(xrltCompiledElement) * priv->compiledSize, 0,
               sizeof(xrltCompiledElement) * 20);
        priv->compiled = arr;
        priv->compiledSize += 20;
    }

    priv->compiled[priv->compiledLen].transform = transform;
    priv->compiled[priv->compiledLen].free = free;
    priv->compiled[priv->compiledLen].data = data;

    return priv->compiledLen++;
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
    xmlNodePtr                tmp;
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
        if (xmlIsBlankNode(first)) {
            // Remove blank nodes.
            tmp = first->next;
            xmlUnlinkNode(first);
            xmlFreeNode(first);
            first = tmp;
            continue;
        }

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
            prevcomp = num > 0 ? priv->compiled[num].data : NULL;

            if (elem->compile != NULL) {
                comp = elem->compile(sheet, first, prevcomp);
                if (comp == NULL) { return FALSE; }
            } else {
                comp = NULL;
            }

            if (num == 0) {
                num = xrltCompiledElementPush(
                    sheet, elem->transform, elem->free, comp
                );
            } else {
                priv->compiled[num].transform = elem->transform;
                priv->compiled[num].free = elem->free;
                priv->compiled[num].data = comp;
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
