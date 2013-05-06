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
        if (param->name != NULL && param->ownName) {
            xmlFree(param->name);
        }

        if (param->jsname != NULL && param->ownJsname) {
            xmlFree(param->jsname);
        }

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


    XRLT_MALLOC(NULL, sheet, node, ret, xrltFunctionParamPtr,
                sizeof(xrltFunctionParam), NULL);

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);
    ret->ownName = TRUE;

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

    XRLT_MALLOC(NULL, sheet, node, ret, xrltFunctionData*,
                sizeof(xrltFunctionData), NULL);

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
                RAISE_OUT_OF_MEMORY(NULL, sheet, node);
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
    size_t                 i, j;
    int                    k;

    if (prevcomp == NULL) {
        XRLT_MALLOC(NULL, sheet, node, ret, xrltApplyData*,
                    sizeof(xrltApplyData), NULL);

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
                    RAISE_OUT_OF_MEMORY(NULL, sheet, node);
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
            // list of the parameters to process in the runtime.
            XRLT_MALLOC(NULL, sheet, node, newp, xrltFunctionParamPtr*,
                        sizeof(xrltFunctionParamPtr) * ret->func->paramLen,
                        NULL);

            memcpy(newp, ret->func->param,
                   sizeof(xrltFunctionParamPtr) * ret->func->paramLen);

            for (i = 0; i < ret->func->paramLen; i++) {
                newp[i]->ownName = FALSE;
                newp[i]->ownJsname = FALSE;
                newp[i]->ownXval = FALSE;
            }

            i = 0;
            j = 0;

            while (i < ret->func->paramLen && j < ret->paramLen) {
                k = xmlStrcmp(newp[i]->name, ret->param[j]->name);

                if (k == 0) {
                    memcpy(newp[i], ret->param[j], sizeof(xrltFunctionParam));
                    memset(ret->param[j], 0, sizeof(xrltFunctionParam));
                    i++;
                    j++;
                } else if (k < 0) {
                    i++;
                } else {
                    j++;
                }
            }
        } else {
            newp = NULL;
        }

        if (ret->param != NULL) {
            for (i = 0; i < ret->paramLen; i++) {
                xrltFunctionParamFree(ret->param[i]);
            }

            xrltFree(ret->param);
        }

        ret->param = newp;
        ret->paramLen = ret->func->paramLen;
        ret->paramSize = ret->paramLen;
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


static void
xrltApplyTransformingFree(void *data)
{
    xrltApplyTransformingData  *tdata;

    if (data != NULL) {
        tdata = (xrltApplyTransformingData *)data;

        xrltFree(data);
    }
}


xrltBool
xrltApplyTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                   void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr                  node;
    xmlDocPtr                   pdoc;
    xrltApplyData              *acomp = (xrltApplyData *)comp;
    xrltNodeDataPtr             n;
    xrltApplyTransformingData  *tdata;
    size_t                      i;

    if (data == NULL) {
        NEW_CHILD(ctx, node, insert, "a");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, acomp->node, tdata, xrltApplyTransformingData*,
                    sizeof(xrltApplyTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltApplyTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "p");

        tdata->paramNode = node;

        for (i = 0; i < acomp->paramLen; i++) {
            pdoc = xmlNewDoc(NULL);
            xmlAddChild(node, (xmlNodePtr)pdoc);

            if (acomp->param[i]->nval != NULL) {
                TRANSFORM_SUBTREE(ctx, acomp->param[i]->nval, (xmlNodePtr)pdoc);
            } else if (acomp->param[i]->xval != NULL) {

            }

            printf("pppp %s\n", (char *)acomp->param[i]->name);
        }

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltApplyTransform, comp, insert,
                          tdata);
    } else {
        tdata = (xrltApplyTransformingData *)data;

        ASSERT_NODE_DATA(tdata->paramNode, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltApplyTransform, comp, insert,
                              data);

            return TRUE;
        }

        COUNTER_DECREASE(ctx, tdata->node);
    }

    return TRUE;
}
