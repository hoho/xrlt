#include <libxml/xpathInternals.h>
#include "transform.h"
#include "variable.h"


void *
xrltVariableCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltVariableData *ret;
    int                        conf;
    xmlChar                   *tmp;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltVariableData*,
                sizeof(xrltVariableData), NULL);

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

    if (xmlValidateNCName(ret->name, 0)) {
        xrltTransformError(NULL, sheet, node, "Invalid variable name\n");
        goto error;
    }

    conf = XRLT_TESTNAMEVALUE_VALUE_ATTR |
           XRLT_TESTNAMEVALUE_VALUE_NODE |
           XRLT_TESTNAMEVALUE_VALUE_REQUIRED;

    if (!xrltCompileTestNameValueNode(sheet, node, conf, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, NULL, NULL, &tmp,
                                      &ret->nval, &ret->xval))
    {
        goto error;
    }

    ret->node = node;

    return ret;

  error:
    xrltVariableFree(ret);
    return NULL;
}


void
xrltVariableFree(void *comp)
{
    if (comp != NULL) {
        xrltVariableData *data = (xrltVariableData *)comp;

        if (data->name != NULL) { xmlFree(data->name); }
        if (data->xval.expr != NULL) { xmlXPathFreeCompExpr(data->xval.expr); }

        xmlFree(data);
    }
}


#define XRLT_SET_VARIABLE_ID(id, scope, pos)                                  \
    sprintf((char *)id, "%p-%zd", scope, pos);


#define XRLT_SET_VARIABLE(ctx, node, name, vdoc, val) {                       \
    if (vdoc) {                                                               \
        val = xmlXPathNewNodeSet((xmlNodePtr)vdoc);                           \
    }                                                                         \
    if (val == NULL) {                                                        \
        xrltTransformError(ctx, NULL, NULL,                                   \
                           "Failed to initialize variable\n");                \
        return FALSE;                                                         \
    }                                                                         \
fprintf(stderr,"DECL: %s\n", id);\
    if (xmlHashAddEntry2(ctx->xpath->varHash, id, name, val)) {               \
        xmlXPathFreeObject(val);                                              \
        xrltTransformError(ctx, NULL, node,                                   \
                           "Variable already exists\n");                      \
        return FALSE;                                                         \
    }                                                                         \
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

    if (!xrltXPathEval(ctx, insert, &vcomp->xval, NULL, &val)) {
        return FALSE;
    }

    ASSERT_NODE_DATA(vdoc, n);

    if (ctx->xpathWait == NULL) {
        XRLT_SET_VARIABLE_ID(id, insert, 1);

        if (n->data != NULL) {
            xmlHashRemoveEntry2(ctx->xpath->varHash, id, vcomp->name,
                                (xmlHashDeallocator)xmlXPathFreeObject);

            COUNTER_DECREASE(ctx, (xmlNodePtr)vdoc);
        }

        XRLT_SET_VARIABLE(ctx, vcomp->node, vcomp->name, NULL, val);

        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltVariableTransform, comp, insert, data
        );
    } else {
        if (n->data == NULL) {
            XRLT_SET_VARIABLE_ID(id, insert, 1);

            XRLT_SET_VARIABLE(ctx, vcomp->node, vcomp->name, vdoc, val);

            COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

            n->data = (void *)0x1;
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

    if (data == NULL) {
        vdoc = xmlNewDoc(NULL);

        xmlAddChild(ctx->var, (xmlNodePtr)vdoc);

        if (vcomp->nval != NULL) {
            XRLT_SET_VARIABLE_ID(id, insert, 1);

            XRLT_SET_VARIABLE(ctx, vcomp->node, vcomp->name, vdoc, val);

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
        }
    } else {
        COUNTER_DECREASE(ctx, (xmlNodePtr)data);
    }

    return TRUE;
}
