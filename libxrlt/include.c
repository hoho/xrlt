#include "transform.h"
#include "include.h"


static inline xrltBool
xrltIncludeStrNodeXPath(xrltRequestsheetPtr sheet, xmlNodePtr node,
                        xrltBool tostring,  xmlChar **val, xmlNodePtr *nval,
                        xmlXPathCompExprPtr *xval)
{
    xmlChar              *value;
    xmlXPathCompExprPtr   expr;

    value = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);

    if (value != NULL && node->children != NULL) {
        xrltTransformError(
            NULL, sheet, node,
            "Element should be empty to have 'select' attribute\n"
        );

        xmlFree(value);
        return FALSE;
    }

    if (node->children != NULL) {
        if (!tostring || xrltHasXRLTElement(node->children)) {
            *nval = node->children;
        } else {
            *val = xmlXPathCastNodeToString(node);
        }
    }

    if (value != NULL) {
        expr = xmlXPathCompile(value);

        xmlFree(value);

        if (expr == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile expression\n");
            return FALSE;
        }

        *xval = expr;
    }

    return TRUE;
}


static inline xrltCompiledIncludeParamPtr
xrltIncludeParamCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                        xrltBool header)
{
    xmlNodePtr                    ntype = NULL;
    xmlNodePtr                    nname = NULL;
    xmlNodePtr                    nvalue = NULL;
    xmlNodePtr                    ntest = NULL;
    xmlNodePtr                    tmp;
    xrltBool                      other = FALSE;
    xmlNodePtr                    dup = NULL;
    xmlChar                      *type = NULL;
    xmlChar                      *name = NULL;
    xmlChar                      *value = NULL;
    xmlChar                      *test = NULL;
    xrltCompiledIncludeParamPtr   ret = NULL;
    xrltNodeDataPtr               n;

    tmp = node->children;

    while (tmp != NULL) {
        if (tmp->ns != NULL && xmlStrEqual(tmp->ns->href, XRLT_NS)) {
            if (xmlStrEqual(tmp->name, XRLT_ELEMENT_TYPE)) {
                if (ntype != NULL) {
                    dup = tmp;
                    break;
                }
                ntype = tmp;
            } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_NAME)) {
                if (nname != NULL) {
                    dup = tmp;
                    break;
                }
                nname = tmp;
            } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_VALUE)) {
                if (nvalue != NULL) {
                    dup = tmp;
                    break;
                }
                nvalue = tmp;
            } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_TEST)) {
                if (ntest != NULL) {
                    dup = tmp;
                    break;
                }
                ntest = tmp;
            } else {
                other = TRUE;
            }
        } else {
            other = TRUE;
        }

        tmp = tmp->next;
    }

    if (dup != NULL) {
        xrltTransformError(NULL, sheet, dup, "Duplicate element\n");
        return NULL;
    }

    if ((ntype != NULL || nname != NULL || nvalue != NULL || ntest != NULL) &&
        other)
    {
        xrltTransformError(
            NULL, sheet, node,
            "Can't mix <xrl:test>, <xrl:type>, <xrl:name> and <xrl:value> "
            "elements with other elements\n"
        );
        return NULL;
    }

    type = xmlGetProp(node, XRLT_ELEMENT_ATTR_TYPE);
    if (!header) {
        if (type != NULL && ntype != NULL) {
            xrltTransformError(
                NULL, sheet, node,
                "Element shouldn't have <xrl:type> to have 'type' attribute\n"
            );
            goto error;
        }
    } else if (type != NULL || ntype != NULL) {
        xrltTransformError(NULL, sheet, node, "Header shouldn't have type\n");
        goto error;
    }

    name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);
    if (name != NULL && nname != NULL) {
        xrltTransformError(
            NULL, sheet, node,
            "Element shouldn't have <xrl:name> to have 'name' attribute\n"
        );
        goto error;
    }

    value = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);
    if (value != NULL && nvalue != NULL) {
        xrltTransformError(
            NULL, sheet, node,
            "Element shouldn't have <xrl:value> to have 'select' attribute\n"
        );
        goto error;
    }

    test = xmlGetProp(node, XRLT_ELEMENT_ATTR_TEST);
    if (test != NULL && ntest != NULL) {
        xrltTransformError(
            NULL, sheet, node,
            "Element shouldn't have <xrl:test> to have 'test' attribute\n"
        );
        goto error;
    }

    if (header && name == NULL && nname == NULL) {
        xrltTransformError(NULL, sheet, node, "Header has no name\n");
        goto error;
    }

    if (!header &&
        name == NULL && nname == NULL && value == NULL && nvalue == NULL)
    {
        xrltTransformError(NULL, sheet, node,
                           "Parameter has no name and no value\n");
        goto error;
    }

    if (value != NULL && other) {
        xrltTransformError(
            NULL, sheet, node,
            "Element should be empty to have 'select' attribute\n"
        );
        goto error;
    }

    ret = xrltMalloc(sizeof(xrltCompiledIncludeParam));

    if (ret == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "xrltIncludeParamCompile: Out of memory\n");
        goto error;
    }

    memset(ret, 0, sizeof(xrltCompiledIncludeParam));

    if (type != NULL) {
        // We have type attribute for a parameter. It should be 'body' or
        // 'querystring'. Just compare the value 'body' and treat it as
        // 'querystring' otherwise.
        ret->body = xmlStrEqual(type, (const xmlChar *)"body") ? TRUE : FALSE;
        xmlFree(type);
        type = NULL;
    }

    if (ntype != NULL) {
        // The same with previous one, but we have an <xrl:type> node.
        if (!xrltIncludeStrNodeXPath(sheet, ntype, TRUE, &type, &ret->nbody,
                                     &ret->xbody))
        {
            goto error;
        }

        if (type != NULL) {
            ret->body = \
                xmlStrEqual(type, (const xmlChar *)"body") ? TRUE : FALSE;
            xmlFree(type);
            type = NULL;
        }

        ASSERT_NODE_DATA_GOTO(ntype, n);
        n->xrlt = TRUE;
    }

    if (name != NULL) {
        ret->name = name;
        name = NULL;
    }

    if (nname != NULL) {
        if (!xrltIncludeStrNodeXPath(sheet, nname, TRUE, &ret->name,
                                     &ret->nname, &ret->xname))
        {
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(nname, n);
        n->xrlt = TRUE;
    }

    if (value != NULL) {
        ret->xvalue = xmlXPathCompile(value);

        xmlFree(value);
        value = NULL;

        if (ret->xvalue == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile select expression\n");
            goto error;
        }
    }

    if (nvalue != NULL) {
        if (!xrltIncludeStrNodeXPath(sheet, nvalue, TRUE, &ret->value,
                                     &ret->nvalue, &ret->xvalue))
        {
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(nvalue, n);
        n->xrlt = TRUE;
    }


    if (test != NULL) {
        ret->xtest = xmlXPathCompile(test);

        xmlFree(test);
        test = NULL;

        if (ret->xtest == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile test expression\n");
            goto error;
        }
    }

    if (ntest != NULL) {
        if (!xrltIncludeStrNodeXPath(sheet, ntest, TRUE, &test,
                                     &ret->ntest, &ret->xtest))
        {
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(ntest, n);
        n->xrlt = TRUE;

        if (test != NULL) {
            ret->test = xmlStrEqual(test, (const xmlChar *)"") ? FALSE : TRUE;
            xmlFree(test);
            test = NULL;
        }
    }

    return ret;

  error:
    if (type != NULL) { xmlFree(type); }
    if (name != NULL) { xmlFree(name); }
    if (value != NULL) { xmlFree(value); }
    if (test != NULL) { xmlFree(test); }

    if (ret != NULL) {
        if (ret->xtest != NULL) { xmlXPathFreeCompExpr(ret->xtest); }

        if (ret->xbody != NULL) { xmlXPathFreeCompExpr(ret->xbody); }

        if (ret->name != NULL) { xmlFree(ret->name); }
        if (ret->xname != NULL) { xmlXPathFreeCompExpr(ret->xname); }

        if (ret->value != NULL) { xmlFree(ret->value); }
        if (ret->xvalue != NULL) { xmlXPathFreeCompExpr(ret->xvalue); }

        xrltFree(ret);
    }

    return NULL;
}


void *
xrltIncludeCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltCompiledIncludeData      *ret = NULL;
    xmlNodePtr                    tmp;
    xrltNodeDataPtr               n;
    xrltCompiledIncludeParamPtr   param;

    ret = \
        (xrltCompiledIncludeData *)xrltMalloc(sizeof(xrltCompiledIncludeData));

    if (ret == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "xrltIncludeCompile: Out of memory\n");
        return NULL;
    }

    memset(ret, 0, sizeof(xrltCompiledIncludeData));

    tmp = node->children;

    while (tmp != NULL) {
        if (tmp->ns == NULL || !xmlStrEqual(tmp->ns->href, XRLT_NS)) {
            xrltTransformError(NULL, sheet, tmp, "Unknown element\n");
            goto error;
        }

        if (xmlStrEqual(tmp->name, XRLT_ELEMENT_HREF)) {
            if (!xrltIncludeStrNodeXPath(sheet, tmp, TRUE, &ret->href,
                                         &ret->nhref, &ret->xhref))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_HEADER)) {
            param = xrltIncludeParamCompile(sheet, tmp, TRUE);

            if (param == NULL) {
                goto error;
            }

            if (ret->fheader == NULL) {
                ret->fheader = param;
            } else {
                ret->lheader->next = param;
            }
            ret->lheader = param;
            ret->headerCount++;
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_PARAM)) {
            param = xrltIncludeParamCompile(sheet, tmp, FALSE);

            if (param == NULL) {
                goto error;
            }

            if (ret->fparam == NULL) {
                ret->fparam = param;
            } else {
                ret->lparam->next = param;
            }
            ret->lparam = param;
            ret->paramCount++;
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_BODY)) {
            if (!xrltIncludeStrNodeXPath(sheet, tmp, TRUE, &ret->body,
                                         &ret->nbody, &ret->xbody))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_SUCCESS)) {
            if (!xrltIncludeStrNodeXPath(sheet, tmp, FALSE, NULL,
                                         &ret->nsuccess, &ret->xsuccess))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_FAILURE)) {
            if (!xrltIncludeStrNodeXPath(sheet, tmp, FALSE, NULL,
                                         &ret->nfailure, &ret->xfailure))
            {
                goto error;
            }
        } else {
            xrltTransformError(NULL, sheet, tmp, "Unknown element\n");
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(tmp, n);

        n->xrlt = TRUE;

        tmp = tmp->next;
    }

    if (ret->href == NULL && ret->nhref == NULL && ret->xhref == NULL) {
        xrltTransformError(NULL, sheet, node, "No href\n");
        goto error;
    }

    ret->node = node;

    return ret;

  error:
    xrltIncludeFree(ret);

    return NULL;
}


void
xrltIncludeFree(void *comp)
{
    xrltCompiledIncludeData      *ret = (xrltCompiledIncludeData *)comp;
    xrltCompiledIncludeParamPtr   param, tmp;

    if (comp != NULL) {
        if (ret->href != NULL) { xmlFree(ret->href); }
        if (ret->xhref != NULL) { xmlXPathFreeCompExpr(ret->xhref); }

        if (ret->body != NULL) { xmlFree(ret->body); }
        if (ret->xbody != NULL) { xmlXPathFreeCompExpr(ret->xbody); }

        if (ret->xsuccess != NULL) { xmlXPathFreeCompExpr(ret->xsuccess); }

        if (ret->xfailure != NULL) { xmlXPathFreeCompExpr(ret->xfailure); }

        param = ret->fheader;
        while (param != NULL) {
            if (param->xtest != NULL) { xmlXPathFreeCompExpr(param->xtest); }

            if (param->name != NULL) { xmlFree(param->name); }
            if (param->xname != NULL) { xmlXPathFreeCompExpr(param->xname); }

            if (param->value != NULL) { xmlFree(param->value); }
            if (param->xvalue != NULL) { xmlXPathFreeCompExpr(param->xvalue); }

            tmp = param->next;
            xrltFree(param);
            param = tmp;
        }

        param = ret->fparam;
        while (param != NULL) {
            if (param->xtest != NULL) { xmlXPathFreeCompExpr(param->xtest); }

            if (param->xbody != NULL) { xmlXPathFreeCompExpr(param->xbody); }

            if (param->name != NULL) { xmlFree(param->name); }
            if (param->xname != NULL) { xmlXPathFreeCompExpr(param->xname); }

            if (param->value != NULL) { xmlFree(param->value); }
            if (param->xvalue != NULL) { xmlXPathFreeCompExpr(param->xvalue); }

            tmp = param->next;
            xrltFree(param);
            param = tmp;
        }

        xrltFree(ret);
    }
}


static void
xrltIncludeTransformingFree(void *data)
{
    if (data != NULL) {
        xrltTransformingIncludeData  *tdata;
        size_t                        i;

        tdata = (xrltTransformingIncludeData *)data;

        printf("jjjjjjjjjj %s\n", (char *)tdata->href);

        if (tdata->href != NULL) { xmlFree(tdata->href); }
        if (tdata->body != NULL) { xmlFree(tdata->body); }

        for (i = 0; i < tdata->headerCount; i++) {
            if (tdata->header[i].name != NULL) {
                printf("jjjjjjjjjj %s\n", (char *)tdata->header[i].name);
                xmlFree(tdata->header[i].name);
            }

            if (tdata->header[i].value != NULL) {
                printf("jjjjjjjjjj %s\n", (char *)tdata->header[i].value);
                xmlFree(tdata->header[i].value);
            }
        }

        for (i = 0; i < tdata->paramCount; i++) {
            if (tdata->param[i].name != NULL) {
                printf("jjjjjjjjjj %s\n", (char *)tdata->param[i].name);
                xmlFree(tdata->param[i].name);
            }

            if (tdata->param[i].value != NULL) {
                printf("jjjjjjjjjj %s\n", (char *)tdata->param[i].value);
                xmlFree(tdata->param[i].value);
            }
        }

        xrltFree(tdata);
    }
}


static xrltBool
xrltIncludeParamTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                          void *data)
{
    xmlNodePtr        node = (xmlNodePtr)comp;
    xrltNodeDataPtr   n;
    xmlChar          *ret;

    ASSERT_NODE_DATA(node, n);

    if (n->count > 0) {
        // Node is not ready.
        SCHEDULE_CALLBACK(ctx, &n->tcb, xrltIncludeParamTransform, comp,
                          insert, data);
    } else {
        *((xmlChar **)data) = xmlXPathCastNodeToString(node);
    }

    return TRUE;
}


static inline xrltBool
xrltIncludeTransformToString(xrltContextPtr ctx, xmlNodePtr insert,
                             xmlChar *val, xmlNodePtr nval,
                             xmlXPathCompExprPtr xval, xmlChar **ret)
{
    if (val != NULL) {
        printf("98887878787 %s\n", (char *)val);
        *ret = xmlStrdup(val);
    } else if (nval != NULL) {
        xmlNodePtr           node;

        node = xmlNewChild(insert, NULL, (const xmlChar *)"tmp", NULL);

        if (node == NULL) {
            xrltTransformError(ctx, NULL, NULL,
                               "Failed to create include subelement\n");
            return FALSE;
        }

        if (!xrltElementTransform(ctx, nval, node)) {
            return FALSE;
        }

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeParamTransform,
                          (void *)node, insert, (void *)ret);
    } else if (xval != NULL) {

    }

    return TRUE;
}


xrltBool
xrltIncludeTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                     void *data)
{
    xrltCompiledIncludeData      *icomp = (xrltCompiledIncludeData *)comp;
    xmlNodePtr                    node;
    xrltNodeDataPtr               n;
    xrltTransformingIncludeData  *tdata;
    xrltCompiledIncludeParamPtr   p;
    size_t                        i;

    if (data == NULL) {
        // First call.
        node = xmlNewChild(insert, NULL, (const xmlChar *)"include", NULL);

        if (node == NULL) {
            xrltTransformError(ctx, NULL, icomp->node,
                               "Failed to create include element\n");
            return FALSE;
        }

        ASSERT_NODE_DATA(node, n);

        tdata = (xrltTransformingIncludeData *)xrltMalloc(
            sizeof(xrltTransformingIncludeData) +
            sizeof(xrltTransformingParam) * icomp->headerCount +
            sizeof(xrltTransformingParam) * icomp->paramCount
        );

        if (tdata == NULL) {
            xrltTransformError(ctx, NULL, icomp->node,
                               "xrltIncludeTransform: Out of memory\n");
            return FALSE;
        }

        memset(tdata, 0, sizeof(xrltTransformingIncludeData) +
                         sizeof(xrltTransformingParam) * icomp->headerCount +
                         sizeof(xrltTransformingParam) * icomp->paramCount);

        tdata->header = (xrltTransformingParam *)(tdata + 1);

        if (icomp->paramCount > 0) {
            tdata->paramCount = icomp->paramCount;
            tdata->param = tdata->header + icomp->headerCount;
        }

        if (icomp->headerCount == 0) {
            tdata->header = NULL;
        } else {
            tdata->headerCount = icomp->headerCount;
        }

        n->data = tdata;
        n->free = xrltIncludeTransformingFree;

        // Mark node as not ready.
        //xrltNotReadyCounterIncrease(ctx, node);

        if (!xrltIncludeTransformToString(ctx, node, icomp->href, icomp->nhref,
                                          icomp->xhref, &tdata->href))
        {
            return FALSE;
        }

        p = icomp->fheader;

        for (i = 0; i < tdata->headerCount; i++) {
            if (!xrltIncludeTransformToString(ctx, node, p->name, p->nname,
                                              p->xname, &tdata->header[i].name))
            {
                return FALSE;
            }

            if (!xrltIncludeTransformToString(ctx, node, p->value, p->nvalue,
                                              p->xvalue,
                                              &tdata->header[i].value))
            {
                return FALSE;
            }
        }

        p = icomp->fparam;

        for (i = 0; i < tdata->paramCount; i++) {
            if (!xrltIncludeTransformToString(ctx, node, p->name, p->nname,
                    p->xname, &tdata->param[i].name))
            {
                return FALSE;
            }

            if (!xrltIncludeTransformToString(ctx, node, p->value, p->nvalue,
                    p->xvalue,
                    &tdata->param[i].value))
            {
                return FALSE;
            }
        }

        // Schedule the next call.
        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform, comp, insert,
                          (void *)node);
    } else {
        node = (xmlNodePtr)data;

        ASSERT_NODE_DATA(node, n);

        if (n->count > 0) {
            //xrltNotReadyCounterDecrease(ctx, node);

            //if (n->count > 0) {
                SCHEDULE_CALLBACK(ctx, &n->tcb, xrltIncludeTransform, comp,
                                  insert, (void *)node);
                return TRUE;
            //}
        }



    }

    return TRUE;
}
