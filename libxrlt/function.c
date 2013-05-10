#include "transform.h"
#include "function.h"



static void
xrltFunctionRemove(void *payload, xmlChar *name)
{
    xrltFunctionFree(payload);
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
    xmlNodePtr            tmp, tmp2;
    xmlChar              *t;
    xrltVariableDataPtr   p;
    xrltVariableDataPtr  *newp;
    size_t                i;
    xmlBufferPtr          buf = NULL;

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

    t = xmlGetProp(node, XRLT_ELEMENT_ATTR_TYPE);
    if (t != NULL) {
        if (xmlStrcasecmp(t, (xmlChar *)"javascript") == 0) {
            ret->js = TRUE;
        }
        xmlFree(t);
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
        p = (xrltVariableDataPtr)xrltVariableCompile(sheet, tmp, NULL);

        if (p == NULL) {
            goto error;
        }

        if (ret->paramLen >= ret->paramSize) {
            ret->paramSize += 10;
            newp = (xrltVariableDataPtr *)xrltRealloc(
                ret->param, sizeof(xrltVariableDataPtr) * ret->paramSize
            );

            if (newp == NULL) {
                xrltVariableFree(p);
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

    if (ret->js) {
        tmp2 = tmp;

        while (tmp2 != NULL) {
            if (tmp2->type != XML_TEXT_NODE &&
                tmp2->type != XML_CDATA_SECTION_NODE)
            {
                xrltTransformError(NULL, sheet, node, "Unexpected element\n");
                goto error;
            }
            tmp2 = tmp2->next;
        }

        buf = xmlBufferCreateSize(64);
        if (buf == NULL) {
            xrltTransformError(NULL, sheet, node, "Failed to create buffer\n");
            goto error;
        }

        if (tmp != NULL) {
            while (tmp != NULL) {
                xmlBufferCat(buf, tmp->content);
                tmp = tmp->next;
            }
        } else {
            xmlBufferCat(buf, (const xmlChar *)"return undefined;");
        }

        if (!xrltJSFunction(sheet, node, ret->name, ret->param, ret->paramLen,
                            xmlBufferContent(buf)))
        {
            xmlBufferFree(buf);
            goto error;
        }

        xmlBufferFree(buf);
    } else {
        ret->children = tmp;
    }

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
                xrltVariableFree(f->param[i]);
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
            p = (xrltVariableDataPtr)xrltVariableCompile(sheet, tmp, NULL);

            if (p == NULL) {
                goto error;
            }

            if (ret->paramLen >= ret->paramSize) {
                ret->paramSize += 10;
                newp = (xrltVariableDataPtr *)xrltRealloc(
                    ret->param, sizeof(xrltVariableDataPtr) * ret->paramSize
                );

                if (newp == NULL) {
                    xrltVariableFree(p);
                    RAISE_OUT_OF_MEMORY(NULL, sheet, node);
                    goto error;
                }

                ret->param = newp;
            }

            p->declScope = p->declScope->parent;
            if (p->xval.expr != NULL) {
                p->xval.scope = p->declScope;
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

            ret->merged = newp;

            for (i = 0; i < ret->func->paramLen; i++) {
                XRLT_MALLOC(NULL, sheet, node, p, xrltVariableDataPtr,
                            sizeof(xrltVariableData), NULL);

                newp[i] = p;

                memcpy(p, ret->func->param[i], sizeof(xrltVariableData));

                p->ownName = FALSE;
                p->ownXval = FALSE;
            }

            i = 0;
            j = 0;

            while (i < ret->func->paramLen && j < ret->paramLen) {
                k = xmlStrcmp(newp[i]->name, ret->param[j]->name);

                if (k == 0) {
                    tmp = newp[i]->declScope;
                    memcpy(newp[i], ret->param[j], sizeof(xrltVariableData));
                    newp[i]->declScope = tmp;

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
                xrltVariableFree(ret->param[i]);
            }

            xrltFree(ret->param);
        }

        ret->param = newp;
        ret->merged = NULL;
        ret->paramLen = ret->func->paramLen;
        ret->paramSize = ret->paramLen;

        ret->node = node;
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
                xrltVariableFree(a->param[i]);
            }

            xrltFree(a->param);
        }

        if (a->merged != NULL) {
            for (i = 0; i < a->paramLen; i++) {
                xrltVariableFree(a->merged[i]);
            }

            xrltFree(a->merged);
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

        xrltFree(tdata);
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
    size_t                      newScope;

    if (data == NULL) {
        NEW_CHILD(ctx, node, insert, "a");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, acomp->node, tdata, xrltApplyTransformingData*,
                    sizeof(xrltApplyTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltApplyTransformingFree;

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "tmp");
        tdata->ret = node;

        newScope = ++ctx->maxVarScope;

        for (i = 0; i < acomp->paramLen; i++) {
            acomp->param[i]->declScopePos = newScope;

            if (!xrltVariableTransform(ctx, acomp->param[i], insert, NULL)) {
                return FALSE;
            }
        }

        ctx->varScope = newScope;

        if (acomp->func->js) {
            if (!xrltJSApply(ctx, acomp->func->node, acomp->func->name, NULL,
                             0, insert))
            {
                return FALSE;
            }
        } else {
            COUNTER_INCREASE(ctx, tdata->node);

            TRANSFORM_SUBTREE(ctx, acomp->func->children, node);

            SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltApplyTransform, comp, insert,
                              tdata);
        }
    } else {
        tdata = (xrltApplyTransformingData *)data;

        ASSERT_NODE_DATA(tdata->ret, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltApplyTransform, comp, insert,
                              data);

            return TRUE;
        }

        COUNTER_DECREASE(ctx, tdata->node);

        REPLACE_RESPONSE_NODE(
            ctx, tdata->node, tdata->ret->children, acomp->node
        );
    }

    return TRUE;
}
