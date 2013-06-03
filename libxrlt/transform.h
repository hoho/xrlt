/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_TRANSFORM_H__
#define __XRLT_TRANSFORM_H__


#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

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
#define XRLT_ELEMENT_ATTR_ASYNC     (const xmlChar *)"async"
#define XRLT_ELEMENT_ATTR_MAIN      (const xmlChar *)"main"
#define XRLT_ELEMENT_ATTR_SRC       (const xmlChar *)"src"
#define XRLT_ELEMENT_PARAM          (const xmlChar *)"param"
#define XRLT_ELEMENT_HREF           (const xmlChar *)"href"
#define XRLT_ELEMENT_METHOD         (const xmlChar *)"method"
#define XRLT_ELEMENT_WITH_HEADER    (const xmlChar *)"with-header"
#define XRLT_ELEMENT_WITH_COOKIE    (const xmlChar *)"with-cookie"
#define XRLT_ELEMENT_WITH_PARAM     (const xmlChar *)"with-param"
#define XRLT_ELEMENT_WITH_BODY      (const xmlChar *)"with-body"
#define XRLT_ELEMENT_SUCCESS        (const xmlChar *)"success"
#define XRLT_ELEMENT_FAILURE        (const xmlChar *)"failure"
#define XRLT_ELEMENT_TEST           (const xmlChar *)"test"
#define XRLT_ELEMENT_TYPE           (const xmlChar *)"type"
#define XRLT_ELEMENT_BODY           (const xmlChar *)"body"
#define XRLT_ELEMENT_NAME           (const xmlChar *)"name"
#define XRLT_ELEMENT_VALUE          (const xmlChar *)"value"
#define XRLT_ELEMENT_PATH           (const xmlChar *)"path"
#define XRLT_ELEMENT_EXPIRES        (const xmlChar *)"expires"
#define XRLT_ELEMENT_DOMAIN         (const xmlChar *)"domain"
#define XRLT_ELEMENT_SECURE         (const xmlChar *)"secure"
#define XRLT_ELEMENT_HTTPONLY       (const xmlChar *)"httponly"
#define XRLT_ELEMENT_PERMANENT      (const xmlChar *)"permanent"

#define XRLT_VALUE_YES              (const xmlChar *)"yes"
#define XRLT_VALUE_NO               (const xmlChar *)"no"


#define ERROR_OUT_OF_MEMORY(ctx, sheet, node)                                 \
    xrltTransformError(ctx, sheet, node, "%s: Out of memory\n", __func__);

#define ERROR_CREATE_NODE(ctx, sheet, node)                                   \
    xrltTransformError(ctx, sheet, node,                                      \
                       "%s: Failed to create element\n", __func__);

#define ERROR_ADD_NODE(ctx, sheet, node)                                      \
    xrltTransformError(ctx, sheet, node,                                      \
                       "%s: Failed to add element\n", __func__);

#define ERROR_UNEXPECTED_ELEMENT(ctx, sheet, node)                            \
    xrltTransformError(ctx, sheet, node,                                      \
                       "%s: Unexpected element\n", __func__);


#define XRLT_MALLOC(ctx, sheet, node, ret, type, size, error) {               \
    ret = (type)xmlMalloc(size);                                              \
    if (ret == NULL) {                                                        \
        ERROR_OUT_OF_MEMORY(ctx, sheet, node);                                \
        return error;                                                         \
    }                                                                         \
    memset(ret, 0, size);                                                     \
}


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


#define REPLACE_RESPONSE_NODE(ctx, node, children, src) {                     \
    xmlNodePtr   n1 = node;                                                   \
    xmlNodePtr   n2 = children;                                               \
    xmlNodePtr   n3;                                                          \
    while (n2 != NULL) {                                                      \
        n3 = n2->next;                                                        \
        n1 = xmlAddNextSibling(n1, n2);                                       \
        if (n1 == NULL) {                                                     \
            ERROR_ADD_NODE(ctx, NULL, src);                                   \
            return FALSE;                                                     \
        }                                                                     \
        n2 = n3;                                                              \
    }                                                                         \
    REMOVE_RESPONSE_NODE(ctx, node);                                          \
}


#define SCHEDULE_CALLBACK(ctx, tcb, func, comp, insert, data) {               \
    if (!xrltTransformCallbackQueuePush(tcb, func, comp, insert,              \
                                        ctx->varScope, data))                 \
    {                                                                         \
        xrltTransformError(ctx, NULL, NULL, "Failed to push callback\n");     \
        return FALSE;                                                         \
    }                                                                         \
}


#define COUNTER_INCREASE(ctx, node) {                                         \
    if (!xrltNotReadyCounterIncrease(ctx, node)) { return FALSE; }            \
}


#define COUNTER_DECREASE(ctx, node) {                                         \
    if (!xrltNotReadyCounterDecrease(ctx, node)) { return FALSE; }            \
}


#define NEW_CHILD(ctx, ret, parent, name) {                                   \
    ret = xmlNewChild(parent, NULL, (const xmlChar *)name, NULL);             \
    if (ret == NULL) {                                                        \
        ERROR_CREATE_NODE(ctx, NULL, NULL);                                   \
        return FALSE;                                                         \
    }                                                                         \
}


#define NEW_CHILD_GOTO(ctx, ret, parent, name) {                              \
    ret = xmlNewChild(parent, NULL, (const xmlChar *)name, NULL);             \
    if (ret == NULL) {                                                        \
        ERROR_CREATE_NODE(ctx, NULL, NULL);                                   \
        goto error;                                                           \
    }                                                                         \
}


#define TRANSFORM_SUBTREE(ctx, first, insert) {                               \
    if (!xrltElementTransform(ctx, first, insert)) { return FALSE; }          \
}


#define TRANSFORM_SUBTREE_GOTO(ctx, first, insert) {                          \
    if (!xrltElementTransform(ctx, first, insert)) { goto error; }            \
}


#define TRANSFORM_TO_STRING(ctx, node, val, ret) {                            \
    if ((val)->type != XRLT_VALUE_EMPTY) {                                    \
        if (!xrltTransformToString(ctx, node, val, ret)) {                    \
            return FALSE;                                                     \
        }                                                                     \
    }                                                                         \
}


#define TRANSFORM_TO_BOOLEAN(ctx, node, val, ret) {                           \
    if ((val)->type != XRLT_VALUE_EMPTY) {                                    \
        if (!xrltTransformToBoolean(ctx, node, val, ret)) {                   \
            return FALSE;                                                     \
        }                                                                     \
    }                                                                         \
}


#define TRANSFORM_XPATH_TO_NODE(ctx, expr, insert) {                          \
    if (!xrltTransformByXPath(ctx, expr, insert, NULL)) {                     \
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
    int                          count;      // Ready flag for response doc
                                             // nodes, number of compile passes
                                             // otherwise.
    xrltTransformFunction        transform;  // Unused for response doc nodes.
    xrltFreeFunction             free;       // Data free function.
    void                        *data;
    xrltTransformCallbackQueue   tcb;        // Callbacks to be called when
                                             // count == 0, only for response
                                             // doc nodes.
    xmlDocPtr                    root;       // Root for XPath requests.
    void                        *sr;         // Subrequest data, to get headers
                                             // from.
};


void
        xrltUnregisterBuiltinElements   (void);
xrltBool
        xrltHasXRLTElement              (xmlNodePtr node);
xmlXPathObjectPtr
        xrltVariableLookupFunc          (void *ctxt, const xmlChar *name,
                                         const xmlChar *ns_uri);


static inline xrltBool
xrltTransformCallbackQueuePush(xrltTransformCallbackQueue *tcb,
                               xrltTransformFunction func, void *comp,
                               xmlNodePtr insert, size_t varScope, void *data)
{
    if (tcb == NULL || func == NULL) {
        return FALSE;
    }

    xrltTransformCallbackPtr   item;

    XRLT_MALLOC(NULL, NULL, NULL, item, xrltTransformCallbackPtr,
                sizeof(xrltTransformCallback), FALSE);

    item->func = func;
    item->insert = insert;
    item->comp = comp;
    item->varScope = varScope;
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
                                xmlNodePtr *insert, size_t *varScope,
                                void **data)
{
    if (tcb == NULL) { return FALSE; }

    xrltTransformCallbackPtr   item = tcb->first;

    if (item == NULL) {
        return FALSE;
    }

    *func = item->func;
    *comp = item->comp;
    *insert = item->insert;
    *varScope = item->varScope;
    *data = item->data;

    tcb->first = item->next;
    if (item->next == NULL) { tcb->last = NULL; }

    xmlFree(item);

    return TRUE;
}


static inline void
xrltTransformCallbackQueueClear(xrltTransformCallbackQueue *tcb)
{
    xrltTransformFunction   func;
    void                   *comp;
    xmlNodePtr              insert;
    size_t                  varScope;
    void                   *data;

    while (xrltTransformCallbackQueueShift(tcb, &func, &comp, &insert,
                                           &varScope, &data));
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

        if (n->count == 0 && n->tcb.first != NULL) {
            // Node has become ready. Move all the node's callbacks to the
            // main queue.
            if (ctx->tcb.first == NULL) {
                ctx->tcb.first = n->tcb.first;
            } else {
                ctx->tcb.last->next = n->tcb.first;
            }
            ctx->tcb.last = n->tcb.last;

            n->tcb.first = NULL;
            n->tcb.last = NULL;
        }

        node = node->parent;
    }

    return TRUE;
}


static inline xrltBool
xrltCompileCheckSubnodes(xrltRequestsheetPtr sheet, xmlNodePtr node,
                         const xmlChar *name1, const xmlChar *name2,
                         const xmlChar *name3, const xmlChar *name4,
                         const xmlChar *name5, const xmlChar *name6,
                         const xmlChar *name7, const xmlChar *name8,
                         xrltBool *hasxrlt)
{
    xmlNodePtr   tmp;
    xrltBool     xrlt = FALSE;
    xrltBool     other = FALSE;

    tmp = node->children;

    while (tmp != NULL) {
        if (tmp->ns != NULL && xmlStrEqual(tmp->ns->href, XRLT_NS) &&
            ((name1 != NULL && xmlStrEqual(name1, tmp->name)) ||
             (name2 != NULL && xmlStrEqual(name2, tmp->name)) ||
             (name3 != NULL && xmlStrEqual(name3, tmp->name)) ||
             (name4 != NULL && xmlStrEqual(name4, tmp->name)) ||
             (name5 != NULL && xmlStrEqual(name5, tmp->name)) ||
             (name6 != NULL && xmlStrEqual(name6, tmp->name)) ||
             (name7 != NULL && xmlStrEqual(name7, tmp->name)) ||
             (name8 != NULL && xmlStrEqual(name8, tmp->name))))
        {
            xrlt = TRUE;
        } else {
            other = TRUE;
        }

        if (xrlt && other) {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
            return FALSE;
        }

        tmp = tmp->next;
    }

    *hasxrlt = xrlt;

    return TRUE;
}


static inline xrltBool
xrltCompileValue(xrltRequestsheetPtr sheet, xmlNodePtr node,
                 xmlNodePtr nodeVal, const xmlChar *xpathAttrName,
                 const xmlChar *valAttrName, const xmlChar *valNodeName,
                 xrltBool tostr, xrltCompiledValue *val)
{
    xmlChar          *sel, *vsel, *v;
    xmlNodePtr        tmp, tmp2, vNode = NULL;
    xrltNodeDataPtr   n;
    xrltBool          ret = FALSE;

    if (valNodeName != NULL) {
        tmp = node->children;

        while (tmp != NULL) {
            if (tmp->ns != NULL && xmlStrEqual(tmp->ns->href, XRLT_NS) &&
                xmlStrEqual(tmp->name, valNodeName))
            {
                if (vNode == NULL) {
                    vNode = tmp;
                } else {
                    ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
                    return FALSE;
                }
            }

            tmp = tmp->next;
        }
    }

    sel = xpathAttrName == NULL ? NULL : xmlGetProp(node, xpathAttrName);
    vsel = vNode == NULL ? NULL : xmlGetProp(vNode, XRLT_ELEMENT_ATTR_SELECT);
    v = valAttrName == NULL ? NULL : xmlGetProp(node, valAttrName);
    tmp = vNode == NULL ? nodeVal : vNode->children;

    if (sel != NULL && vNode != NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Either '%s' attribute or '%s' element is allowed\n",
                           xpathAttrName, valNodeName);
        goto error;
    }

    if (v != NULL && vNode != NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Either '%s' attribute or '%s' element is allowed\n",
                           valAttrName, valNodeName);
        goto error;
    }

    if (sel != NULL && v != NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Either '%s' attribute or '%s' attribute is "
                           "allowed\n", valAttrName, xpathAttrName);
        goto error;
    }

    if (sel != NULL && tmp != NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Either '%s' attribute or content is allowed\n",
                            xpathAttrName);
        goto error;
    }

    if (v != NULL && tmp != NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Either '%s' attribute or content is allowed\n",
                           valAttrName);
        goto error;
    }

    if (vsel != NULL && tmp != NULL) {
        xrltTransformError(NULL, sheet, vNode,
                           "Either '%s' attribute or content is allowed\n",
                           XRLT_ELEMENT_ATTR_SELECT);
        goto error;
    }

    memset(val, 0, sizeof(xrltCompiledValue));

    if (sel != NULL) {
        val->xpathval.expr = xmlXPathCompile(sel);
        if (val->xpathval.expr == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile '%s' expression\n",
                               xpathAttrName);
            goto error;
        }

        val->type = XRLT_VALUE_XPATH;
        val->xpathval.src = node;
        val->xpathval.scope = node->parent;
    } else if (vsel != NULL) {
        val->xpathval.expr = xmlXPathCompile(vsel);
        if (val->xpathval.expr == NULL) {
            xrltTransformError(NULL, sheet, vNode,
                               "Failed to compile '%s' expression\n",
                               XRLT_ELEMENT_ATTR_SELECT);
            goto error;
        }

        val->type = XRLT_VALUE_XPATH;
        val->xpathval.src = vNode;
        val->xpathval.scope = node->parent;
    } else if (v != NULL) {
        val->type = XRLT_VALUE_TEXT;
        val->textval = v;
        v = NULL;
    } else if (tmp != NULL) {
        tmp2 = tmp;

        while (tmp2 != NULL) {
            if (tmp2->type == XML_TEXT_NODE ||
                tmp2->type == XML_CDATA_SECTION_NODE)
            {
                tmp2 = tmp2->next;
            } else {
                break;
            }
        }

        if (tmp2 == NULL || (tostr && !xrltHasXRLTElement(tmp))) {
            val->type = XRLT_VALUE_TEXT;
            val->textval = xmlXPathCastNodeToString(tmp->parent);
        } else {
            val->type = XRLT_VALUE_NODELIST;
            val->nodeval = tmp;
        }
    }

    if (vNode != NULL) {
        ASSERT_NODE_DATA_GOTO(vNode, n);
        n->xrlt = TRUE;
    }

    ret = TRUE;

  error:
    if (sel != NULL) { xmlFree(sel); }
    if (vsel != NULL) { xmlFree(vsel); }
    if (v != NULL) { xmlFree(v); }

    return ret;
}


xrltBool
    xrltSetStringResult           (xrltContextPtr ctx, void *comp,
                                   xmlNodePtr insert, void *data);
xrltBool
    xrltSetStringResultByXPath    (xrltContextPtr ctx, void *comp,
                                   xmlNodePtr insert, void *data);
xrltBool
    xrltSetBooleanResult          (xrltContextPtr ctx, void *comp,
                                   xmlNodePtr insert, void *data);
xrltBool
    xrltSetBooleanResultByXPath   (xrltContextPtr ctx, void *comp,
                                   xmlNodePtr insert, void *data);


static inline xrltBool
xrltTransformToString(xrltContextPtr ctx, xmlNodePtr insert,
                      xrltCompiledValue *val, xmlChar **ret)
{
    switch (val->type) {
        case XRLT_VALUE_TEXT:
            *ret = xmlStrdup(val->textval);

            break;

        case XRLT_VALUE_NODELIST:
            {
                xmlNodePtr   node;

                NEW_CHILD(ctx, node, insert, "tmp");

                TRANSFORM_SUBTREE(ctx, val->nodeval, node);

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltSetStringResult, node, insert, ret
                );
            }

            break;

        case XRLT_VALUE_XPATH:
            COUNTER_INCREASE(ctx, insert);

            return xrltSetStringResultByXPath(ctx, &val->xpathval, insert, ret);

        case XRLT_VALUE_EMPTY:
        case XRLT_VALUE_INT:
            *ret = NULL;

            break;
    }

    return TRUE;
}


static inline xrltBool
xrltTransformToBoolean(xrltContextPtr ctx, xmlNodePtr insert,
                       xrltCompiledValue *val, xrltBool *ret)
{
    switch (val->type) {
        case XRLT_VALUE_TEXT:
            *ret = xmlXPathCastStringToBoolean(val->textval);

            break;

        case XRLT_VALUE_NODELIST:
            {
                xmlNodePtr   node;

                NEW_CHILD(ctx, node, insert, "tmp");

                TRANSFORM_SUBTREE(ctx, val->nodeval, node);

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltSetBooleanResult, node, insert, ret
                );
            }
            break;

        case XRLT_VALUE_XPATH:
            COUNTER_INCREASE(ctx, insert);

            return xrltSetBooleanResultByXPath(ctx, &val->xpathval, insert,
                                               ret);

        case XRLT_VALUE_INT:
            *ret = val->intval ? TRUE : FALSE;

            break;

        case XRLT_VALUE_EMPTY:
            *ret = FALSE;

            break;

    }

    return TRUE;
}


static inline xrltBool
xrltTransformByXPath(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                     void *data)
{
    xrltXPathExpr                *expr = (xrltXPathExpr *)comp;
    xmlXPathObjectPtr             v;
    int                           i;
    xmlChar                      *s = NULL;
    xmlNodePtr                    node;
    xmlNodeSetPtr                 ns = NULL;
    xrltBool                      ret = TRUE;

    if (!xrltXPathEval(ctx, insert, expr, &v)) {
        return FALSE;
    }

    if (v == NULL) {
        // Some variables are not ready.
        xrltNodeDataPtr   n;

        ASSERT_NODE_DATA(ctx->xpathWait, n);

        if (data == NULL) {
            COUNTER_INCREASE(ctx, insert);
        }

        SCHEDULE_CALLBACK(
            ctx, &n->tcb, xrltTransformByXPath, comp, insert, (void *)0x1
        );
    } else {
        switch (v->type) {
            case XPATH_NODESET:
                if (!xmlXPathNodeSetIsEmpty(v->nodesetval)) {
                    ns = xmlXPathNodeSetCreate(NULL);
                    if (ns == NULL) {
                        xrltTransformError(ctx, NULL, expr->src,
                                           "Failed to create node-set\n");
                        ret = FALSE;
                        goto error;
                    }

                    for (i = 0; i < v->nodesetval->nodeNr; i++) {
                        node = v->nodesetval->nodeTab[i];

                        if (node != NULL) {
                            if (node->type == XML_DOCUMENT_NODE) {
                                node = node->children;

                                while (node != NULL) {
                                    xmlXPathNodeSetAdd(ns, node);
                                    node = node->next;
                                }
                            } else {
                                xmlXPathNodeSetAdd(ns, node);
                            }
                        }
                    }

                    for (i = 0; i < ns->nodeNr; i++) {
                        node = xmlDocCopyNode(ns->nodeTab[i], insert->doc, 1);

                        if (node == NULL) {
                            ERROR_CREATE_NODE(ctx, NULL, expr->src);
                            ret = FALSE;
                            goto error;
                        }

                        if (xmlAddChild(insert, node) == NULL) {
                            ERROR_ADD_NODE(ctx, NULL, expr->src);
                            xmlFreeNode(node);
                            ret = FALSE;
                            goto error;
                        }
                    }
                }

                break;

            case XPATH_BOOLEAN:
            case XPATH_NUMBER:
            case XPATH_STRING:
                s = xmlXPathCastToString(v);

                if (s == NULL) {
                    xrltTransformError(ctx, NULL, expr->src,
                                       "Failed to cast result to string\n");
                    ret = FALSE;
                    goto error;
                }

                node = xmlNewText(s);

                if (node == NULL) {
                    ERROR_CREATE_NODE(ctx, NULL, expr->src);
                    ret = FALSE;
                    goto error;
                }

                if (xmlAddChild(insert, node) == NULL) {
                    ERROR_ADD_NODE(ctx, NULL, expr->src);
                    xmlFreeNode(node);
                    ret = FALSE;
                    goto error;
                }

                break;

            case XPATH_UNDEFINED:
            case XPATH_POINT:
            case XPATH_RANGE:
            case XPATH_LOCATIONSET:
            case XPATH_USERS:
            case XPATH_XSLT_TREE:
                break;
        }

        if (data != NULL) {
            COUNTER_DECREASE(ctx, insert);
        }
    }

  error:
    if (v != NULL) { xmlXPathFreeObject(v); }
    if (ns != NULL) { xmlXPathFreeNodeSet(ns); }
    if (s != NULL) { xmlFree(s); }

    return ret;
}


static inline xrltBool
xrltTransformByCompiledValue(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                             void *data)
{
    xrltCompiledValue  *val = (xrltCompiledValue *)comp;
    xmlNodePtr          node;

    if (data == NULL) {
        switch (val->type) {
            case XRLT_VALUE_NODELIST:
                COUNTER_INCREASE(ctx, insert);

                TRANSFORM_SUBTREE(ctx, val->nodeval, insert);

                SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltTransformByCompiledValue,
                                  comp, insert, (void *)0x1);

                break;

            case XRLT_VALUE_XPATH:
                TRANSFORM_XPATH_TO_NODE(ctx, &val->xpathval, insert);

                break;

            case XRLT_VALUE_TEXT:
                node = xmlNewText(val->textval);

                if (node == NULL) {
                    ERROR_CREATE_NODE(ctx, NULL, NULL);

                    return FALSE;
                }

                if (xmlAddChild(insert, node) == NULL) {
                    ERROR_ADD_NODE(ctx, NULL, NULL);

                    return FALSE;
                }

                break;

            case XRLT_VALUE_EMPTY:
            case XRLT_VALUE_INT:
                break;
        }
    } else {
        COUNTER_DECREASE(ctx, insert);
    }

    return TRUE;
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_TRANSFORM_H__ */
