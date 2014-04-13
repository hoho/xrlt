/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_VARIABLE_H__
#define __XRLT_VARIABLE_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


#define XRLT_SET_VARIABLE_ID(_id, _scope, _pos)                               \
    sprintf((char *)_id, "%p-%zd", _scope, _pos);


#define XRLT_SET_VARIABLE(_id, _node, _name, _scope, _pos, _vdoc, _val) {     \
    xmlNodePtr   _lookup = _scope;                                            \
    while (_lookup != NULL && _lookup != ctx->sheetNode) {                    \
        XRLT_SET_VARIABLE_ID(_id, _lookup, _pos);                             \
        if (xmlHashLookup2(ctx->xpath->varHash, _id, _name)) {                \
            if (_val) { xmlXPathFreeObject(_val); }                           \
            xrltTransformError(ctx, NULL, _node,                              \
                               "Redefinition of variable '%s'\n", _name);     \
            return FALSE;                                                     \
        }                                                                     \
        _lookup = _lookup->parent;                                            \
    }                                                                         \
    if (_vdoc != NULL) {                                                      \
        _val = xmlXPathNewNodeSet((xmlNodePtr)_vdoc);                         \
    }                                                                         \
    if (_val == NULL) {                                                       \
        xrltTransformError(ctx, NULL, _node,                                  \
                           "Failed to initialize variable\n");                \
        return FALSE;                                                         \
    }                                                                         \
    XRLT_SET_VARIABLE_ID(_id, _scope, _pos);                                  \
    if (xmlHashAddEntry2(ctx->xpath->varHash, _id, _name, _val)) {            \
        xmlXPathFreeObject(_val);                                             \
        xrltTransformError(ctx, NULL, _node,                                  \
                           "Redefinition of variable '%s'\n", _name);         \
        return FALSE;                                                         \
    }                                                                         \
}


typedef struct _xrltVariableData xrltVariableData;
typedef xrltVariableData* xrltVariableDataPtr;
struct _xrltVariableData {
    xmlNodePtr          node;
    xmlNodePtr          declScope;
    size_t              declScopePos;

    xrltBool            isParam;

    xrltBool            sync;

    xrltBool            ownName;
    xmlChar            *name;

    xrltBool            ownJsname;
    xmlChar            *jsname;

    xrltBool            ownVal;
    xrltCompiledValue   val;
};


typedef struct {
    xmlNodePtr                  node;
} xrltVariableTransformingData;


void *
        xrltVariableCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltVariableFree        (void *comp);
xrltBool
        xrltVariableTransform   (xrltContextPtr ctx, void *comp,
                                 xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_VARIABLE_H__ */
