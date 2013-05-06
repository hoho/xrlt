#include <libxml/xpathInternals.h>
#include "transform.h"
#include "variable.h"


void *
xrltVariableCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltCompiledVariableData  *ret;
    int                        conf;
    xmlChar                   *tmp;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltCompiledVariableData*,
                sizeof(xrltCompiledVariableData), NULL);

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
    xmlDocPtr                  vdoc;
    xmlXPathObjectPtr          val;
    xmlChar                    id[sizeof(xmlNodePtr) * 3];

    if (data == NULL) {
        if (ctx->xpath->varHash == NULL) {
            ctx->xpath->varHash = xmlHashCreate(5);
            if (ctx->xpath->varHash == NULL) {
                xrltTransformError(ctx, NULL, NULL,
                                   "Variable hash creation failed\n");
                return FALSE;
            }
        }

        sprintf((char *)id, "%p", insert);

        vdoc = xmlNewDoc(NULL);
        xmlAddChild(ctx->var, (xmlNodePtr)vdoc);

        if (vdata->nval != NULL) {
            // Variable from content.
            val = xmlXPathNewNodeSet((xmlNodePtr)vdoc);

            if (val == NULL) {
                xrltTransformError(ctx, NULL, NULL,
                                   "Failed to initialize variable\n");
                return FALSE;
            }

            if (xmlHashAddEntry2(ctx->xpath->varHash, id, vdata->name, val)) {
                // It could be out of memory, but it's redefenition of the
                // variable most likely.
                xmlXPathFreeObject(val);

                xrltTransformError(ctx, NULL, vdata->node,
                                   "Variable already exists\n");
                return FALSE;
            }

            COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

            TRANSFORM_SUBTREE(ctx, vdata->node->children, (xmlNodePtr)vdoc);

            SCHEDULE_CALLBACK(
                ctx, &ctx->tcb, xrltVariableTransform, comp, insert, vdoc
            );
        } else if (vdata->xval != NULL) {
            // Variable from XPath expression.
            if (!xrltXPathEval(ctx, insert, vdata->xval, &val)) {
                return FALSE;
            }

            if (ctx->xpathWait == NULL) {
                if (xmlHashAddEntry2(ctx->xpath->varHash, id, vdata->name, val))
                {
                    // It could be out of memory, but it's redefenition of the
                    // variable most likely.
                    xmlXPathFreeObject(val);

                    xrltTransformError(ctx, NULL, vdata->node,
                                       "Variable already exists\n");
                    return FALSE;
                }
            } else {
                COUNTER_INCREASE(ctx, (xmlNodePtr)vdoc);

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltVariableTransform, comp, insert, vdoc
                );
            }
        }
    } else {
        COUNTER_DECREASE(ctx, (xmlNodePtr)data);
    }

    return TRUE;
}
