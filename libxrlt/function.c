#include "transform.h"
#include "function.h"


static void
xrltFunctionRemove(void *payload, xmlChar *name)
{
    xrltFunctionFree(payload);
}


static void
xrltFunctionParamFree(xrltVariableDataPtr param)
{
    if (param != NULL) {
        if (param->name != NULL && param->ownName) {
            xmlFree(param->name);
        }

        if (param->jsname != NULL && param->ownJsname) {
            xmlFree(param->jsname);
        }

        if (param->xval.expr != NULL && param->ownXval) {
            xmlXPathFreeCompExpr(param->xval.expr);
        }

        xrltFree(param);
    }
}


static xrltVariableDataPtr
xrltFunctionParamCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                         xrltBool func)
{
    xrltVariableDataPtr   ret = NULL;
    xmlChar              *select;
    xrltNodeDataPtr       n;


    XRLT_MALLOC(NULL, sheet, node, ret, xrltVariableDataPtr,
                sizeof(xrltVariableData), NULL);

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
        ret->xval.src = node;
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
    ret->ownNval = TRUE;

    if (ret->xval.expr != NULL && ret->nval != NULL) {
        xrltTransformError(
            NULL, sheet, node,
            "Element shouldn't have both 'select' attribute and content\n"
        );
        goto error;
    }

    if (ret->xval.expr == NULL && ret->nval == NULL && !func) {
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
    xrltVariableDataPtr   pa;
    xrltVariableDataPtr   pb;

    memcpy(&pa, a, sizeof(xrltVariableDataPtr));
    memcpy(&pb, b, sizeof(xrltVariableDataPtr));

    return xmlStrcmp(pa->name, pb->name);
}


void *
xrltFunctionCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltFunctionData     *ret = NULL;
    xmlNodePtr            tmp;
    xrltVariableDataPtr   p;
    xrltVariableDataPtr  *newp;
    size_t                i;

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
            newp = (xrltVariableDataPtr *)xrltRealloc(
                ret->param, sizeof(xrltVariableDataPtr) * ret->paramSize
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
        qsort(ret->param, ret->paramLen, sizeof(xrltVariableDataPtr),
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
    ret->node = node;

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
    xrltApplyData        *ret = NULL;
    xmlChar *             name;
    xmlNodePtr            tmp;
    xrltVariableDataPtr   p;
    xrltVariableDataPtr  *newp;
    size_t                i, j;
    int                   k;

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
                newp = (xrltVariableDataPtr *)xrltRealloc(
                    ret->param, sizeof(xrltVariableDataPtr) * ret->paramSize
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
            qsort(ret->param, ret->paramLen, sizeof(xrltVariableDataPtr),
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
        fprintf(stderr, "COMPILE %d\n", node->line);


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
            XRLT_MALLOC(NULL, sheet, node, newp, xrltVariableDataPtr*,
                        sizeof(xrltVariableDataPtr) * ret->func->paramLen,
                        NULL);

            memcpy(newp, ret->func->param,
                   sizeof(xrltVariableDataPtr) * ret->func->paramLen);

            for (i = 0; i < ret->func->paramLen; i++) {
                newp[i]->ownName = FALSE;
                newp[i]->ownJsname = FALSE;
                newp[i]->ownNval = FALSE;
                newp[i]->ownXval = FALSE;
            }

            i = 0;
            j = 0;

            while (i < ret->func->paramLen && j < ret->paramLen) {
                k = xmlStrcmp(newp[i]->name, ret->param[j]->name);

                if (k == 0) {
                    memcpy(newp[i], ret->param[j], sizeof(xrltVariableData));
                    memset(ret->param[j], 0, sizeof(xrltVariableData));
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

        ret->node = node;
        ret->decl = ret->func->node;
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
    xrltApplyData              *acomp = (xrltApplyData *)comp;
    xrltNodeDataPtr             n;
    xrltApplyTransformingData  *tdata;
    size_t                      i;
    size_t                      newScope, oldScope;

    if (data == NULL) {
        fprintf(stderr, "TRA %d\n", acomp->node->line);

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

        newScope = ++ctx->lastVarScope;
        oldScope = ctx->varScope;

        for (i = 0; i < acomp->paramLen; i++) {
            if (acomp->param[i]->ownNval || acomp->param[i]->ownXval) {
                fprintf(stderr, "AAAAAA! %s %d\n", (char *)acomp->param[i]->node->parent->name, acomp->param[i]->node->line);
                ctx->varScope = newScope;
            } else {
                fprintf(stderr, "BBBBBBB! %s %d\n", (char *)acomp->param[i]->node->parent->name, acomp->param[i]->node->line);
                ctx->varScope = oldScope;
            }

            fprintf(stderr, "CCCC! %zd\n", ctx->varScope);


            if (!xrltVariableTransform(ctx, acomp->param[i], insert, NULL)) {
                return FALSE;
            }
        }


        ctx->varScope = newScope;

        TRANSFORM_SUBTREE(ctx, acomp->func->children, node);

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
