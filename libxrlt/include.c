#include <ctype.h>
#include "transform.h"
#include "include.h"


static inline char
xrltFromHex(char ch)
{
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}


static inline char
xrltToHex(char code)
{
    static char hex[] = "0123456789ABCDEF";
    return hex[code & 15];
}


static inline size_t
xrltURLEncode(char *str, char *out)
{
    char *pstr = str;
    char *pbuf = out;

    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' ||
            *pstr == '~')
        {
            *pbuf++ = *pstr;
        } else if (*pstr == ' ') {
            *pbuf++ = '+';
        } else {
            *pbuf++ = '%';
            *pbuf++ = xrltToHex(*pstr >> 4);
            *pbuf++ = xrltToHex(*pstr & 15);
        }

        pstr++;
    }

    *pbuf = '\0';

    return pbuf - out;
}


static inline char *
xrltURLDecode(char *str)
{
    char *pstr = str;
    char *buf = (char *)xrltMalloc(strlen(str) + 1);
    char *pbuf = buf;

    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf++ = xrltFromHex(pstr[1]) << 4 | xrltFromHex(pstr[2]);
                pstr += 2;
            }
        } else if (*pstr == '+') {
            *pbuf++ = ' ';
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
    }

    *pbuf = '\0';

    return buf;
}


static inline xrltCompiledIncludeParamPtr
xrltIncludeParamCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                        xrltBool header)
{
    xmlNodePtr                    ntype = NULL;
    xmlNodePtr                    nname = NULL;
    xmlNodePtr                    nval = NULL;
    xmlNodePtr                    ntest = NULL;
    xmlNodePtr                    tmp;
    xrltBool                      other = FALSE;
    xmlNodePtr                    dup = NULL;
    xmlChar                      *type = NULL;
    xmlChar                      *name = NULL;
    xmlChar                      *val = NULL;
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
                if (nval != NULL) {
                    dup = tmp;
                    break;
                }
                nval = tmp;
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

    if ((ntype != NULL || nname != NULL || nval != NULL || ntest != NULL) &&
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

    val = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);
    if (val != NULL && nval != NULL) {
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
        name == NULL && nname == NULL && val == NULL && nval == NULL)
    {
        xrltTransformError(NULL, sheet, node,
                           "Parameter has no name and no value\n");
        goto error;
    }

    if (val != NULL && other) {
        xrltTransformError(
            NULL, sheet, node,
            "Element should be empty to have 'select' attribute\n"
        );
        goto error;
    }

    XRLT_MALLOC(ret, xrltCompiledIncludeParamPtr,
                sizeof(xrltCompiledIncludeParam), "xrltIncludeParamCompile",
                NULL);

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

    if (val != NULL) {
        ret->xval = xmlXPathCompile(val);

        xmlFree(val);
        val = NULL;

        if (ret->xval == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Failed to compile select expression\n");
            goto error;
        }
    }

    if (nval == NULL && node->children != NULL) {
        nval = node;
    }

    if (nval != NULL) {
        if (!xrltIncludeStrNodeXPath(sheet, nval, TRUE, &ret->val,
                                     &ret->nval, &ret->xval))
        {
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(nval, n);
        n->xrlt = TRUE;
    }

    if (test == NULL && ntest == NULL) {
        // Default value for test is TRUE.
        ret->test = TRUE;
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
    if (val != NULL) { xmlFree(val); }
    if (test != NULL) { xmlFree(test); }

    if (ret != NULL) {
        if (ret->xtest != NULL) { xmlXPathFreeCompExpr(ret->xtest); }

        if (ret->xbody != NULL) { xmlXPathFreeCompExpr(ret->xbody); }

        if (ret->name != NULL) { xmlFree(ret->name); }
        if (ret->xname != NULL) { xmlXPathFreeCompExpr(ret->xname); }

        if (ret->val != NULL) { xmlFree(ret->val); }
        if (ret->xval != NULL) { xmlXPathFreeCompExpr(ret->xval); }

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

    XRLT_MALLOC(ret, xrltCompiledIncludeData*,
                sizeof(xrltCompiledIncludeData), "xrltIncludeCompile", NULL);

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

            if (param->val != NULL) { xmlFree(param->val); }
            if (param->xval != NULL) { xmlXPathFreeCompExpr(param->xval); }

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

            if (param->val != NULL) { xmlFree(param->val); }
            if (param->xval != NULL) { xmlXPathFreeCompExpr(param->xval); }

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
        xrltIncludeTransformingData  *tdata;
        size_t                        i;

        tdata = (xrltIncludeTransformingData *)data;

        printf("jjjjjjjjjj %s\n", (char *)tdata->href);

        if (tdata->href != NULL) { xmlFree(tdata->href); }
        if (tdata->body != NULL) { xmlFree(tdata->body); }

        for (i = 0; i < tdata->headerCount; i++) {
            if (tdata->header[i].name != NULL) {
                printf("aaaaaaaaaaa %s\n", (char *)tdata->header[i].name);
                xmlFree(tdata->header[i].name);
            }

            if (tdata->header[i].val != NULL) {
                printf("bbbbbbbbbbb %s\n", (char *)tdata->header[i].val);
                xmlFree(tdata->header[i].val);
            }
        }

        for (i = 0; i < tdata->paramCount; i++) {
            if (tdata->param[i].name != NULL) {
                printf("ccccccccccc %s\n", (char *)tdata->param[i].name);
                xmlFree(tdata->param[i].name);
            }

            if (tdata->param[i].val != NULL) {
                printf("ddddddddddd %s\n", (char *)tdata->param[i].val);
                xmlFree(tdata->param[i].val);
            }
        }

        xrltFree(tdata);
    }
}


static xrltBool
xrltIncludeSetStringResult(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                           void *data)
{
    xmlNodePtr        node = (xmlNodePtr)comp;
    xrltNodeDataPtr   n;

    ASSERT_NODE_DATA(node, n);

    if (n->count > 0) {
        // Node is not ready.
        SCHEDULE_CALLBACK(
            ctx, &n->tcb, xrltIncludeSetStringResult, comp, insert, data
        );
    } else {
        *((xmlChar **)data) = xmlXPathCastNodeToString(node);
    }

    return TRUE;
}


static inline xrltBool
xrltIncludeSetStringResultByXPath(xrltContextPtr ctx, void *comp,
                                  xmlNodePtr insert, void *data)
{
    xmlXPathObjectPtr  v;

    if (!xrltXPathEval(ctx, NULL, insert, (xmlXPathCompExprPtr)comp, &v)) {
        return FALSE;
    }

    if (v == NULL) {
        // Some variables are not ready.
        xrltNodeDataPtr   n;

        ASSERT_NODE_DATA(ctx->xpathWait, n);

        SCHEDULE_CALLBACK(
            ctx, &n->tcb, xrltIncludeSetStringResultByXPath, comp, insert, data
        );
    } else {
        *((xmlChar **)data) = xmlXPathCastToString(v);

        xmlXPathFreeObject(v);

        COUNTER_DECREASE(ctx, insert);
    }

    return TRUE;
}


static inline xrltBool
xrltIncludeTransformToString(xrltContextPtr ctx, xmlNodePtr insert,
                             xmlChar *val, xmlNodePtr nval,
                             xmlXPathCompExprPtr xval, xmlChar **ret)
{
    if (val != NULL) {
        *ret = xmlStrdup(val);
    } else if (nval != NULL) {
        xmlNodePtr   node;

        NEW_CHILD(ctx, node, insert, "tmp");

        TRANSFORM_SUBTREE(ctx, nval, node);

        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltIncludeSetStringResult, node, insert, ret
        );
    } else if (xval != NULL) {
        COUNTER_INCREASE(ctx, insert);

        return xrltIncludeSetStringResultByXPath(ctx, xval, insert, ret);
    }

    return TRUE;
}


static xrltBool
xrltIncludeSetBooleanResult(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                            void *data)
{
    xmlNodePtr        node = (xmlNodePtr)comp;
    xrltNodeDataPtr   n;
    xmlChar          *ret;

    ASSERT_NODE_DATA(node, n);

    if (n->count > 0) {
        // Node is not ready.
        SCHEDULE_CALLBACK(
            ctx, &n->tcb, xrltIncludeSetBooleanResult, comp, insert, data
        );
    } else {
        ret = xmlXPathCastNodeToString(node);
        *((xrltBool *)data) = xmlXPathCastStringToBoolean(ret) ? TRUE : FALSE;
        xmlFree(ret);
    }

    return TRUE;
}


static inline xrltBool
xrltIncludeSetBooleanResultByXPath(xrltContextPtr ctx, void *comp,
                                   xmlNodePtr insert, void *data)
{
    xmlXPathObjectPtr  v;

    if (!xrltXPathEval(ctx, NULL, insert, (xmlXPathCompExprPtr)comp, &v)) {
        return FALSE;
    }

    if (v == NULL) {
        // Some variables are not ready.
        xrltNodeDataPtr   n;

        ASSERT_NODE_DATA(ctx->xpathWait, n);

        SCHEDULE_CALLBACK(
            ctx, &n->tcb, xrltIncludeSetBooleanResultByXPath, comp, insert, data
        );
    } else {
        *((xrltBool *)data) = xmlXPathCastToBoolean(v) ? TRUE : FALSE;

        xmlXPathFreeObject(v);

        COUNTER_DECREASE(ctx, insert);
    }

    return TRUE;
}


static inline xrltBool
xrltIncludeTransformToBoolean(xrltContextPtr ctx, xmlNodePtr insert,
                              xrltBool val, xmlNodePtr nval,
                              xmlXPathCompExprPtr xval, xrltBool *ret)
{
    if (nval != NULL) {
        xmlNodePtr   node;

        NEW_CHILD(ctx, node, insert, "tmp");

        TRANSFORM_SUBTREE(ctx, nval, node);

        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltIncludeSetBooleanResult, node, insert, ret
        );
    } else if (xval != NULL) {
        COUNTER_INCREASE(ctx, insert);

        return xrltIncludeSetBooleanResultByXPath(ctx, xval, insert, ret);
    } else {
        *ret = val;
    }

    return TRUE;
}


#define TRANSFORM_TO_STRING(ctx, node, val, nval, xval, ret) {                \
    if (!xrltIncludeTransformToString(ctx, node, val, nval, xval, ret)) {     \
        return FALSE;                                                         \
    }                                                                         \
}


#define TRANSFORM_TO_BOOLEAN(ctx, node, val, nval, xval, ret) {               \
    if (!xrltIncludeTransformToBoolean(ctx, node, val, nval, xval, ret)) {    \
        return FALSE;                                                         \
    }                                                                         \
}


static xrltBool
xrltIncludeSubrequestHeader(xrltContextPtr ctx, xrltTransformValue *value,
                            void *data)
{
    return TRUE;
}


static xrltBool
xrltIncludeSubrequestBody(xrltContextPtr ctx, xrltTransformValue *value,
                          void *data)
{
    return TRUE;
}


static inline xrltBool
xrltIncludeAddSubrequest(xrltContextPtr ctx, xrltIncludeTransformingData *data)
{
    size_t           id;
    size_t           i, blen, qlen;
    xrltHeaderList   header;
    xrltString       href;
    xrltString       query;
    xrltString       body;
    char            *bp, *qp;
    char            *ebody = NULL;
    char            *equery = NULL;
    xrltBool         ret = TRUE;

    xrltHeaderListInit(&header);

    for (i = 0; i < data->headerCount; i++) {
        if (data->header[i].test) {
            // Use query and body variables as name and value to reduce
            // declared variables count.
            xrltStringSet(&query, (char *)data->header[i].name);
            xrltStringSet(&body, (char *)data->header[i].val);

            if (!xrltHeaderListPush(&header, &query, &body)) {
                xrltTransformError(ctx, NULL, data->inode,
                                   "xrltIncludeAddSubrequest: Out of memory\n");
                ret = FALSE;
                goto error;
            }
        }
    }

    xrltStringSet(&href, (char *)data->href);

    memset(&query, 0, sizeof(xrltString));
    memset(&body, 0, sizeof(xrltString));

    blen = 0;
    qlen = 0;

    for (i = 0; i < data->paramCount; i++) {
        if (data->param[i].test) {
            if (data->param[i].body) {
                blen += xmlStrlen(data->param[i].name);
                blen += xmlStrlen(data->param[i].val);
                blen += 2;
            } else {
                qlen += xmlStrlen(data->param[i].name);
                qlen += xmlStrlen(data->param[i].val);
                qlen += 2;
            }
        }
    }

    body.len = (size_t)xmlStrlen(data->body);

    if (blen > 0 && body.len > 0) {
        xrltTransformError(
            ctx, NULL, data->inode,
            "Can't have request body and body parameters at the same time\n"
        );
        ret = FALSE;
        goto error;
    }

    if (body.len > 0) {
        body.data = (char *)data->body;
    }

    if (blen > 0 || qlen > 0) {
        if (blen > 0) {
            ebody = (char *)xrltMalloc((blen * 3) + 1);
        }

        if (qlen > 0) {
            equery = (char *)xrltMalloc((qlen * 3) + 1);
        }

        if ((blen > 0 && ebody == NULL) || (qlen > 0 && equery == NULL)) {
            xrltTransformError(ctx, NULL, data->inode,
                               "xrltIncludeAddSubrequest: Out of memory\n");
            ret = FALSE;
            goto error;
        }

        bp = ebody;
        qp = equery;

        for (i = 0; i < data->paramCount; i++) {
            if (data->param[i].test) {
                if (data->param[i].body) {
                    if (bp != ebody) { *bp++ = '&'; }

                    blen = xrltURLEncode((char *)data->param[i].name, bp);
                    bp += blen;
                    if (blen > 0) { *bp++ = '='; }

                    blen = xrltURLEncode((char *)data->param[i].val, bp);
                    bp += blen;
                } else {
                    if (qp != equery) { *qp++ = '&'; }

                    qlen = xrltURLEncode((char *)data->param[i].name, qp);
                    qp += qlen;
                    if (qlen > 0) { *qp++ = '='; }

                    qlen = xrltURLEncode((char *)data->param[i].val, qp);
                    qp += qlen;
                }
            }
        }

        if (ebody != NULL) {
            *bp = '\0';
            body.len = bp - ebody;
            body.data = ebody;
        }

        if (equery != NULL) {
            *qp = '\0';
            query.len = qp - equery;
            query.data = equery;
        }

        printf("Reqbody: %s\n", ebody);
        printf("Reqquery: %s\n", equery);
    }


    id = ++ctx->includeId;

    xrltSubrequestListPush(&ctx->sr, id, &header, &href, &query, &body);


    /*
    xrltInputSubscribe(ctx, XRLT_PROCESS_SUBREQUEST_HEADER, id,
                       xrltIncludeSubrequestHeader, data);

    xrltInputSubscribe(ctx, XRLT_PROCESS_SUBREQUEST_BODY, id,
                       xrltIncludeSubrequestBody, data);
                       */

  error:
    xrltHeaderListClear(&header);

    if (ebody != NULL) { xrltFree(ebody); }
    if (equery != NULL) { xrltFree(equery); }

    return ret;
}


xrltBool
xrltIncludeTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                     void *data)
{
    xrltCompiledIncludeData      *icomp = (xrltCompiledIncludeData *)comp;
    xmlNodePtr                    node;
    xrltNodeDataPtr               n;
    xrltIncludeTransformingData  *tdata;
    xrltCompiledIncludeParamPtr   p;
    size_t                        i;

    if (data == NULL) {
        // First call.
        NEW_CHILD(ctx, node, insert, "include");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(tdata, xrltIncludeTransformingData*,
                    sizeof(xrltIncludeTransformingData) +
                    sizeof(xrltTransformingParam) * icomp->headerCount +
                    sizeof(xrltTransformingParam) * icomp->paramCount,
                    "xrltIncludeTransform", FALSE;);

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

        tdata->node = node;
        tdata->inode = icomp->node;

        COUNTER_INCREASE(ctx, node);

        NEW_CHILD(ctx, node, node, "params");

        tdata->pnode = node;

        TRANSFORM_TO_STRING(
            ctx, node, icomp->href, icomp->nhref, icomp->xhref, &tdata->href
        );

        TRANSFORM_TO_STRING(
            ctx, node, icomp->body, icomp->nbody, icomp->xbody, &tdata->body
        );

        p = icomp->fheader;

        for (i = 0; i < tdata->headerCount; i++) {
            TRANSFORM_TO_BOOLEAN(
                ctx, node, p->test, p->ntest, p->xtest, &tdata->header[i].test
            );
            p = p->next;
        }

        p = icomp->fparam;

        for (i = 0; i < tdata->paramCount; i++) {
            TRANSFORM_TO_BOOLEAN(
                ctx, node, p->test, p->ntest, p->xtest, &tdata->param[i].test
            );
            p = p->next;
        }

        COUNTER_INCREASE(ctx, node);

        // Schedule the next call.
        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltIncludeTransform, comp, insert, tdata
        );
    } else {
        tdata = (xrltIncludeTransformingData *)data;

        switch (tdata->stage) {
            case XRLT_INCLUDE_TRANSFORM_PARAMS_BEGIN:
            case XRLT_INCLUDE_TRANSFORM_PARAMS_END:
                node = tdata->pnode;
                break;

            case XRLT_INCLUDE_TRANSFORM_RESULT_BEGIN:
            case XRLT_INCLUDE_TRANSFORM_RESULT_END:
                node = tdata->node;
                break;
        }

        ASSERT_NODE_DATA(node, n);

        if (n->count > 0) {

            COUNTER_DECREASE(ctx, node);

            if (n->count > 0) {
                SCHEDULE_CALLBACK(
                    ctx, &n->tcb, xrltIncludeTransform, comp, insert, data
                );

                return TRUE;
            }
        }

        switch (tdata->stage) {
            case XRLT_INCLUDE_TRANSFORM_PARAMS_BEGIN:
                p = icomp->fheader;

                for (i = 0; i < tdata->headerCount; i++) {
                    if (tdata->header[i].test) {
                        TRANSFORM_TO_STRING(
                            ctx, node, p->name, p->nname, p->xname,
                            &tdata->header[i].name
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, p->val, p->nval, p->xval,
                            &tdata->header[i].val
                        );
                    }
                    p = p->next;
                }

                p = icomp->fparam;

                for (i = 0; i < tdata->paramCount; i++) {
                    if (tdata->param[i].test) {
                        TRANSFORM_TO_BOOLEAN(
                            ctx, node, p->body, p->nbody, p->xbody,
                            &tdata->param[i].body
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, p->name, p->nname, p->xname,
                            &tdata->param[i].name
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, p->val, p->nval, p->xval,
                            &tdata->param[i].val
                        );
                    }
                    p = p->next;
                }

                COUNTER_INCREASE(ctx, node);

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltIncludeTransform, comp, insert, data
                );

                tdata->stage = XRLT_INCLUDE_TRANSFORM_PARAMS_END;
                break;

            case XRLT_INCLUDE_TRANSFORM_PARAMS_END:
                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltIncludeTransform, comp, insert, data
                );

                xrltIncludeAddSubrequest(ctx, tdata);

                tdata->stage = XRLT_INCLUDE_TRANSFORM_RESULT_BEGIN;
                break;

            case XRLT_INCLUDE_TRANSFORM_RESULT_BEGIN:
                break;

            case XRLT_INCLUDE_TRANSFORM_RESULT_END:
                break;
        }

    }

    return TRUE;
}
