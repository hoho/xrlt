/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "function.h"
#include <libxslt/xsltutils.h>


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
    xmlNodePtr            tmp;
    xmlChar              *t;
    xrltVariableDataPtr   p;
    xrltVariableDataPtr  *newp;
    size_t                i;
    xrltBool              isTransformation;
    xmlHashTablePtr       table;
    const char           *what;
#ifndef __XRLT_NO_JAVASCRIPT__
    xmlNodePtr            tmp2;
    xmlBufferPtr          buf = NULL;
#endif

    XRLT_MALLOC(NULL, sheet, node, ret, xrltFunctionData*,
                sizeof(xrltFunctionData), NULL);

    if (xmlStrEqual(node->name, (const xmlChar *)"transformation")) {
        isTransformation = TRUE;
        what = "transformation";
    } else {
        isTransformation = FALSE;
        what = "function";
    }

    if (!isTransformation && sheet->funcs == NULL) {
        sheet->funcs = xmlHashCreate(20);

        if (sheet->funcs == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Functions hash creation failed\n");
            goto error;
        }
    } else if (isTransformation && sheet->transforms == NULL) {
        sheet->transforms = xmlHashCreate(20);

        if (sheet->transforms == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Transformations hash creation failed\n");
            goto error;
        }
    }

    table = isTransformation ? sheet->transforms : sheet->funcs;

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

    if (xmlValidateNCName(ret->name, 0)) {
        xrltTransformError(NULL, sheet, node, "Invalid %s name\n", what);
        goto error;
    }

    t = xmlGetProp(node, XRLT_ELEMENT_ATTR_TYPE);
    if (t != NULL) {
        if (xmlStrcasecmp(t, (xmlChar *)"javascript") == 0) {
            ret->js = TRUE;
        }

        // TODO: Throw an error for invalid type values.

        if (isTransformation) {
            if (xmlStrcasecmp(t, (xmlChar *)"xslt-stringify") == 0) {
                ret->transformation = XRLT_TRANSFORMATION_XSLT_STRINGIFY;
            } else if (xmlStrcasecmp(t, (xmlChar *)"xslt") == 0) {
                ret->transformation = XRLT_TRANSFORMATION_XSLT;
            } else if (xmlStrcasecmp(t, (xmlChar *)"json-stringify") == 0) {
                ret->transformation = XRLT_TRANSFORMATION_JSON_STRINGIFY;
            } else if (xmlStrcasecmp(t, (xmlChar *)"json-parse") == 0) {
                ret->transformation = XRLT_TRANSFORMATION_JSON_PARSE;
            } else if (xmlStrcasecmp(t, (xmlChar *)"xml-stringify") == 0) {
                ret->transformation = XRLT_TRANSFORMATION_XML_STRINGIFY;
            } else if (xmlStrcasecmp(t, (xmlChar *)"xml-parse") == 0) {
                ret->transformation = XRLT_TRANSFORMATION_XML_PARSE;
            } else {
                ret->transformation = XRLT_TRANSFORMATION_CUSTOM;
            }
        } else {
            ret->transformation = XRLT_TRANSFORMATION_FUNCTION;
        }

        xmlFree(t);
    }

    // We keep the last function declaration as a function.
    xmlHashRemoveEntry3(table, ret->name, NULL, NULL, xrltFunctionRemove);

    if (xmlHashAddEntry3(table, ret->name, NULL, NULL, ret)) {
        xrltTransformError(NULL, sheet, node, "Failed to add %s\n", what);
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
            newp = (xrltVariableDataPtr *)xmlRealloc(
                ret->param, sizeof(xrltVariableDataPtr) * ret->paramSize
            );

            if (newp == NULL) {
                xrltVariableFree(p);
                ERROR_OUT_OF_MEMORY(NULL, sheet, node);
                goto error;
            }

            ret->param = newp;
        }

        ret->param[ret->paramLen++] = p;

        t = xmlGetProp(tmp, XRLT_ELEMENT_ATTR_ASYNC);
        if (t != NULL) {
            p->sync = \
                xmlStrcasecmp(t, (const xmlChar *)"yes") == 0 ? FALSE : TRUE;
            xmlFree(t);
        } else {
            p->sync = TRUE;
        }

        if ((ret->transformation == XRLT_TRANSFORMATION_XSLT_STRINGIFY ||
             ret->transformation == XRLT_TRANSFORMATION_XSLT) && !p->sync)
        {
            xrltTransformError(NULL, sheet, p->node,
                               "XSLT transformations can't have async "
                               "params\n");
            goto error;
        }

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
#ifndef __XRLT_NO_JAVASCRIPT__
        tmp2 = tmp;

        while (tmp2 != NULL) {
            if (tmp2->type != XML_TEXT_NODE &&
                tmp2->type != XML_CDATA_SECTION_NODE)
            {
                ERROR_UNEXPECTED_ELEMENT(NULL, sheet, node);
                goto error;
            }
            tmp2 = tmp2->next;
        }

        buf = xmlBufferCreateSize(64);
        if (buf == NULL) {
            xrltTransformError(NULL, sheet, node, "Failed to create buffer\n");
            goto error;
        }

        xmlBufferCat(buf, (const xmlChar *)"\"use strict\";\n");

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
#endif
    } else {
        ret->children = tmp;
    }

    ret->node = node;

    if (ret->children != NULL &&
        ret->transformation != XRLT_TRANSFORMATION_FUNCTION &&
        ret->transformation != XRLT_TRANSFORMATION_CUSTOM)
    {
        ERROR_UNEXPECTED_ELEMENT(NULL, sheet, ret->children);
        goto error;
    }

    if (ret->transformation == XRLT_TRANSFORMATION_XSLT_STRINGIFY ||
        ret->transformation == XRLT_TRANSFORMATION_XSLT)
    {
        t = xmlGetProp(node, XRLT_ELEMENT_ATTR_SRC);
        if (t == NULL) {
            xrltTransformError(NULL, sheet, node, "No 'src' attrubute\n");
            goto error;
        }

        ret->xslt = xsltParseStylesheetFile(t);

        if (ret->xslt == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to load '%s' stylesheet\n", t);
            xmlFree(t);

            goto error;
        }

        xmlFree(t);
    }

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

            xmlFree(f->param);
        }

        if (f->xslt != NULL) {
            xsltFreeStylesheet(f->xslt);
        }

        xmlFree(comp);
    }
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
    const char           *what, *what2;

    if (prevcomp == NULL) {
        XRLT_MALLOC(NULL, sheet, node, ret, xrltApplyData*,
                    sizeof(xrltApplyData), NULL);

        if (xmlStrEqual(node->name, (const xmlChar *)"transform")) {
            ret->transform = TRUE;
        } else {
            ret->transform = FALSE;
        }

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
                newp = (xrltVariableDataPtr *)xmlRealloc(
                    ret->param, sizeof(xrltVariableDataPtr) * ret->paramSize
                );

                if (newp == NULL) {
                    xrltVariableFree(p);
                    ERROR_OUT_OF_MEMORY(NULL, sheet, node);
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

        if (ret->transform) {
            ret->children = tmp;

            if (!xrltCompileValue(sheet, node, tmp, XRLT_ELEMENT_ATTR_SELECT,
                                  NULL, NULL, FALSE, &ret->self))
            {
                goto error;
            }
        } else if (tmp != NULL) {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
            goto error;
        }
    } else {
        ret = (xrltApplyData *)prevcomp;

        if (ret->transform) {
            what = "transformation";
            what2 = "Transformation";
        } else {
            what = "function";
            what2 = "Function";
        }

        name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

        if (xmlValidateNCName(name, 0)) {
            if (name != NULL) { xmlFree(name); }
            xrltTransformError(NULL, sheet, node, "Invalid %s name\n", what);
            return NULL;
        }

        ret->func = (xrltFunctionData *)xmlHashLookup3(
            ret->transform ? sheet->transforms : sheet->funcs, name, NULL, NULL
        );

        xmlFree(name);

        if (ret->func == NULL) {
            xrltTransformError(
                NULL, sheet, node, "%s is not declared\n", what2
            );
            return NULL;
        }

        if (ret->func->paramLen > 0) {
            // Merge xrl:apply parameters to xrl:function parameters to get the
            // list of the parameters to process in the runtime.
            XRLT_MALLOC(NULL, sheet, node, newp, xrltVariableDataPtr*,
                        sizeof(xrltVariableDataPtr) * ret->func->paramLen,
                        NULL);

            ret->merged = newp;
            ret->mergedLen = ret->func->paramLen;

            for (i = 0; i < ret->func->paramLen; i++) {
                XRLT_MALLOC(NULL, sheet, node, p, xrltVariableDataPtr,
                            sizeof(xrltVariableData), NULL);

                newp[i] = p;

                memcpy(p, ret->func->param[i], sizeof(xrltVariableData));

                p->ownName = FALSE;
                p->ownJsname = FALSE;
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

                    newp[i]->sync = ret->func->param[i]->sync;

                    memset(ret->param[j], 0, sizeof(xrltVariableData));

                    i++;
                    j++;
                } else if (k < 0) {
                    i++;
                } else {
                    j++;
                }
            }

            for (i = 0; i < ret->func->paramLen; i++) {
                if (newp[i]->sync) {
                    ret->hasSyncParam = TRUE;
                }
            }
        } else {
            newp = NULL;
        }

        if (ret->param != NULL) {
            for (i = 0; i < ret->paramLen; i++) {
                xrltVariableFree(ret->param[i]);
            }

            xmlFree(ret->param);
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

            xmlFree(a->param);
        }

        if (a->merged != NULL) {
            for (i = 0; i < a->mergedLen; i++) {
                xrltVariableFree(a->merged[i]);
            }

            xmlFree(a->merged);
        }

        CLEAR_XRLT_VALUE(a->self);

        xmlFree(comp);
    }
}


static void
xrltApplyTransformingFree(void *data)
{
    xrltApplyTransformingData  *tdata;
    xmlNodePtr                  tmp;
    xmlDocPtr                   d;

    if (data != NULL) {
        tdata = (xrltApplyTransformingData *)data;

        if (tdata->paramNode) {
            tmp = tdata->paramNode->children;

            while (tmp != NULL) {
                d = (xmlDocPtr)tmp;

                tmp = tmp->next;

                xmlUnlinkNode((xmlNodePtr)d);
                xmlFreeDoc(d);
            }
        }

        xmlFree(tdata);
    }
}


static inline xrltBool
xrltXSLTTransform(xrltContextPtr ctx, xmlNodePtr src, xsltStylesheetPtr style,
                  xmlDocPtr doc, xrltBool tostr, xmlNodePtr insert)
{
    xmlDocPtr    res = NULL;
    xmlChar     *ret;
    int          len;
    xmlNodePtr   node;

    res = xsltApplyStylesheet(style, doc, NULL);

    if (res == NULL) {
        xrltTransformError(ctx, NULL, src,
                           "XSLT transformation failed\n");
        goto error;
    }

    if (tostr) {
        if (xsltSaveResultToString(&ret, &len, res, style) == 0) {
            node = xmlNewTextLen(ret, len);

            xmlFree(ret);

            if (node == NULL) {
                ERROR_CREATE_NODE(ctx, NULL, src);

                goto error;
            }

            if (xmlAddChild(insert, node) == NULL) {
                ERROR_ADD_NODE(ctx, NULL, src);

                goto error;
            }
        } else {
            xrltTransformError(ctx, NULL, src,
                               "Failed to stringify XSLT result\n");
            goto error;
        }
    } else if (res->children != NULL) {
        node = xmlDocCopyNodeList(insert->doc, res->children);

        if (node == NULL) {
            ERROR_CREATE_NODE(ctx, NULL, src);

            goto error;
        }

        if (xmlAddChildList(insert, node) == NULL) {
            ERROR_ADD_NODE(ctx, NULL, src);

            xmlFreeNodeList(node);

            goto error;
        }
    }

    xmlFreeDoc(res);

    return TRUE;

  error:
    if (res != NULL) { xmlFreeDoc(res); }

    return FALSE;
}


xrltBool
xrltApplyTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                   void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr                  node, tmp;
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

        if (acomp->hasSyncParam) {
            NEW_CHILD(ctx, tdata->paramNode, node, "p");
        }

        NEW_CHILD(ctx, tdata->retNode, node, "r");

        newScope = ++ctx->maxVarScope;
        node = ctx->var;

        for (i = 0; i < acomp->paramLen; i++) {
            acomp->param[i]->declScopePos = newScope;

            ctx->var = acomp->param[i]->sync ? tdata->paramNode : node;

            if (!xrltVariableTransform(ctx, acomp->param[i], insert, NULL)) {
                return FALSE;
            }
        }

        ctx->var = node;
        ctx->varScope = newScope;

        COUNTER_INCREASE(ctx, tdata->node);

        if (acomp->func->transformation != XRLT_TRANSFORMATION_FUNCTION) {
            tdata->self = xmlNewDoc(NULL);

            if (tdata->self == NULL) {
                ERROR_CREATE_NODE(ctx, NULL, acomp->node);

                return FALSE;
            }

            if (xmlAddChild(ctx->var, (xmlNodePtr)tdata->self) == NULL) {
                ERROR_ADD_NODE(ctx, NULL, acomp->node);

                xmlFreeDoc(tdata->self);

                return FALSE;
            }

            if (!xrltTransformByCompiledValue(ctx, &acomp->self,
                                              (xmlNodePtr)tdata->self, NULL))
            {
                return FALSE;
            }
        } else if (!acomp->hasSyncParam && !acomp->func->js) {
            TRANSFORM_SUBTREE(ctx, acomp->func->children, tdata->retNode);
        }

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltApplyTransform, comp, insert,
                          tdata);
    } else {
        tdata = (xrltApplyTransformingData *)data;

        if (!tdata->finalize) {
            if (tdata->self != NULL) {
                ASSERT_NODE_DATA(tdata->self, n);

                if (n->count > 0) {
                    SCHEDULE_CALLBACK(ctx, &n->tcb, xrltApplyTransform, comp,
                                      insert, tdata);
                    return TRUE;
                }
            }

            if (tdata->paramNode != NULL) {
                ASSERT_NODE_DATA(tdata->paramNode, n);

                if (n->count > 0) {
                    SCHEDULE_CALLBACK(ctx, &n->tcb, xrltApplyTransform, comp,
                                      insert, tdata);
                    return TRUE;
                }

                node = tdata->paramNode->children;

                while (node != NULL) {
                    tmp = node->next;

                    xmlUnlinkNode(node);

                    if (!xmlAddChild(ctx->var, node)) {
                        ERROR_ADD_NODE(ctx, NULL, acomp->node);

                        xmlFreeDoc((xmlDocPtr)node);

                        return FALSE;
                    }

                    node = tmp;
                }

                tdata->paramNode = NULL;

                if (!acomp->func->js) {
                    switch (acomp->func->transformation) {
                        case XRLT_TRANSFORMATION_FUNCTION:
                        case XRLT_TRANSFORMATION_CUSTOM:
                            ASSERT_NODE_DATA(tdata->retNode, n);

                            n->root = tdata->self;

                            TRANSFORM_SUBTREE(ctx, acomp->func->children,
                                              tdata->retNode);
                            break;

                        case XRLT_TRANSFORMATION_XSLT_STRINGIFY:
                            if (!xrltXSLTTransform(ctx, acomp->node,
                                                   acomp->func->xslt,
                                                   tdata->self, TRUE,
                                                   tdata->retNode))
                            {
                                return FALSE;
                            }

                            break;

                        case XRLT_TRANSFORMATION_XSLT:
                            if (!xrltXSLTTransform(ctx, acomp->node,
                                                   acomp->func->xslt,
                                                   tdata->self, FALSE,
                                                   tdata->retNode))
                            {
                                return FALSE;
                            }

                            break;

                        case XRLT_TRANSFORMATION_JSON_STRINGIFY:
                        case XRLT_TRANSFORMATION_JSON_PARSE:
                        case XRLT_TRANSFORMATION_XML_STRINGIFY:
                        case XRLT_TRANSFORMATION_XML_PARSE:
                            break;
                    }

                    SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltApplyTransform, comp,
                                      insert, tdata);
                    return TRUE;
                }
            }

            ASSERT_NODE_DATA(tdata->retNode, n);

            if (acomp->func->js) {
#ifndef __XRLT_NO_JAVASCRIPT__
                ctx->varContext = acomp->func->node;

                if (!xrltJSApply(ctx, acomp->func->node, acomp->func->name,
                                 acomp->param, acomp->paramLen, tdata->retNode))
                {
                    return FALSE;
                }
#endif
            } else {
                if (n->count > 0) {
                    SCHEDULE_CALLBACK(ctx, &n->tcb, xrltApplyTransform, comp,
                                      insert, data);

                    return TRUE;
                }
            }

            COUNTER_DECREASE(ctx, tdata->node);

            if (n->count > 0) {
                tdata->finalize = TRUE;

                SCHEDULE_CALLBACK(ctx, &n->tcb, xrltApplyTransform, comp,
                                  insert, data);

                return TRUE;
            }
        }

        REPLACE_RESPONSE_NODE(
            ctx, tdata->node, tdata->retNode->children, acomp->node
        );
    }

    return TRUE;
}
