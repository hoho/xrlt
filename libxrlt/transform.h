#ifndef __XRLT_TRANSFORM_H__
#define __XRLT_TRANSFORM_H__


#include <libxml/tree.h>
#include <libxml/hash.h>

#include "xrltstruct.h"
#include "xrlt.h"
#include "xrlterror.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    XRLT_PASS1 = 0,
    XRLT_PASS2,
    XRLT_COMPILED
} xrltCompilePass;


typedef struct _xrltTransformCallback xrltTransformCallback;
typedef xrltTransformCallback* xrltTransformCallbackPtr;
struct _xrltTransformCallback {
    xrltTransformFunction      func;    // Function to call.
    void                      *comp;    // Compiled element's data.
    xmlNodePtr                 insert;  // Place to insert the result.
    void                      *data;    // Data allocated by transform
                                        // function. These datas are stored in
                                        // the transformation context. They are
                                        // freed by free function of
                                        // xrltTransformingElement, when the
                                        // context is being freed.
    xrltTransformCallbackPtr   next;    // Next callback in this queue.
};


typedef struct {
    xrltTransformCallbackPtr   first;
    xrltTransformCallbackPtr   last;
} xrltTransformCallbackQueue;


typedef struct _xrltElement xrltElement;
typedef xrltElement* xrltElementPtr;
struct _xrltElement {
    xrltCompileFunction     compile;
    xrltFreeFunction        free;
    xrltTransformFunction   transform;
    size_t                  passes;
};


typedef struct _xrltCompiledElement xrltCompiledElement;
typedef xrltCompiledElement* xrltCompiledElementPtr;
struct _xrltCompiledElement {
    xrltTransformFunction   transform;
    xrltFreeFunction        free;
    void                   *data;
};


typedef struct _xrltTransformingElement xrltTransformingElement;
typedef xrltTransformingElement* xrltTransformingElementPtr;
struct _xrltTransformingElement {
    int                          count; // How many unfinished subelements.
    void                        *data;  // Some data allocated by transform
                                        // function.
    xrltFreeFunction             free;  // Free function to free *data.
    xrltTransformCallbackQueue   tcb;   // Callbacks to be called when
                                        // count == 0;
};


typedef struct {
    xrltCompilePass             pass;        // Indicates current compilation
                                             // pass.
    xrltCompiledElementPtr      comp;        // Array of compiled elements.
    size_t                      compLen;
    size_t                      compSize;

    xmlNodePtr                  response;    // Node to begin transformation
                                             // from.
    xmlHashTablePtr             funcs;       // Functions of this requestsheet.
    xmlHashTablePtr             transforms;  // Transformations of this
                                             // requestsheet.
} xrltRequestsheetPrivate;


typedef struct {
    xrltHeaderList               header;
    xmlDocPtr                    responseDoc;
    xrltTransformCallbackQueue   tcb;

    xrltTransformingElementPtr   tr;
    size_t                       trLen;
    size_t                       trSize;

    xmlNodePtr                   subdoc;
    size_t                       subdocLen;
    size_t                       subdocSize;
} xrltContextPrivate;


void
        xrltUnregisterBuiltinElements   (void);


static inline xrltBool
xrltTransformCallbackQueuePush(xrltContextPtr ctx,
                               xrltTransformCallbackQueue *tcb,
                               xrltTransformFunction func, void *comp,
                               xmlNodePtr insert, void *data)
{
    if (ctx == NULL || tcb == NULL || func == NULL || insert == NULL) {
        return FALSE;
    }

    xrltTransformCallbackPtr   item;

    item = xrltMalloc(sizeof(xrltTransformCallback));

    if (item == NULL) {
        xrltTransformError(ctx, NULL, NULL,
                           "xrltTransformCallbackQueuePush: Out of memory\n");
        return FALSE;
    }

    memset(item, 0, sizeof(xrltTransformCallback));

    item->func = func;
    item->insert = insert;
    item->comp = comp;
    item->data = data;

    if (tcb->first == NULL) {
        tcb->first = item;
        tcb->last = item;
    } else {
        tcb->last->next = item;
        tcb->last = item;
    }

    return TRUE;
}


static inline xrltBool
xrltTransformCallbackQueueShift(xrltContextPtr ctx,
                                xrltTransformCallbackQueue *tcb,
                                xrltTransformFunction *func, void **comp,
                                xmlNodePtr *insert, void **data)
{
    if (ctx == NULL || tcb == NULL) { return FALSE; }

    xrltTransformCallbackPtr   item = tcb->first;

    if (item == NULL) {
        return FALSE;
    }

    *func = item->func;
    *comp = item->comp;
    *insert = item->insert;
    *data = item->data;

    tcb->first = item->next;
    if (item->next == NULL) { tcb->last = NULL; }

    xrltFree(item);

    return TRUE;
}


static inline void
xrltTransformCallbackQueueClear(xrltContextPtr ctx,
                                xrltTransformCallbackQueue *tcb)
{
    xrltTransformFunction   func;
    void                   *comp;
    xmlNodePtr              insert;
    void                   *data;

    while (xrltTransformCallbackQueueShift(ctx, tcb, &func, &comp, &insert,
                                           &data));
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
xrltTransformingElementPush(xrltContextPtr ctx,
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

    priv->tr[priv->trLen].free = free;
    priv->tr[priv->trLen].data = data;

    return priv->trLen++;
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_TRANSFORM_H__ */
