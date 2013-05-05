#include "transform.h"
#include "function.h"


static void
xrltFunctionRemove(void *payload, xmlChar *name)
{
    xrltFunctionFree(payload);
}


static void
xrltFunctionParamFree(xrltFunctionParamPtr param)
{
    if (param != NULL) {
        if (param->name != NULL) { xmlFree(param->name); }
        if (param->jsname != NULL) { xmlFree(param->jsname); }

        if (param->xval != NULL && param->ownXval) {
            xmlXPathFreeCompExpr(param->xval);
        }

        xrltFree(param);
    }
}


static xrltFunctionParamPtr
xrltFunctionParamCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                         xrltBool func)
{
    xrltFunctionParamPtr   ret = NULL;
    xmlChar               *select;
    xrltNodeDataPtr        n;


    XRLT_MALLOC(ret, xrltFunctionParamPtr, sizeof(xrltFunctionParam),
                "xrltFunctionParamCompile", NULL);

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

    if (xmlValidateNCName(ret->name, 0)) {
        xrltTransformError(NULL, sheet, node, "Invalid function name\n");
        goto error;
    }

    ret->node = node;

    ASSERT_NODE_DATA_GOTO(node, n);
    n->xrlt = TRUE;

    select = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);
    if (select != NULL) {
        ret->xval = xmlXPathCompile(select);
        ret->ownXval = TRUE;

        xmlFree(select);

        if (ret->xval == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile 'select' expression\n");
            goto error;
        }
    }

    ret->nval = node->children;

    if (ret->xval != NULL && ret->nval != NULL) {
        xrltTransformError(
            NULL, sheet, node,
            "Element shouldn't have both 'select' attribute and content\n"
        );
        goto error;
    }

    if (ret->xval == NULL && ret->nval == NULL && !func) {
        xrltTransformError(NULL, sheet, node, "Parameter has no value\n");
        goto error;
    }

    return ret;

  error:
    xrltFunctionParamFree(ret);

    return NULL;
}


static int
xrltFunctionParamSort(const void *a, const void *b)
{
    xrltFunctionParamPtr   pa;
    xrltFunctionParamPtr   pb;

    memcpy(&pa, a, sizeof(xrltFunctionParamPtr));
    memcpy(&pb, b, sizeof(xrltFunctionParamPtr));

    return xmlStrcmp(pa->name, pb->name);
}


void *
xrltFunctionCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltFunctionData      *ret = NULL;
    xmlNodePtr             tmp;
    xrltFunctionParamPtr   p;
    xrltFunctionParamPtr  *newp;
    size_t                 i;

    XRLT_MALLOC(ret, xrltFunctionData*, sizeof(xrltFunctionData),
                "xrltFunctionCompile", NULL);

    if (sheet->funcs == NULL) {
        sheet->funcs = xmlHashCreate(20);

        if (sheet->funcs == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Functions hash creation failed\n");
            goto error;
        }
    }

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

    if (xmlValidateNCName(ret->name, 0)) {
        xrltTransformError(NULL, sheet, node, "Invalid function name\n");
        goto error;
    }

    // We keep the last function declaration as a function.
    xmlHashRemoveEntry3(sheet->funcs, ret->name, NULL, NULL,
                        xrltFunctionRemove);

    if (xmlHashAddEntry3(sheet->funcs, ret->name, NULL, NULL, ret)) {
        xrltTransformError(NULL, sheet, node, "Failed to add function\n");
        goto error;
    }

    tmp = node->children;

    while (tmp != NULL && xmlStrEqual(tmp->name, XRLT_ELEMENT_PARAM) &&
           tmp->ns != NULL && xmlStrEqual(tmp->ns->href, XRLT_NS))
    {
        p = xrltFunctionParamCompile(sheet, tmp, TRUE);

        if (p == NULL) {
            goto error;
        }

        if (ret->paramLen >= ret->paramSize) {
            ret->paramSize += 10;
            newp = (xrltFunctionParamPtr *)xrltRealloc(
                ret->param, sizeof(xrltFunctionParamPtr) * ret->paramSize
            );

            if (newp == NULL) {
                xrltFunctionParamFree(p);
                xrltTransformError(NULL, sheet, node,
                                   "xrltFunctionCompile: Out of memory\n");
                goto error;
            }

            ret->param = newp;
        }

        ret->param[ret->paramLen++] = p;

        tmp = tmp->next;
    }

    // Don't want to mess with hashes at the moment, so, just sort the
    // parameters.
    if (ret->paramLen > 1) {
        qsort(ret->param, ret->paramLen, sizeof(xrltFunctionParamPtr),
              xrltFunctionParamSort);
    }

    for (i = 1; i < ret->paramLen; i++) {
        if (xmlStrEqual(ret->param[i - 1]->name, ret->param[i]->name)) {
            xrltTransformError(NULL, sheet, ret->param[i]->node,
                               "Duplicate parameter\n");
            goto error;
        }
    }

    if (tmp == NULL) {
        xrltTransformError(NULL, sheet, node, "Function has no content\n");
        goto error;
    }

    ret->children = tmp;

    return ret;

  error:
    xrltFunctionFree(ret);

    return NULL;
}


void
xrltFunctionFree(void *comp)
{
    if (comp != NULL) {
        xrltFunctionData  *f = (xrltFunctionData *)comp;
        size_t             i;

        if (f->name != NULL) { xmlFree(f->name); }

        if (f->param != NULL) {
            for (i = 0; i < f->paramLen; i++) {
                xrltFunctionParamFree(f->param[i]);
            }

            xrltFree(f->param);
        }

        xrltFree(comp);
    }
}


xrltBool
xrltFunctionTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    return TRUE;
}


void *
xrltApplyCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltApplyData         *ret = NULL;
    xmlChar *              name;
    xmlNodePtr             tmp;
    xrltFunctionParamPtr   p;
    xrltFunctionParamPtr  *newp;
    size_t                 i;

    if (prevcomp == NULL) {
        XRLT_MALLOC(ret, xrltApplyData*, sizeof(xrltApplyData),
                    "xrltApplyCompile", NULL);

        tmp = node->children;

        while (tmp != NULL && xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_PARAM)
               && tmp->ns != NULL && xmlStrEqual(tmp->ns->href, XRLT_NS))
        {
            p = xrltFunctionParamCompile(sheet, tmp, FALSE);

            if (p == NULL) {
                goto error;
            }

            if (ret->paramLen >= ret->paramSize) {
                ret->paramSize += 10;
                newp = (xrltFunctionParamPtr *)xrltRealloc(
                    ret->param, sizeof(xrltFunctionParamPtr) * ret->paramSize
                );

                if (newp == NULL) {
                    xrltFunctionParamFree(p);
                    xrltTransformError(NULL, sheet, node,
                                       "xrltApplyCompile: Out of memory\n");
                    goto error;
                }

                ret->param = newp;
            }

            ret->param[ret->paramLen++] = p;

            tmp = tmp->next;
        }

        if (ret->paramLen > 1) {
            qsort(ret->param, ret->paramLen, sizeof(xrltFunctionParamPtr),
                  xrltFunctionParamSort);
        }

        for (i = 1; i < ret->paramLen; i++) {
            if (xmlStrEqual(ret->param[i - 1]->name, ret->param[i]->name)) {
                xrltTransformError(NULL, sheet, ret->param[i]->node,
                                   "Duplicate parameter\n");
                goto error;
            }
        }

        if (tmp != NULL) {
            xrltTransformError(NULL, sheet, tmp, "Unexpected element\n");
            goto error;
        }
    } else {
        ret = (xrltApplyData *)prevcomp;

        name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

        if (xmlValidateNCName(name, 0)) {
            if (name != NULL) { xmlFree(name); }
            xrltTransformError(NULL, sheet, node, "Invalid function name\n");
            return NULL;
        }

        ret->func = (xrltFunctionData *)xmlHashLookup3(sheet->funcs, name,
                                                       NULL, NULL);
        xmlFree(name);

        if (ret->func == NULL) {
            xrltTransformError(NULL, sheet, node, "Function is not declared\n");
            return NULL;
        }

        if (ret->func->paramLen > 0) {
            // Merge xrl:apply parameters to xrl:function parameters to get the
            // list of the parameters to process the runtime.
            XRLT_MALLOC(newp, xrltFunctionParamPtr*,
                        sizeof(xrltFunctionParamPtr) * ret->func->paramLen,
                        "xrltApplyCompile", NULL);

            // TODO: Merge.
        } else {
            newp = NULL;
        }

        if (ret->param != NULL) {
            // TODO: Free the remainings of the ret->param.
        }

        ret->param = newp;
        ret->paramLen = ret->func->paramLen;
    }

    return ret;

  error:
    xrltApplyFree(ret);

    return NULL;
}


void
xrltApplyFree(void *comp)
{
    if (comp != NULL) {
        xrltApplyData  *a = (xrltApplyData *)comp;
        size_t          i;

        if (a->param != NULL) {
            for (i = 0; i < a->paramLen; i++) {
                xrltFunctionParamFree(a->param[i]);
            }

            xrltFree(a->param);
        }

        xrltFree(comp);
    }
}


xrltBool
xrltApplyTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                   void *data)
{
    return TRUE;
}
