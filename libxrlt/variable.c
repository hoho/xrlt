/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include <libxml/xpathInternals.h>
#include "transform.h"
#include "variable.h"


void *
xrltVariableCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltVariableDataPtr   ret = NULL;
    xmlChar              *select;
    xrltNodeDataPtr       n;
    int                   i;


    XRLT_MALLOC(NULL, sheet, node, ret, xrltVariableDataPtr,
                sizeof(xrltVariableData), NULL);

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);
    ret->ownName = TRUE;

    if (xmlValidateNCName(ret->name, 0)) {
        xrltTransformError(NULL, sheet, node, "Invalid name\n");
        goto error;
    }

    ret->jsname = xmlStrdup(ret->name);
    ret->ownJsname = TRUE;

    if (ret->jsname == NULL) {
        ERROR_OUT_OF_MEMORY(NULL, sheet, node);
        goto error;
    }

    for (i = xmlStrlen(ret->jsname); i >= 0; i--) {
        if (ret->jsname[i] == '-') {
            ret->jsname[i] = '_';
        }
    }

    ASSERT_NODE_DATA_GOTO(node, n);
    n->xrlt = TRUE;

    select = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);
    if (select != NULL) {
        ret->xval.src = node;
        ret->xval.scope = node->parent;
        ret->xval.expr = xmlXPathCompile(select);
        ret->ownXval = TRUE;

        xmlFree(select);

        if (ret->xval.expr == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile 'select' expression\n");
            goto error;
        }
    }

    ret->nval = node->children;

    if (ret->xval.expr != NULL && ret->nval != NULL) {
        xrltTransformError(
            NULL, sheet, node,
            "Element shouldn't have both 'select' attribute and content\n"
        );
        goto error;
    }

    ret->node = node;
    ret->declScope = node->parent;

    ASSERT_NODE_DATA_GOTO(node->parent, n);
    n->hasVar = TRUE;

    return ret;

  error:
    xrltVariableFree(ret);

    return NULL;
}


void
xrltVariableFree(void *comp)
{
    if (comp != NULL) {
        xrltVariableData  *vcomp = (xrltVariableData *)comp;

        if (vcomp->name != NULL && vcomp->ownName) {
            xmlFree(vcomp->name);
        }

        if (vcomp->jsname != NULL && vcomp->ownJsname) {
            xmlFree(vcomp->jsname);
        }

        if (vcomp->xval.expr != NULL && vcomp->ownXval) {
            xmlXPathFreeCompExpr(vcomp->xval.expr);
        }

        xmlFree(vcomp);
    }
}


static xrltBool
xrltVariableFromXPath(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    xrltVariableData   *vcomp = (xrltVariableData *)comp;
    xmlNodePtr          vdoc = (xmlNodePtr)data;
    xmlXPathObjectPtr   val;
    xmlChar             id[sizeof(xmlNodePtr) * 7];
    xrltNodeDataPtr     n;
    size_t              sc;

    if (!xrltXPathEval(ctx, insert, &vcomp->xval, &val)) {
        return FALSE;
    }

    ASSERT_NODE_DATA(vdoc, n);

    if (ctx->xpathWait == NULL) {
        if (n->data != NULL) {
            sc = ((size_t)n->data) - 1;
        } else {
            sc = vcomp->declScopePos > 0 ? vcomp->declScopePos : ctx->varScope;
        }

        XRLT_SET_VARIABLE_ID(id, vcomp->declScope, sc);

        if (n->data != NULL) {
            xmlHashRemoveEntry2(ctx->xpath->varHash, id, vcomp->name,
                                (xmlHashDeallocator)xmlXPathFreeObject);

            COUNTER_DECREASE(ctx, (xmlNodePtr)vdoc);
        }

        XRLT_SET_VARIABLE(
            id, vcomp->node, vcomp->name, vcomp->declScope, sc, NULL, val
        );

        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltVariableTransform, comp, insert, data
        );
    } else {
        if (n->data == NULL) {
            sc = vcomp->declScopePos > 0 ? vcomp->declScopePos : ctx->varScope;

            XRLT_SET_VARIABLE(
                id, vcomp->node, vcomp->name, vcomp->declScope, sc, vdoc, val
            );

            COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

            n->data = (void *)(sc + 1);
        }

        ASSERT_NODE_DATA(ctx->xpathWait, n);

        SCHEDULE_CALLBACK(
            ctx, &n->tcb, xrltVariableFromXPath, comp, insert, data
        );
    }

    return TRUE;
}


xrltBool
xrltVariableTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    xrltVariableData   *vcomp = (xrltVariableData *)comp;
    xmlDocPtr           vdoc;
    xmlXPathObjectPtr   val = NULL;
    xmlChar             id[sizeof(xmlNodePtr) * 7];
    size_t              sc;

    sc = vcomp->declScopePos > 0 ? vcomp->declScopePos : ctx->varScope;

    if (data == NULL) {
        vdoc = xmlNewDoc(NULL);

        xmlAddChild(ctx->var, (xmlNodePtr)vdoc);

        if (vcomp->nval != NULL) {
            XRLT_SET_VARIABLE(
                id, vcomp->node, vcomp->name, vcomp->declScope, sc, vdoc, val
            );

            COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

            TRANSFORM_SUBTREE(ctx, vcomp->nval, (xmlNodePtr)vdoc);

            SCHEDULE_CALLBACK(
                ctx, &ctx->tcb, xrltVariableTransform, comp, insert, vdoc
            );
        } else if (vcomp->xval.expr != NULL) {
            COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

            if (!xrltVariableFromXPath(ctx, comp, insert, vdoc)) {
                return FALSE;
            }
        } else {
            val = xmlXPathNewNodeSet(NULL);

            XRLT_SET_VARIABLE(
                id, vcomp->node, vcomp->name, vcomp->declScope, sc, NULL, val
            );
        }
    } else {
        COUNTER_DECREASE(ctx, (xmlNodePtr)data);
    }

    return TRUE;
}
