#ifndef __XRLT_TRANSFORM_H__
#define __XRLT_TRANSFORM_H__


#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xpath.h>

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
#define XRLT_ELEMENT_METHOD         (const xmlChar *)"method"
#define XRLT_ELEMENT_WITH_HEADER    (const xmlChar *)"with-header"
#define XRLT_ELEMENT_WITH_PARAM     (const xmlChar *)"with-param"
#define XRLT_ELEMENT_WITH_BODY      (const xmlChar *)"with-body"
#define XRLT_ELEMENT_SUCCESS        (const xmlChar *)"success"
#define XRLT_ELEMENT_FAILURE        (const xmlChar *)"failure"
#define XRLT_ELEMENT_TEST           (const xmlChar *)"test"
#define XRLT_ELEMENT_TYPE           (const xmlChar *)"type"
#define XRLT_ELEMENT_NAME           (const xmlChar *)"name"
#define XRLT_ELEMENT_VALUE          (const xmlChar *)"value"


#define XRLT_TESTNAMEVALUE_TEST_ATTR                2
#define XRLT_TESTNAMEVALUE_TEST_NODE                4
#define XRLT_TESTNAMEVALUE_TEST_REQUIRED            8
#define XRLT_TESTNAMEVALUE_TYPE_ATTR                16
#define XRLT_TESTNAMEVALUE_TYPE_NODE                32
#define XRLT_TESTNAMEVALUE_TYPE_REQUIRED            64
#define XRLT_TESTNAMEVALUE_NAME_ATTR                128
#define XRLT_TESTNAMEVALUE_NAME_NODE                256
#define XRLT_TESTNAMEVALUE_NAME_REQUIRED            512
#define XRLT_TESTNAMEVALUE_VALUE_ATTR               1024
#define XRLT_TESTNAMEVALUE_VALUE_NODE               2048
#define XRLT_TESTNAMEVALUE_VALUE_REQUIRED           4096
#define XRLT_TESTNAMEVALUE_TO_STRING                8192
#define XRLT_TESTNAMEVALUE_NAME_OR_VALUE_REQUIRED   16384


#define RAISE_OUT_OF_MEMORY(ctx, sheet, node)                                 \
    xrltTransformError(ctx, sheet, node, "%s: Out of memory\n", __func__);


#define XRLT_MALLOC(ctx, sheet, node, ret, type, size, error) {               \
    ret = (type)xrltMalloc(size);                                             \
    if (ret == NULL) {                                                        \
        RAISE_OUT_OF_MEMORY(ctx, sheet, node);                                \
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


#define SCHEDULE_CALLBACK(ctx, tcb, func, comp, insert, data) {               \
    if (!xrltTransformCallbackQueuePush(tcb, func, comp, insert, data)) {     \
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
        xrltTransformError(ctx, NULL, NULL,                                   \
                           "Failed to create element\n");                     \
        return FALSE;                                                         \
    }                                                                         \
}


#define NEW_CHILD_GOTO(ctx, ret, parent, name) {                              \
    ret = xmlNewChild(parent, NULL, (const xmlChar *)name, NULL);             \
    if (ret == NULL) {                                                        \
        xrltTransformError(ctx, NULL, NULL,                                   \
                           "Failed to create element\n");                     \
        goto error;                                                           \
    }                                                                         \
}


#define TRANSFORM_SUBTREE(ctx, first, insert) {                               \
    if (!xrltElementTransform(ctx, first, insert)) { return FALSE; }          \
}

#define TRANSFORM_SUBTREE_GOTO(ctx, first, insert) {                          \
    if (!xrltElementTransform(ctx, first, insert)) { goto error; }            \
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
    if (tcb == NULL || func == NULL) {
        return FALSE;
    }

    xrltTransformCallbackPtr   item;

    XRLT_MALLOC(NULL, NULL, NULL, item, xrltTransformCallbackPtr,
                sizeof(xrltTransformCallback), FALSE);

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
xrltCompileTestNameValueNode(xrltRequestsheetPtr sheet, xmlNodePtr node,
                             int conf, int *test, xmlNodePtr *ntest,
                             xmlXPathCompExprPtr *xtest, xmlChar **type,
                             xmlNodePtr *ntype, xmlXPathCompExprPtr *xtype,
                             xmlChar **name, xmlNodePtr *nname,
                             xmlXPathCompExprPtr *xname, xmlChar **val,
                             xmlNodePtr *nval, xmlXPathCompExprPtr *xval)
{
    xmlNodePtr        _ntest = NULL;
    xmlNodePtr        _ntype = NULL;
    xmlNodePtr        _nname = NULL;
    xmlNodePtr        _nval = NULL;
    xmlNodePtr        tmp, tmp2;
    xmlNodePtr        other = NULL;
    xmlNodePtr        dup = NULL;
    xmlChar          *_test = NULL;
    xmlChar          *_type = NULL;
    xmlChar          *_name = NULL;
    xmlChar          *_val = NULL;
    xmlChar          *_etype = NULL;
    xmlChar          *_ename = NULL;
    xmlChar          *_eval = NULL;
    xrltNodeDataPtr   n;

    if ((conf & XRLT_TESTNAMEVALUE_TEST_ATTR) |
        (conf & XRLT_TESTNAMEVALUE_TEST_NODE))
    {
        *test = 0;
        *ntest = NULL;
        *xtest = NULL;
    }

    if ((conf & XRLT_TESTNAMEVALUE_TYPE_ATTR) |
        (conf & XRLT_TESTNAMEVALUE_TYPE_NODE))
    {
        *type = NULL;
        *ntype = NULL;
        *xtype = NULL;
    }

    if ((conf & XRLT_TESTNAMEVALUE_NAME_ATTR) |
        (conf & XRLT_TESTNAMEVALUE_NAME_NODE))
    {
        *name = NULL;
        *nname = NULL;
        *xname = NULL;
    }

    if ((conf & XRLT_TESTNAMEVALUE_VALUE_ATTR) |
        (conf & XRLT_TESTNAMEVALUE_VALUE_NODE))
    {
        *val = NULL;
        *nval = NULL;
        *xval = NULL;
    }

    tmp = node->children;
    tmp2 = NULL;

    while (tmp != NULL) {
        if (tmp->ns != NULL && xmlStrEqual(tmp->ns->href, XRLT_NS)) {
            if ((conf & XRLT_TESTNAMEVALUE_TEST_NODE) &&
                xmlStrEqual(tmp->name, XRLT_ELEMENT_TEST))
            {
                if (_ntest != NULL) {
                    dup = tmp;
                    break;
                }
                if (tmp2 == NULL) { tmp2 = tmp; }
                _ntest = tmp;
            } else if ((conf & XRLT_TESTNAMEVALUE_TYPE_NODE) &&
                       xmlStrEqual(tmp->name, XRLT_ELEMENT_TYPE))
            {
                if (_ntype != NULL) {
                    dup = tmp;
                    break;
                }
                if (tmp2 == NULL) { tmp2 = tmp; }
                _ntype = tmp;
            } else if ((conf & XRLT_TESTNAMEVALUE_NAME_NODE) &&
                       xmlStrEqual(tmp->name, XRLT_ELEMENT_NAME))
            {
                if (_nname != NULL) {
                    dup = tmp;
                    break;
                }
                if (tmp2 == NULL) { tmp2 = tmp; }
                _nname = tmp;
            } else if ((conf & XRLT_TESTNAMEVALUE_VALUE_NODE) &&
                       xmlStrEqual(tmp->name, XRLT_ELEMENT_VALUE))
            {
                if (_nval != NULL) {
                    dup = tmp;
                    break;
                }
                if (tmp2 == NULL) { tmp2 = tmp; }
                _nval = tmp;
            } else if (other == NULL) {
                other = tmp;
            }
        } else if (other == NULL) {
            other = tmp;
        }

        tmp = tmp->next;
    }

    if (dup != NULL) {
        xrltTransformError(NULL, sheet, dup, "Duplicate element\n");
        return FALSE;
    }

    if ((_ntest != NULL || _ntype != NULL || _nname != NULL || _nval != NULL) &&
        other != NULL)
    {
        if (tmp2 != node->children) { other = tmp2; }
        xrltTransformError(NULL, sheet, other, "Unexpected element\n");
        return FALSE;
    } else if (other != NULL) {
        _nval = node;
    }

    if (conf & XRLT_TESTNAMEVALUE_TEST_ATTR) {
        _test = xmlGetProp(node, XRLT_ELEMENT_ATTR_TEST);

        if (_test != NULL && _ntest != NULL) {
            xrltTransformError(
                NULL, sheet, node,
                "Element shouldn't have both 'test' attribute and 'test' node\n"
            );
            goto error;
        }
    }

    if (conf & XRLT_TESTNAMEVALUE_TYPE_ATTR) {
        _type = xmlGetProp(node, XRLT_ELEMENT_ATTR_TYPE);

        if (_type != NULL && _ntype != NULL) {
            xrltTransformError(
                NULL, sheet, node,
                "Element shouldn't have both 'type' attribute and 'type' node\n"
            );
            goto error;
        }
    }

    if (conf & XRLT_TESTNAMEVALUE_NAME_ATTR) {
        _name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

        if (_name != NULL && _nname != NULL) {
            xrltTransformError(
                NULL, sheet, node,
                "Element shouldn't have both 'name' attribute and 'name' node\n"
            );
            goto error;
        }
    }

    if (conf & XRLT_TESTNAMEVALUE_VALUE_ATTR) {
        _val = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);

        if (_val != NULL && _nval != NULL) {
            xrltTransformError(
                NULL, sheet, node,
                "Element shouldn't have both 'select' attribute and value "
                "node\n"
            );
            goto error;
        }
    }

    if (_ntest != NULL) {
        _test = xmlGetProp(_ntest, XRLT_ELEMENT_ATTR_SELECT);

        if (_test != NULL && _ntest->children != NULL) {
            xrltTransformError(
                NULL, sheet, _ntest,
                "Element shouldn't have both 'select' attribute and content\n"
            );
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(_ntest, n);
        n->xrlt = TRUE;
    }

    if (_ntype != NULL) {
        _etype = xmlGetProp(_ntype, XRLT_ELEMENT_ATTR_SELECT);

        if (_etype != NULL && _ntype->children != NULL) {
            xrltTransformError(
                NULL, sheet, _ntype,
                "Element shouldn't have both 'select' attribute and content\n"
            );
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(_ntype, n);
        n->xrlt = TRUE;
    }

    if (_nname != NULL) {
        _ename = xmlGetProp(_nname, XRLT_ELEMENT_ATTR_SELECT);

        if (_ename != NULL && _nname->children != NULL) {
            xrltTransformError(
                NULL, sheet, _nname,
                "Element shouldn't have both 'select' attribute and content\n"
            );
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(_nname, n);
        n->xrlt = TRUE;
    }

    if (_nval != NULL) {
        _eval = xmlGetProp(_nval, XRLT_ELEMENT_ATTR_SELECT);

        if (_eval != NULL && _nval->children != NULL) {
            xrltTransformError(
                NULL, sheet, _nval,
                "Element shouldn't have both 'select' attribute and content\n"
            );
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(_nval, n);
        n->xrlt = TRUE;
    }

    if (_test != NULL) {
        *xtest = xmlXPathCompile(_test);

        xmlFree(_test);
        _test = NULL;

        if (*xtest == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile 'test' expression\n");
            goto error;
        }
    } else if (_ntest != NULL) {
        if (xrltHasXRLTElement(_ntest->children)) {
            *ntest = _ntest->children;
        } else {
            _test = xmlXPathCastNodeToString(_ntest);

            *test = xmlXPathCastStringToBoolean(_test) ? 1 : 2;

            xmlFree(_test);
            _test = NULL;
        }
    }

    if (_type != NULL) {
        *type = _type;
        _type = NULL;
    } else if (_etype != NULL) {
        *xtype = xmlXPathCompile(_etype);

        xmlFree(_etype);
        _etype = NULL;

        if (*xtype == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile 'type' select expression\n");
            goto error;
        }
    } else if (_ntype != NULL) {
        if (xrltHasXRLTElement(_ntype->children)) {
            *ntype = _ntype->children;
        } else {
            *type = xmlXPathCastNodeToString(_ntype);
        }
    }

    if (_name != NULL) {
        *name = _name;
        _name = NULL;
    } else if (_ename != NULL) {
        *xname = xmlXPathCompile(_ename);

        xmlFree(_ename);
        _ename = NULL;

        if (*xname == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile 'name' select expression\n");
            goto error;
        }
    } else if (_nname != NULL) {
        if (xrltHasXRLTElement(_nname->children)) {
            *nname = _nname->children;
        } else {
            *name = xmlXPathCastNodeToString(_nname);
        }
    }

    if (_val != NULL) {
        *xval = xmlXPathCompile(_val);

        xmlFree(_val);
        _val = NULL;

        if (*xval == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile 'value' select expression\n");
            goto error;
        }
    } else if (_eval != NULL) {
        *xval = xmlXPathCompile(_eval);

        xmlFree(_eval);
        _eval = NULL;

        if (*xval == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile 'value' select expression\n");
            goto error;
        }
    } else if (_nval != NULL) {
        if ((conf & XRLT_TESTNAMEVALUE_TO_STRING) &&
            !xrltHasXRLTElement(_nval->children))
        {
            *val = xmlXPathCastNodeToString(_nval);
        } else {
            *nval = _nval->children;
        }
    }

    if ((conf & XRLT_TESTNAMEVALUE_TEST_REQUIRED) && *type == 0 &&
        *ntype == NULL && *xtype == NULL)
    {
        xrltTransformError(
            NULL, sheet, node, "'test' attribute or 'test' node is required\n"
        );
        goto error;
    }

    if ((conf & XRLT_TESTNAMEVALUE_TYPE_REQUIRED) && *type == NULL &&
        *ntype == NULL && *xtype == NULL)
    {
        xrltTransformError(
            NULL, sheet, node, "'type' attribute or 'type' node is required\n"
        );
        goto error;
    }

    if ((conf & XRLT_TESTNAMEVALUE_NAME_REQUIRED) && *name == NULL &&
        *nname == NULL && *xname == NULL)
    {
        xrltTransformError(
            NULL, sheet, node, "'name' attribute or 'name' node is required\n"
        );
        goto error;
    }

    if ((conf & XRLT_TESTNAMEVALUE_VALUE_REQUIRED) && *val == NULL &&
        *nval == NULL && *xval == NULL)
    {
        xrltTransformError(
            NULL, sheet, node, "'select' attribute or content is required\n"
        );
        goto error;
    }

    if ((conf & XRLT_TESTNAMEVALUE_NAME_OR_VALUE_REQUIRED) && *val == NULL &&
        *nval == NULL && *xval == NULL && *name == NULL && *nname == NULL &&
        *xname == NULL)
    {
        xrltTransformError(NULL, sheet, node, "Name or value are required\n");
        goto error;
    }

    ASSERT_NODE_DATA_GOTO(node, n);
    n->xrlt = TRUE;

    return TRUE;

  error:
    if (_test != NULL) { xmlFree(_test); }
    if (_type != NULL) { xmlFree(_type); }
    if (_name != NULL) { xmlFree(_name); }
    if (_val != NULL) { xmlFree(_val); }
    if (_etype != NULL) { xmlFree(_etype); }
    if (_ename != NULL) { xmlFree(_ename); }
    if (_eval != NULL) { xmlFree(_eval); }

    if (((conf & XRLT_TESTNAMEVALUE_TEST_ATTR) |
         (conf & XRLT_TESTNAMEVALUE_TEST_NODE)) && *xtest != NULL)
    {
        xmlXPathFreeCompExpr(*xtest);
        *xtest = NULL;
    }

    if (((conf & XRLT_TESTNAMEVALUE_TYPE_ATTR) |
         (conf & XRLT_TESTNAMEVALUE_TYPE_NODE)) && *xtype != NULL)
    {
        xmlXPathFreeCompExpr(*xtype);
        *xtype = NULL;
    }

    if (((conf & XRLT_TESTNAMEVALUE_NAME_ATTR) |
         (conf & XRLT_TESTNAMEVALUE_NAME_NODE)) && *xname != NULL)
    {
        xmlXPathFreeCompExpr(*xname);
        *xname = NULL;
    }

    if (((conf & XRLT_TESTNAMEVALUE_VALUE_ATTR) |
         (conf & XRLT_TESTNAMEVALUE_VALUE_NODE)) && *xval != NULL)
    {
        xmlXPathFreeCompExpr(*xval);
        *xval = NULL;
    }

    return FALSE;
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
                      xmlChar *val, xmlNodePtr nval, xmlXPathCompExprPtr xval,
                      xmlChar **ret)
{
    if (val != NULL) {
        *ret = xmlStrdup(val);
    } else if (nval != NULL) {
        xmlNodePtr   node;

        NEW_CHILD(ctx, node, insert, "tmp");

        TRANSFORM_SUBTREE(ctx, nval, node);

        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltSetStringResult, node, insert, ret
        );
    } else if (xval != NULL) {
        COUNTER_INCREASE(ctx, insert);

        return xrltSetStringResultByXPath(ctx, xval, insert, ret);
    }

    return TRUE;
}


static inline xrltBool
xrltTransformToBoolean(xrltContextPtr ctx, xmlNodePtr insert, xrltBool val,
                       xmlNodePtr nval, xmlXPathCompExprPtr xval, xrltBool *ret)
{
    if (nval != NULL) {
        xmlNodePtr   node;

        NEW_CHILD(ctx, node, insert, "tmp");

        TRANSFORM_SUBTREE(ctx, nval, node);

        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltSetBooleanResult, node, insert, ret
        );
    } else if (xval != NULL) {
        COUNTER_INCREASE(ctx, insert);

        return xrltSetBooleanResultByXPath(ctx, xval, insert, ret);
    } else {
        *ret = val;
    }

    return TRUE;
}


#define TRANSFORM_TO_STRING(ctx, node, val, nval, xval, ret) {                \
    if (!xrltTransformToString(ctx, node, val, nval, xval, ret)) {            \
        return FALSE;                                                         \
    }                                                                         \
}


#define TRANSFORM_TO_BOOLEAN(ctx, node, val, nval, xval, ret) {               \
    if (!xrltTransformToBoolean(ctx, node, val, nval, xval, ret)) {           \
        return FALSE;                                                         \
    }                                                                         \
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_TRANSFORM_H__ */
