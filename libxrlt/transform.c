#include <libxml/tree.h>

#include "transform.h"
#include "builtins.h"
#include "xrlterror.h"


static xmlHashTablePtr xrltRegisteredElements = NULL;


static void
xrltRegisteredElementsDeallocator(void *payload, xmlChar * name)
{
    xrltFree(payload);
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
            "xrltRegisterBuiltinElementsIfUnregistered: Out of memory\n"
        );
        return FALSE;
    }

    xrltBool   ret = FALSE;

    ret |= xrltElementRegister(XRLT_NS, (const xmlChar *)"response", TRUE,
                               xrltResponseCompile, xrltResponseFree,
                               xrltResponseTransform);

    ret |= xrltElementRegister(XRLT_NS, (const xmlChar *)"if", FALSE,
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
                    xrltBool toplevel, xrltCompileFunction compile,
                    xrltFreeFunction free,
                    xrltTransformFunction transform)
{
    if (name == NULL || transform == NULL) { return FALSE; }
    if (!xrltRegisterBuiltinElementsIfUnregistered()) { return FALSE; }

    xrltElementPtr   elem;
    const xmlChar   *_toplevel = (const xmlChar *)(toplevel ? "yes" : "no");

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

    if (!xmlHashAddEntry3(xrltRegisteredElements, name, _toplevel, ns, elem)) {
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

    const xmlChar   *ns;
    const xmlChar   *name;
    const xmlChar   *toplevel;
    xrltElementPtr   elem;
    xmlNodePtr       tmp;
    void            *comp;
    size_t           num;

    toplevel = first != NULL && first->parent != NULL &&
               xmlStrEqual(first->parent->name, XRLT_ROOT_NAME) &&
               first->parent->ns != NULL &&
               xmlStrEqual(first->parent->ns->href, XRLT_NS)
        ?
        (xmlChar *)"yes"
        :
        (xmlChar *)"no";


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
            xrltRegisteredElements, name, toplevel, ns
        );

        printf("0000000 %s\n", (char *)first->name);

        if (elem == NULL) {
            first->_private = NULL;

            if (first->ns != NULL && xmlStrEqual(first->ns->href, XRLT_NS)) {
                // Don't allow unknown elements from XRLT namespace.
                xrltTransformError(NULL, sheet, first, "Unknown node\n");
                return FALSE;
            }

            if (first->children != NULL &&
                !xrltElementCompile(sheet, first->children))
            {
                return FALSE;
            }
        } else {
            if (elem->compile != NULL) {
                comp = elem->compile(sheet, first);
                if (comp == NULL) { return FALSE; }
            } else {
                comp = NULL;
            }

            num = xrltCompiledElementPush(
                sheet, elem->transform, elem->free, comp
            );

            printf("pushed %d\n", (int)num);

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
