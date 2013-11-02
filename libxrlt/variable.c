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
    xrltNodeDataPtr       n;
    int                   i;
    xrltBool              toplevel;


    XRLT_MALLOC(NULL, sheet, node, ret, xrltVariableDataPtr,
                sizeof(xrltVariableData), NULL);

    toplevel = node->parent == NULL || node->parent->parent == NULL ||
               node->parent->parent->parent == NULL;

    ret->isParam = \
        toplevel && xmlStrEqual(node->name, (const xmlChar *)"param") ? \
            TRUE : FALSE;

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

    if (!xrltCompileValue(sheet, node, node->children,
                          XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, FALSE,
                          &ret->val))
    {
        goto error;
    }

    ret->ownVal = TRUE;

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

        if (vcomp->ownVal) {
            CLEAR_XRLT_VALUE(vcomp->val);
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

    if (!xrltXPathEval(ctx, insert, &vcomp->val.xpathval, &val)) {
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
        if (vcomp->isParam && ctx->params != NULL) {
            xmlChar  *v;

            v = (xmlChar *)xmlHashLookup3(ctx->params, vcomp->name, NULL, NULL);

            if (v != NULL) {
                val = xmlXPathNewString(v);

                XRLT_SET_VARIABLE(id, vcomp->node, vcomp->name,
                                  vcomp->declScope, sc, NULL, val);
                return TRUE;
            }
        }

        switch (vcomp->val.type) {
            case XRLT_VALUE_NODELIST:
                vdoc = xmlNewDoc(NULL);
                xmlAddChild(ctx->var, (xmlNodePtr)vdoc);
                vdoc->doc = vdoc;

                if (!xrltCopyXPathRoot(insert, vdoc)) {
                    return FALSE;
                }

                XRLT_SET_VARIABLE(id, vcomp->node, vcomp->name,
                                  vcomp->declScope, sc, vdoc, val);

                COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

                TRANSFORM_SUBTREE(ctx, vcomp->val.nodeval, (xmlNodePtr)vdoc);

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltVariableTransform, comp, insert, vdoc
                );

                break;

            case XRLT_VALUE_XPATH:
                vdoc = xmlNewDoc(NULL);
                xmlAddChild(ctx->var, (xmlNodePtr)vdoc);
                vdoc->doc = vdoc;

                COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

                if (!xrltVariableFromXPath(ctx, comp, insert, vdoc)) {
                    return FALSE;
                }

                break;

            case XRLT_VALUE_TEXT:
            case XRLT_VALUE_INT:
            case XRLT_VALUE_EMPTY:
                if (vcomp->val.type == XRLT_VALUE_TEXT) {
                    val = xmlXPathNewString(vcomp->val.textval);
                } else if (vcomp->val.type == XRLT_VALUE_INT) {
                    val = xmlXPathNewFloat(vcomp->val.intval);
                } else {
                    val = xmlXPathNewNodeSet(NULL);
                }

                XRLT_SET_VARIABLE(id, vcomp->node, vcomp->name,
                                  vcomp->declScope, sc, NULL, val);

                break;
        }
    } else {
        COUNTER_DECREASE(ctx, (xmlNodePtr)data);
    }

    return TRUE;
}
