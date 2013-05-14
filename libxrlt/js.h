/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_JS_H__
#define __XRLT_JS_H__


#include <libxml/xpath.h>

#include <xrltstruct.h>
#include "variable.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltJSContext xrltJSContext;
typedef xrltJSContext* xrltJSContextPtr;
struct _xrltJSContext {
    void *_private;
};


typedef struct _xrltJSArgument xrltJSArgument;
typedef xrltJSArgument* xrltJSArgumentPtr;
struct _xrltJSArgument {
    char               *name;
    xmlXPathObjectPtr   val;
};


typedef struct _xrltJSArgumentList xrltJSArgumentList;
typedef xrltJSArgumentList* xrltJSArgumentListPtr;
struct _xrltJSArgumentList {
    xrltJSArgumentPtr   arg;
    int                 size;
    int                 len;
};


typedef struct _xrltDeferredTransforming xrltDeferredTransforming;
typedef xrltDeferredTransforming* xrltDeferredTransformingPtr;
struct _xrltDeferredTransforming {
    xmlNodePtr   node;
    xmlNodePtr   codeNode;
    xmlChar     *name;
    xmlNodePtr   varContext;
    void        *deferred;
};


static inline xrltJSArgumentListPtr
        xrltJSArgumentListCreate   (int size);
static inline xrltBool
        xrltJSArgumentListPush     (xrltJSArgumentListPtr list, char *name,
                                    xmlXPathObjectPtr val);
static inline void
        xrltJSArgumentListFree     (xrltJSArgumentListPtr list);

void
        xrltJSInit                 (void);
void
        xrltJSFree                 (void);

xrltJSContextPtr
        xrltJSContextCreate        (void);
void
        xrltJSContextFree          (xrltJSContextPtr jsctx);
xrltBool
        xrltJSFunction             (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                    xmlChar *name, xrltVariableDataPtr *param,
                                    size_t paramLen, const xmlChar *code);
xrltBool
        xrltJSApply                (xrltContextPtr ctx, xmlNodePtr node,
                                    xmlChar *name, xrltVariableDataPtr *param,
                                    size_t paramLen, xmlNodePtr insert);
xrltBool
        xrltJSCallback             (xrltJSContextPtr jsctx, void *callback,
                                    xrltJSArgumentListPtr args);


static inline xrltJSArgumentListPtr
xrltJSArgumentListCreate(int size)
{
    xrltJSArgumentListPtr   ret;
    ret = (xrltJSArgumentListPtr)xrltMalloc(sizeof(xrltJSArgumentList) +
                                            sizeof(xrltJSArgument) * size);

    if (ret == NULL) { return NULL; }

    memset(ret, 0, sizeof(xrltJSArgumentList) + sizeof(xrltJSArgument) * size);

    ret->size = size;
    ret->arg = (xrltJSArgumentPtr)(ret + 1);

    return ret;
}


static inline xrltBool
xrltJSArgumentListPush(xrltJSArgumentListPtr list, char *name,
                       xmlXPathObjectPtr val)
{
    if (list == NULL || name == NULL || list->len >= list->size) {
        return FALSE;
    }

    xrltJSArgumentPtr   a = &list->arg[list->len++];

    a->name = strdup(name);

    if (a->name == NULL) {
        list->len--;
        return FALSE;
    }

    a->val = val;

    return TRUE;
}


static inline void
xrltJSArgumentListFree(xrltJSArgumentListPtr list)
{
    if (list == NULL) { return; }

    int   i;

    for (i = list->len - 1; i >= 0; i--) {
        if (list->arg[i].name != NULL) {
            xrltFree(list->arg[i].name);
        }
    }

    xrltFree(list);
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_JS_H__ */
