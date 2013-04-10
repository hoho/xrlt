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
#define XRLT_ELEMENT_ATTR_TYPE      (const xmlChar *)"type"
#define XRLT_ELEMENT_PARAM          (const xmlChar *)"param"
#define XRLT_ELEMENT_HREF           (const xmlChar *)"href"
#define XRLT_ELEMENT_WITH_HEADER    (const xmlChar *)"with-header"
#define XRLT_ELEMENT_WITH_PARAM     (const xmlChar *)"with-param"
#define XRLT_ELEMENT_WITH_BODY      (const xmlChar *)"with-body"
#define XRLT_ELEMENT_SUCCESS        (const xmlChar *)"success"
#define XRLT_ELEMENT_FAILURE        (const xmlChar *)"failure"
#define XRLT_ELEMENT_TEST           (const xmlChar *)"test"
#define XRLT_ELEMENT_TYPE           (const xmlChar *)"type"
#define XRLT_ELEMENT_NAME           (const xmlChar *)"name"
#define XRLT_ELEMENT_VALUE          (const xmlChar *)"value"


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


#define REMOVE_RESPONSE_NODE(ctx, node) {                                     \
    if (ctx->responseCur == node) {                                           \
        ctx->responseCur = ctx->responseCur->next;                            \
    }                                                                         \
    xmlUnlinkNode(node);                                                      \
    xmlFreeNode(node);                                                        \
}


#define SCHEDULE_CALLBACK(ctx, tcb, func, comp, insert, data) {               \
    if (!xrltTransformCallbackQueuePush(tcb, func, comp, insert, data)) {     \
        xrltTransformError(ctx, NULL, NULL, "Failed to push callback\n");     \
        return FALSE;                                                         \
    }                                                                         \
}


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
        ASSERT_NODE_DATA(node, n);

        n->count++;

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
        ASSERT_NODE_DATA(node, n);

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
                SCHEDULE_CALLBACK(ctx, &ctx->tcb, func, comp, insert, data);
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
