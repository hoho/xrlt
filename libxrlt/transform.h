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


#define XRLT_ELEMENT_ATTR_TEST      (const xmlChar *)"test"
#define XRLT_ELEMENT_ATTR_NAME      (const xmlChar *)"name"
#define XRLT_ELEMENT_ATTR_SELECT    (const xmlChar *)"select"
#define XRLT_ELEMENT_PARAM          (const xmlChar *)"param"
#define XRLT_ELEMENT_WITH_PARAM     (const xmlChar *)"with-param"


#define ASSERT_NODE_DATA(node, var) {                                         \
    var = (xrltNodeDataPtr)node->_private;                                    \
    if (var == NULL) {                                                        \
        xrltTransformError(NULL, NULL, NULL, "Element has no data\n");        \
        return FALSE;                                                         \
    }                                                                         \
}


#define ASSERT_NODE_DATA_GOTO(node, var) {                                    \
    var = (xrltNodeDataPtr)node->_private;                                    \
    if (var == NULL) {                                                        \
        xrltTransformError(NULL, NULL, NULL, "Element has no data\n");        \
        goto error;                                                           \
    }                                                                         \
}


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


typedef struct _xrltNodeData xrltNodeData;
typedef xrltNodeData* xrltNodeDataPtr;
struct _xrltNodeData {
    xrltBool                     xrlt;       // Indicates XRLT element.
    xrltBool                     response;   // Indicates that it's a node of
                                             // the response doc.
    xrltBool                     skip;       // Indicates that the node
                                             // shouldn't go to response.
    int                          count;      // Ready flag for response doc
                                             // nodes, number of compile passes
                                             // otherwise.
    xrltTransformFunction        transform;  // Unused for response doc nodes.
    xrltFreeFunction             free;       // Data free function.
    void                        *data;
    xrltTransformCallbackQueue   tcb;        // Callbacks to be called when
                                             // count == 0, only for response
                                             // doc nodes.
};


typedef struct {
    xrltCompilePass             pass;        // Indicates current compilation
                                             // pass.
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

    xmlNodePtr                   subdoc;
    size_t                       subdocLen;
    size_t                       subdocSize;
} xrltContextPrivate;


void
        xrltUnregisterBuiltinElements   (void);
xrltBool
        xrltHasXRLTElement(xmlNodePtr node);


static inline xrltBool
xrltTransformCallbackQueuePush(xrltTransformCallbackQueue *tcb,
                               xrltTransformFunction func, void *comp,
                               xmlNodePtr insert, void *data)
{
    if (tcb == NULL || func == NULL || insert == NULL) {
        return FALSE;
    }

    xrltTransformCallbackPtr   item;

    item = xrltMalloc(sizeof(xrltTransformCallback));

    if (item == NULL) {
        xrltTransformError(NULL, NULL, NULL,
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
    } else {
        tcb->last->next = item;
    }
    tcb->last = item;

    return TRUE;
}


static inline xrltBool
xrltTransformCallbackQueueShift(xrltTransformCallbackQueue *tcb,
                                xrltTransformFunction *func, void **comp,
                                xmlNodePtr *insert, void **data)
{
    if (tcb == NULL) { return FALSE; }

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
xrltTransformCallbackQueueClear(xrltTransformCallbackQueue *tcb)
{
    xrltTransformFunction   func;
    void                   *comp;
    xmlNodePtr              insert;
    void                   *data;

    while (xrltTransformCallbackQueueShift(tcb, &func, &comp, &insert, &data));
}


inline static xrltBool
xrltNotReadyCounterIncrease(xrltContextPtr ctx, xmlNodePtr node)
{
    if (ctx == NULL || node == NULL) { return FALSE; }

    xrltNodeDataPtr n;

    while (node != NULL) {
        n = (xrltNodeDataPtr)node->_private;

        if (n != NULL) {
            n->count++;
        }

        node = node->parent;
    }

    return TRUE;
}


inline static xrltBool
xrltNotReadyCounterDecrease(xrltContextPtr ctx, xmlNodePtr node)
{
    if (ctx == NULL || node == NULL) { return FALSE; }

    xrltNodeDataPtr n;

    while (node != NULL) {
        n = (xrltNodeDataPtr)node->_private;

        if (n != NULL) {
            if (n->count == 0) {
                xrltTransformError(ctx, NULL, NULL, "Can't decrease counter\n");
                return FALSE;
            }

            n->count--;

            if (n->count == 0) {
                // Node has become ready. Call all the callbacks for this node.
                xrltTransformFunction     func;
                void                     *comp;
                xmlNodePtr                insert;
                void                     *data;

                while (xrltTransformCallbackQueueShift(&n->tcb, &func, &comp,
                                                       &insert, &data))
                {
                    if (!func(ctx, comp, insert, data)) {
                        return FALSE;
                    }
                }
            }
        }

        node = node->parent;
    }

    return TRUE;
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_TRANSFORM_H__ */
