#include <libxml/xpathInternals.h>
#include "transform.h"
#include "variable.h"


void *
xrltVariableCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltCompiledVariableData  *ret;

    XRLT_MALLOC(ret, xrltCompiledVariableData*,
                sizeof(xrltCompiledVariableData), "xrltVariableCompile", NULL);

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

    if (xmlValidateNCName(ret->name, 0)) {
        xrltTransformError(NULL, sheet, node, "Invalid variable name\n");
        goto error;
    }

    if (!xrltIncludeStrNodeXPath(sheet, node, FALSE, NULL, &ret->nval,
                                 &ret->xval))
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
        xrltCompiledVariableData  *data = (xrltCompiledVariableData *)comp;

        if (data->name != NULL) { xmlFree(data->name); }
        if (data->xval != NULL) { xmlXPathFreeCompExpr(data->xval); }

        xmlFree(data);
    }
}


xrltBool
xrltVariableTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    xrltCompiledVariableData  *vdata = (xrltCompiledVariableData *)comp;
    xmlNodePtr                 node;
    xmlXPathObjectPtr          val;
    xmlChar                    id[sizeof(size_t) * 2];

    if (data == NULL) {
        if (ctx->xpath->varHash == NULL) {
            ctx->xpath->varHash = xmlHashCreate(5);
            if (ctx->xpath->varHash == NULL) {
                xrltTransformError(ctx, NULL, NULL,
                                   "Variable hash creation failed\n");
                return FALSE;
            }
        }

        NEW_CHILD(ctx, node, ctx->var, "v");

        val = xmlXPathNewNodeSet(node);

        if (val == NULL) {
            xrltTransformError(ctx, NULL, NULL,
                               "Failed to initialize variable\n");
            return FALSE;
        }

        sprintf((char *)id, "%p", insert);

        if (xmlHashAddEntry2(ctx->xpath->varHash, id, vdata->name, val)) {

        }
        printf("hahahahah %p\n", ctx->xpath->varHash);

    } else {

    }

    return TRUE;
}
