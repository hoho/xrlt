#include "transform.h"
#include "include.h"


static inline xrltHTTPMethod
xrltIncludeMethodFromString(xmlChar *method)
{
    if (method != NULL) {
        if (xmlStrcasecmp(method, (const xmlChar *)"GET") == 0) {
            return XRLT_METHOD_GET;
        } else if (xmlStrcasecmp(method, (const xmlChar *)"HEAD") == 0) {
            return XRLT_METHOD_HEAD;
        } else if (xmlStrcasecmp(method, (const xmlChar *)"POST") == 0) {
            return XRLT_METHOD_POST;
        } else if (xmlStrcasecmp(method, (const xmlChar *)"PUT") == 0) {
            return XRLT_METHOD_PUT;
        } else if (xmlStrcasecmp(method, (const xmlChar *)"DELETE") == 0) {
            return XRLT_METHOD_DELETE;
        } else if (xmlStrcasecmp(method, (const xmlChar *)"TRACE") == 0) {
            return XRLT_METHOD_TRACE;
        } else if (xmlStrcasecmp(method, (const xmlChar *)"OPTIONS") == 0) {
            return XRLT_METHOD_OPTIONS;
        } else {
            return XRLT_METHOD_GET;
        }
    } else {
        return XRLT_METHOD_GET;
    }
}


static inline xrltSubrequestDataType
xrltIncludeTypeFromString(xmlChar *type)
{
    if (type != NULL) {
        if (xmlStrcasecmp(type, (const xmlChar *)"JSON") == 0) {
            return XRLT_SUBREQUEST_DATA_JSON;
        } else if (xmlStrcasecmp(type, (const xmlChar *)"TEXT") == 0) {
            return XRLT_SUBREQUEST_DATA_TEXT;
        } else if (xmlStrcasecmp(type, (const xmlChar *)"QUERYSTRING") == 0) {
            return XRLT_SUBREQUEST_DATA_QUERYSTRING;
        } else {
            return XRLT_SUBREQUEST_DATA_XML;
        }
    } else {
        return XRLT_SUBREQUEST_DATA_XML;
    }
}


static inline xrltCompiledIncludeParamPtr
xrltIncludeParamCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                        xrltBool header)
{
    xrltCompiledIncludeParamPtr   ret = NULL;
    int                           conf;
    int                           _test;
    xmlChar                      *_type = NULL;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltCompiledIncludeParamPtr,
                sizeof(xrltCompiledIncludeParam), NULL);

    if (header) {
        conf = XRLT_TESTNAMEVALUE_TEST_ATTR | XRLT_TESTNAMEVALUE_TEST_NODE |
               XRLT_TESTNAMEVALUE_NAME_ATTR | XRLT_TESTNAMEVALUE_NAME_NODE |
               XRLT_TESTNAMEVALUE_VALUE_ATTR | XRLT_TESTNAMEVALUE_VALUE_NODE |
               XRLT_TESTNAMEVALUE_NAME_REQUIRED;
    } else {
        conf = XRLT_TESTNAMEVALUE_TEST_ATTR | XRLT_TESTNAMEVALUE_TEST_NODE |
               XRLT_TESTNAMEVALUE_TYPE_ATTR | XRLT_TESTNAMEVALUE_TYPE_NODE |
               XRLT_TESTNAMEVALUE_NAME_ATTR | XRLT_TESTNAMEVALUE_NAME_NODE |
               XRLT_TESTNAMEVALUE_VALUE_ATTR | XRLT_TESTNAMEVALUE_VALUE_NODE |
               XRLT_TESTNAMEVALUE_NAME_OR_VALUE_REQUIRED;
    }

    if (!xrltCompileTestNameValueNode(sheet, node, conf, &_test, &ret->ntest,
                                      &ret->xtest, &_type, &ret->nbody,
                                      &ret->xbody, &ret->name, &ret->nname,
                                      &ret->xname, &ret->val, &ret->nval,
                                      &ret->xval))
    {
        goto error;
    }

    if (_test > 0) {
        ret->test = _test == 1 ? TRUE : FALSE;
    } else if (ret->ntest == NULL && ret->xtest.expr == NULL) {
        ret->test = TRUE;
    }

    if (_type != NULL) {
        ret->body = xmlStrEqual(_type, (const xmlChar *)"body") ? TRUE : FALSE;
        xmlFree(_type);
    }

    return ret;

  error:
    if (_type != NULL) { xmlFree(_type); }
    if (ret != NULL) { xrltFree(ret); }

    return NULL;
}


void *
xrltIncludeCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltCompiledIncludeData      *ret = NULL;
    xmlNodePtr                    tmp;
    xrltNodeDataPtr               n;
    xrltCompiledIncludeParamPtr   param;
    int                           conf;
    xmlChar                      *tmp2;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltCompiledIncludeData*,
                sizeof(xrltCompiledIncludeData), NULL);

    tmp = node->children;

    while (tmp != NULL) {
        if (tmp->ns == NULL || !xmlStrEqual(tmp->ns->href, XRLT_NS)) {
            xrltTransformError(NULL, sheet, tmp, "Unexpected element\n");
            goto error;
        }

        if (xmlStrEqual(tmp->name, XRLT_ELEMENT_HREF)) {
            conf = XRLT_TESTNAMEVALUE_VALUE_ATTR |
                   XRLT_TESTNAMEVALUE_VALUE_NODE |
                   XRLT_TESTNAMEVALUE_VALUE_REQUIRED |
                   XRLT_TESTNAMEVALUE_TO_STRING;

            if (!xrltCompileTestNameValueNode(sheet, tmp, conf, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, &ret->href,
                                              &ret->nhref, &ret->xhref))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_METHOD)) {
            conf = XRLT_TESTNAMEVALUE_VALUE_ATTR |
                   XRLT_TESTNAMEVALUE_VALUE_NODE |
                   XRLT_TESTNAMEVALUE_VALUE_REQUIRED |
                   XRLT_TESTNAMEVALUE_TO_STRING;

            if (!xrltCompileTestNameValueNode(sheet, tmp, conf, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, &tmp2,
                                              &ret->nmethod, &ret->xmethod))
            {
                goto error;
            }

            ret->method = xrltIncludeMethodFromString(tmp2);
            if (tmp2 != NULL) { xmlFree(tmp2); }
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_TYPE)) {
            conf = XRLT_TESTNAMEVALUE_VALUE_ATTR |
                   XRLT_TESTNAMEVALUE_VALUE_NODE |
                   XRLT_TESTNAMEVALUE_VALUE_REQUIRED |
                   XRLT_TESTNAMEVALUE_TO_STRING;

            if (!xrltCompileTestNameValueNode(sheet, tmp, conf, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, &tmp2,
                                              &ret->ntype, &ret->xtype))
            {
                goto error;
            }

            ret->type = xrltIncludeTypeFromString(tmp2);
            if (tmp2 != NULL) { xmlFree(tmp2); }
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
            conf = XRLT_TESTNAMEVALUE_TEST_ATTR |
                   XRLT_TESTNAMEVALUE_TEST_NODE |
                   XRLT_TESTNAMEVALUE_VALUE_ATTR |
                   XRLT_TESTNAMEVALUE_VALUE_NODE |
                   XRLT_TESTNAMEVALUE_VALUE_REQUIRED |
                   XRLT_TESTNAMEVALUE_TO_STRING;

            if (!xrltCompileTestNameValueNode(sheet, tmp, conf, &conf,
                                              &ret->tnbody, &ret->txbody, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              &ret->body, &ret->nbody,
                                              &ret->xbody))
            {
                goto error;
            }

            if (conf > 0) {
                ret->tbody = conf == 1 ? TRUE : FALSE;
            } else if (ret->nbody == NULL && ret->xbody.expr == NULL) {
                ret->tbody = TRUE;
            }
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_SUCCESS)) {
            conf = XRLT_TESTNAMEVALUE_VALUE_ATTR |
                   XRLT_TESTNAMEVALUE_VALUE_NODE |
                   XRLT_TESTNAMEVALUE_VALUE_REQUIRED;

            if (!xrltCompileTestNameValueNode(sheet, tmp, conf, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, &tmp2,
                                              &ret->nsuccess, &ret->xsuccess))
            {
                goto error;
            }
        } else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_FAILURE)) {
            xrltXPathExpr tmpexpr;

            conf = XRLT_TESTNAMEVALUE_VALUE_NODE |
                   XRLT_TESTNAMEVALUE_VALUE_REQUIRED;

            if (!xrltCompileTestNameValueNode(sheet, tmp, conf, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, &tmp2,
                                              &ret->nfailure, &tmpexpr))
            {
                goto error;
            }
        } else {
            xrltTransformError(NULL, sheet, tmp, "Unexpected element\n");
            goto error;
        }

        ASSERT_NODE_DATA_GOTO(tmp, n);

        n->xrlt = TRUE;

        tmp = tmp->next;
    }

    if (ret->href == NULL && ret->nhref == NULL && ret->xhref.expr == NULL) {
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
        if (ret->xhref.expr != NULL) {
            xmlXPathFreeCompExpr(ret->xhref.expr);
        }

        if (ret->xmethod.expr != NULL) {
            xmlXPathFreeCompExpr(ret->xmethod.expr);
        }

        if (ret->xtype.expr != NULL) {
            xmlXPathFreeCompExpr(ret->xtype.expr);
        }

        if (ret->txbody.expr != NULL) {
            xmlXPathFreeCompExpr(ret->txbody.expr);
        }

        if (ret->body != NULL) { xmlFree(ret->body); }
        if (ret->xbody.expr != NULL) {
            xmlXPathFreeCompExpr(ret->xbody.expr);
        }

        if (ret->xsuccess.expr != NULL) {
            xmlXPathFreeCompExpr(ret->xsuccess.expr);
        }

        param = ret->fheader;
        while (param != NULL) {
            if (param->xtest.expr != NULL) {
                xmlXPathFreeCompExpr(param->xtest.expr);
            }

            if (param->name != NULL) { xmlFree(param->name); }
            if (param->xname.expr != NULL) {
                xmlXPathFreeCompExpr(param->xname.expr);
            }

            if (param->val != NULL) { xmlFree(param->val); }
            if (param->xval.expr != NULL) {
                xmlXPathFreeCompExpr(param->xval.expr);
            }

            tmp = param->next;
            xrltFree(param);
            param = tmp;
        }

        param = ret->fparam;
        while (param != NULL) {
            if (param->xtest.expr != NULL) {
                xmlXPathFreeCompExpr(param->xtest.expr);
            }

            if (param->xbody.expr != NULL) {
                xmlXPathFreeCompExpr(param->xbody.expr);
            }

            if (param->name != NULL) { xmlFree(param->name); }
            if (param->xname.expr != NULL) {
                xmlXPathFreeCompExpr(param->xname.expr);
            }

            if (param->val != NULL) { xmlFree(param->val); }
            if (param->xval.expr != NULL) {
                xmlXPathFreeCompExpr(param->xval.expr);
            }

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

        if (tdata->href != NULL) { xmlFree(tdata->href); }

        if (tdata->cmethod != NULL) { xmlFree(tdata->cmethod); }

        if (tdata->ctype != NULL) { xmlFree(tdata->ctype); }

        if (tdata->body != NULL) { xmlFree(tdata->body); }

        if (tdata->xmlparser != NULL) {
            if (tdata->xmlparser->myDoc != NULL) {
                xmlFreeDoc(tdata->xmlparser->myDoc);
            }
            xmlFreeParserCtxt(tdata->xmlparser);
        }

        if (tdata->jsonparser != NULL) {
            xrltJSON2XMLFree(tdata->jsonparser);
        }

        if (tdata->qsparser != NULL) {
            xrltQueryStringParserFree(tdata->qsparser);
        }

        if (tdata->doc != NULL) { xmlFreeDoc(tdata->doc); }

        for (i = 0; i < tdata->headerCount; i++) {
            if (tdata->header[i].name != NULL) {
                xmlFree(tdata->header[i].name);
            }

            if (tdata->header[i].val != NULL) {
                xmlFree(tdata->header[i].val);
            }
        }

        for (i = 0; i < tdata->paramCount; i++) {
            if (tdata->param[i].name != NULL) {
                xmlFree(tdata->param[i].name);
            }

            if (tdata->param[i].val != NULL) {
                xmlFree(tdata->param[i].val);
            }
        }

        xrltFree(tdata);
    }
}


static xrltBool
xrltIncludeSubrequestHeader(xrltContextPtr ctx, size_t id,
                            xrltTransformValue *val, void *data)
{
    return TRUE;
}


#define XRLT_INCLUDE_PARSING_FAILED                                           \
    tdata->stage = XRLT_INCLUDE_TRANSFORM_FAILURE;                            \
    SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform,                   \
                      tdata->comp, tdata->insert, tdata);                     \
    ctx->cur |= XRLT_STATUS_REFUSE_SUBREQUEST;                                \
    return TRUE;


static xrltBool
xrltIncludeSubrequestBody(xrltContextPtr ctx, size_t id,
                          xrltTransformValue *val, void *data)
{
    xrltIncludeTransformingData  *tdata = (xrltIncludeTransformingData *)data;
    xmlNodePtr                    tmp;



    if (tdata == NULL) { return FALSE; }

    if (tdata->stage != XRLT_INCLUDE_TRANSFORM_READ_RESPONSE) {
        xrltTransformError(ctx, NULL, tdata->srcNode,
                           "Wrong transformation stage\n");
        return FALSE;
    }

    if (val->error) {
        tdata->stage = XRLT_INCLUDE_TRANSFORM_FAILURE;

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform,
                          tdata->comp, tdata->insert, tdata);

        ctx->cur |= XRLT_STATUS_REFUSE_SUBREQUEST;

        return TRUE;
    }

    if (tdata->doc == NULL && tdata->xmlparser == NULL) {
        // On the first call create a doc for the result to and a parser.

        if (tdata->type != XRLT_SUBREQUEST_DATA_XML) {
            // XRLT_SUBREQUEST_DATA_XML type will use the doc from xmlparser.
            tdata->doc = xmlNewDoc(NULL);

            if (tdata->doc == NULL) {
                xrltTransformError(ctx, NULL, tdata->srcNode,
                                   "Failed to create include doc\n");
                return FALSE;
            }
        }

        switch (tdata->type) {
            case XRLT_SUBREQUEST_DATA_XML:
                tdata->xmlparser = xmlCreatePushParserCtxt(
                    NULL, NULL, val->data.data, val->data.len,
                    (const char *)tdata->href
                );

                if (tdata->xmlparser == NULL) {
                    xrltTransformError(ctx, NULL, tdata->srcNode,
                                       "Failed to create parser\n");
                    return FALSE;
                }

                if (val->last) {
                    if (xmlParseChunk(tdata->xmlparser,
                                      val->data.data, 0, 1) != 0)
                    {
                        XRLT_INCLUDE_PARSING_FAILED;
                    }
                }

                break;

            case XRLT_SUBREQUEST_DATA_JSON:
                tdata->jsonparser = xrltJSON2XMLInit((xmlNodePtr)tdata->doc);

                if (tdata->jsonparser == NULL) {
                    xrltTransformError(ctx, NULL, tdata->srcNode,
                                       "Failed to create parser\n");
                    return FALSE;
                }

                if (!xrltJSON2XMLFeed(tdata->jsonparser, val->data.data,
                                      val->data.len))
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_QUERYSTRING:
                tdata->qsparser = \
                    xrltQueryStringParserInit((xmlNodePtr)tdata->doc);

                if (tdata->qsparser == NULL) {
                    xrltTransformError(ctx, NULL, tdata->srcNode,
                                       "Failed to create parser\n");
                    return FALSE;
                }

                if (!xrltQueryStringParserFeed(tdata->qsparser,
                                               val->data.data, val->data.len,
                                               val->last))
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_TEXT:
                tmp = xmlNewTextLen((const xmlChar *)val->data.data,
                                    val->data.len);

                if (tmp == NULL ||
                    xmlAddChild((xmlNodePtr)tdata->doc, tmp) == NULL)
                {
                    if (tmp != NULL) { xmlFreeNode(tmp); }
                    RAISE_OUT_OF_MEMORY(ctx, NULL, tdata->srcNode);
                    return FALSE;
                }

                break;
        }
    } else {
        switch (tdata->type) {
            case XRLT_SUBREQUEST_DATA_XML:
                if (xmlParseChunk(tdata->xmlparser,
                                  val->data.data, val->data.len,
                                  val->last) != 0)
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_JSON:
                if (!xrltJSON2XMLFeed(tdata->jsonparser, val->data.data,
                                      val->data.len))
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_QUERYSTRING:
                if (!xrltQueryStringParserFeed(tdata->qsparser,
                                               val->data.data, val->data.len,
                                               val->last))
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_TEXT:
                tmp = xmlNewTextLen((const xmlChar *)val->data.data,
                                    val->data.len);

                if (tmp == NULL ||
                    xmlAddChild((xmlNodePtr)tdata->doc, tmp) == NULL)
                {
                    if (tmp != NULL) { xmlFreeNode(tmp); }
                    RAISE_OUT_OF_MEMORY(ctx, NULL, tdata->srcNode);
                    return FALSE;
                }

                break;
        }
    }

    if (val->last) {
        if (tdata->type == XRLT_SUBREQUEST_DATA_XML) {
            if (tdata->xmlparser->wellFormed == 0) {
                tdata->stage = XRLT_INCLUDE_TRANSFORM_FAILURE;
            } else {
                tdata->stage = XRLT_INCLUDE_TRANSFORM_SUCCESS;

                tdata->doc = tdata->xmlparser->myDoc;
                tdata->xmlparser->myDoc = NULL;

            }
        } else {
            tdata->stage = XRLT_INCLUDE_TRANSFORM_SUCCESS;
        }

        //xmlDocFormatDump(stdout, tdata->doc, 1);

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform, tdata->comp,
                          tdata->insert, tdata);
    }

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
                RAISE_OUT_OF_MEMORY(ctx, NULL, data->srcNode);
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
            ctx, NULL, data->srcNode,
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
            RAISE_OUT_OF_MEMORY(ctx, NULL, data->srcNode);
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
    }

    id = ++ctx->includeId;

    if (!xrltSubrequestListPush(&ctx->sr, id, data->method, data->type,
                                &header, &href, &query, &body))
    {
        RAISE_OUT_OF_MEMORY(ctx, NULL, data->srcNode);
        ret = FALSE;
        goto error;
    }

    ctx->cur |= XRLT_STATUS_SUBREQUEST;

    if (!xrltInputSubscribe(ctx, XRLT_PROCESS_HEADER, id,
                            xrltIncludeSubrequestHeader, data)
        ||

        !xrltInputSubscribe(ctx, XRLT_PROCESS_BODY, id,
                            xrltIncludeSubrequestBody, data))
    {
        ret = FALSE;
        goto error;
    }

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
    xmlNodePtr                    node = NULL;
    xrltNodeDataPtr               n;
    xrltIncludeTransformingData  *tdata;
    xrltCompiledIncludeParamPtr   p;
    size_t                        i;
    xrltXPathExpr                *expr;

    if (data == NULL) {
        // First call.
        NEW_CHILD(ctx, node, insert, "include");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, icomp->node, tdata, xrltIncludeTransformingData*,
                    sizeof(xrltIncludeTransformingData) +
                    sizeof(xrltTransformingParam) * icomp->headerCount +
                    sizeof(xrltTransformingParam) * icomp->paramCount, FALSE);

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
        tdata->srcNode = icomp->node;

        COUNTER_INCREASE(ctx, node);

        NEW_CHILD(ctx, node, node, "params");

        tdata->pnode = node;

        TRANSFORM_TO_STRING(
            ctx, node, icomp->href, icomp->nhref, &icomp->xhref, &tdata->href
        );

        tdata->method = icomp->method;
        TRANSFORM_TO_STRING(
            ctx, node, NULL, icomp->nmethod, &icomp->xmethod, &tdata->cmethod
        );

        tdata->type = icomp->type;
        TRANSFORM_TO_STRING(
            ctx, node, NULL, icomp->ntype, &icomp->xtype, &tdata->ctype
        );

        TRANSFORM_TO_BOOLEAN(ctx, node, icomp->tbody, icomp->tnbody,
                             &icomp->txbody, &tdata->tbody);

        p = icomp->fheader;

        for (i = 0; i < tdata->headerCount; i++) {
            TRANSFORM_TO_BOOLEAN(
                ctx, node, p->test, p->ntest, &p->xtest, &tdata->header[i].test
            );
            p = p->next;
        }

        p = icomp->fparam;

        for (i = 0; i < tdata->paramCount; i++) {
            TRANSFORM_TO_BOOLEAN(
                ctx, node, p->test, p->ntest, &p->xtest, &tdata->param[i].test
            );
            p = p->next;
        }

        COUNTER_INCREASE(ctx, node);

        tdata->insert = insert;
        tdata->comp = comp;

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

            case XRLT_INCLUDE_TRANSFORM_READ_RESPONSE:
                xrltTransformError(ctx, NULL, tdata->srcNode,
                                   "Wrong transformation stage\n");
                return FALSE;

            case XRLT_INCLUDE_TRANSFORM_SUCCESS:
            case XRLT_INCLUDE_TRANSFORM_FAILURE:
            case XRLT_INCLUDE_TRANSFORM_END:
                node = tdata->node;
                break;
        }

        ASSERT_NODE_DATA(node, n);

        switch (tdata->stage) {
            case XRLT_INCLUDE_TRANSFORM_PARAMS_BEGIN:
                if (n->count > 0) {
                    COUNTER_DECREASE(ctx, node);
                }

                if (n->count > 0) {
                    SCHEDULE_CALLBACK(
                        ctx, &n->tcb, xrltIncludeTransform, comp, insert, data
                    );

                    return TRUE;
                }

                if (tdata->cmethod != NULL) {
                    tdata->method = xrltIncludeMethodFromString(tdata->cmethod);
                }

                if (tdata->ctype != NULL) {
                    tdata->type = xrltIncludeTypeFromString(tdata->ctype);
                }

                p = icomp->fheader;

                for (i = 0; i < tdata->headerCount; i++) {
                    if (tdata->header[i].test) {
                        TRANSFORM_TO_STRING(
                            ctx, node, p->name, p->nname, &p->xname,
                            &tdata->header[i].name
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, p->val, p->nval, &p->xval,
                            &tdata->header[i].val
                        );
                    }
                    p = p->next;
                }

                p = icomp->fparam;

                for (i = 0; i < tdata->paramCount; i++) {
                    if (tdata->param[i].test) {
                        TRANSFORM_TO_BOOLEAN(
                            ctx, node, p->body, p->nbody, &p->xbody,
                            &tdata->param[i].body
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, p->name, p->nname, &p->xname,
                            &tdata->param[i].name
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, p->val, p->nval, &p->xval,
                            &tdata->param[i].val
                        );
                    }
                    p = p->next;
                }

                if (tdata->tbody) {
                    TRANSFORM_TO_STRING(
                        ctx, node, icomp->body, icomp->nbody, &icomp->xbody,
                        &tdata->body
                    );
                }

                tdata->stage = XRLT_INCLUDE_TRANSFORM_PARAMS_END;

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltIncludeTransform, comp, insert, data
                );

                break;

            case XRLT_INCLUDE_TRANSFORM_PARAMS_END:
                if (n->count > 0) {
                    SCHEDULE_CALLBACK(
                        ctx, &n->tcb, xrltIncludeTransform, comp, insert, data
                    );
                } else {
                    xrltIncludeAddSubrequest(ctx, tdata);

                    tdata->stage = XRLT_INCLUDE_TRANSFORM_READ_RESPONSE;
                }

                break;

            case XRLT_INCLUDE_TRANSFORM_SUCCESS:
            case XRLT_INCLUDE_TRANSFORM_FAILURE:
                if (tdata->rnode == NULL) {
                    if (tdata->stage == XRLT_INCLUDE_TRANSFORM_SUCCESS) {
                        node = icomp->nsuccess;
                        expr = &icomp->xsuccess;
                    } else {
                        node = icomp->nfailure;
                        expr = NULL;
                    }

                    NEW_CHILD(ctx, tdata->rnode, tdata->node, "tmp");

                    ASSERT_NODE_DATA(tdata->rnode, n);
                    n->root = tdata->doc;

                    if (node != NULL) {
                        COUNTER_INCREASE(ctx, tdata->rnode);

                        TRANSFORM_SUBTREE(ctx, node, tdata->rnode);

                        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform,
                                          comp, insert, data);
                    } else if (expr != NULL && expr->expr != NULL) {
                        COUNTER_INCREASE(ctx, tdata->rnode);

                        TRANSFORM_XPATH_TO_NODE(ctx, expr, tdata->rnode);

                        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform,
                                          comp, insert, data);
                    } else {
                        if (tdata->stage == XRLT_INCLUDE_TRANSFORM_SUCCESS) {
                            node = xmlDocCopyNodeList(tdata->rnode->doc,
                                                      tdata->doc->children);
                            if (node == NULL) {
                                xrltTransformError(
                                    ctx, NULL, tdata->srcNode,
                                    "Failed to copy response node\n"
                                );
                                return FALSE;
                            }

                            if (xmlAddChildList(tdata->rnode, node) == NULL) {
                                xrltTransformError(
                                    ctx, NULL, tdata->srcNode,
                                    "Failed to add response node\n"
                                );
                                xmlFreeNodeList(node);
                                return FALSE;
                            }
                        }

                        tdata->stage = XRLT_INCLUDE_TRANSFORM_END;

                        return xrltIncludeTransform(ctx, comp, insert, data);
                    }
                } else {
                    COUNTER_DECREASE(ctx, tdata->rnode);

                    ASSERT_NODE_DATA(tdata->rnode, n);

                    tdata->stage = XRLT_INCLUDE_TRANSFORM_END;

                    if (n->count > 0) {
                        SCHEDULE_CALLBACK(ctx, &n->tcb, xrltIncludeTransform,
                                          comp, insert, data);
                    } else {
                        return xrltIncludeTransform(ctx, comp, insert, data);
                    }
                }

                break;

            case XRLT_INCLUDE_TRANSFORM_END:
                COUNTER_DECREASE(ctx, tdata->node);

                REPLACE_RESPONSE_NODE(
                    ctx, tdata->node, tdata->rnode->children, tdata->srcNode
                );

                //xmlDocFormatDump(stdout, ctx->responseDoc, 1);

                break;

            case XRLT_INCLUDE_TRANSFORM_READ_RESPONSE:
                xrltTransformError(ctx, NULL, tdata->srcNode,
                                   "Wrong transformation stage\n");
                return FALSE;
        }

    }

    return TRUE;
}
