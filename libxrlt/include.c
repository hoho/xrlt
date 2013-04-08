#include "transform.h"
#include "include.h"


static inline xrltBool
xrltIncludeFillVariables(xrltRequestsheetPtr sheet, xmlNodePtr node,
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

        if (xmlStrEqual(tmp->name, (const xmlChar *)"href")) {
            if (!xrltIncludeFillVariables(sheet, tmp, TRUE, &ret->href,
                                          &ret->nhref, &ret->xhref))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, (const xmlChar *)"with-header")) {
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
        } else if (xmlStrEqual(tmp->name, (const xmlChar *)"with-param")) {
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
        } else if (xmlStrEqual(tmp->name, (const xmlChar *)"with-body")) {
            if (!xrltIncludeFillVariables(sheet, tmp, TRUE, &ret->body,
                                          &ret->nbody, &ret->xbody))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, (const xmlChar *)"success")) {
            if (!xrltIncludeFillVariables(sheet, tmp, FALSE, NULL,
                                          &ret->nsuccess, &ret->xsuccess))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, (const xmlChar *)"failure")) {
            if (!xrltIncludeFillVariables(sheet, tmp, FALSE, NULL,
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

    //level = xmlGetProp(node, (const xmlChar *)"level");

    return ret;

  error:
    xrltIncludeFree(ret);

    return NULL;
}


void
xrltIncludeFree(void *comp)
{
    xrltCompiledIncludeData      *ret = (xrltCompiledIncludeData *)comp;
    xrltCompiledIncludeParamPtr   param;

    if (comp != NULL) {
        if (ret->href != NULL) { xmlFree(ret->href); }
        if (ret->xhref != NULL) { xmlXPathFreeCompExpr(ret->xhref); }

        if (ret->body != NULL) { xmlFree(ret->body); }
        if (ret->xbody != NULL) { xmlXPathFreeCompExpr(ret->xbody); }

        if (ret->xsuccess != NULL) { xmlXPathFreeCompExpr(ret->xsuccess); }

        if (ret->xfailure != NULL) { xmlXPathFreeCompExpr(ret->xfailure); }

        param = ret->fheader;
        while (param != NULL) {
            if (param->name != NULL) { xmlFree(param->name); }
            if (param->xname != NULL) { xmlXPathFreeCompExpr(param->xname); }

            if (param->value != NULL) { xmlFree(param->value); }
            if (param->xvalue != NULL) { xmlXPathFreeCompExpr(param->xvalue); }

            param = param->next;
        }

        param = ret->fparam;
        while (param != NULL) {
            if (param->xbody != NULL) { xmlXPathFreeCompExpr(param->xbody); }

            if (param->name != NULL) { xmlFree(param->name); }
            if (param->xname != NULL) { xmlXPathFreeCompExpr(param->xname); }

            if (param->value != NULL) { xmlFree(param->value); }
            if (param->xvalue != NULL) { xmlXPathFreeCompExpr(param->xvalue); }

            param = param->next;
        }

        xrltFree(comp);
    }
}


xrltBool
xrltIncludeTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                     void *data)
{
    return TRUE;
}
