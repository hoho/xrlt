/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

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
    xrltBool                      hasxrlt;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltCompiledIncludeParamPtr,
                sizeof(xrltCompiledIncludeParam), NULL);

    if (header) {
        if (!xrltCompileCheckSubnodes(sheet, node, XRLT_ELEMENT_NAME,
                                      XRLT_ELEMENT_VALUE, XRLT_ELEMENT_TEST,
                                      NULL, NULL, NULL, &hasxrlt))
        {
            goto error;
        }
    } else {
        if (!xrltCompileCheckSubnodes(sheet, node, XRLT_ELEMENT_NAME,
                                      XRLT_ELEMENT_VALUE, XRLT_ELEMENT_TEST,
                                      XRLT_ELEMENT_BODY, NULL, NULL, &hasxrlt))
        {
            goto error;
        }
    }

    if (!xrltCompileValue(sheet, node, NULL, NULL, XRLT_ELEMENT_NAME,
                          XRLT_ELEMENT_NAME, TRUE, &ret->name))
    {
        goto error;
    }

    if (!xrltCompileValue(sheet, node, hasxrlt ? NULL : node->children,
                          XRLT_ELEMENT_ATTR_SELECT, NULL, XRLT_ELEMENT_VALUE,
                          TRUE, &ret->val))
    {
        goto error;
    }

    if (!xrltCompileValue(sheet, node, NULL, XRLT_ELEMENT_ATTR_TEST, NULL,
                          XRLT_ELEMENT_TEST, FALSE, &ret->test))
    {
        goto error;
    }

    if (!header) {
        if (!xrltCompileValue(sheet, node, NULL, NULL, XRLT_ELEMENT_BODY,
                              XRLT_ELEMENT_BODY, TRUE, &ret->body))
        {
            goto error;
        }
    }

    if (ret->name.type == XRLT_VALUE_EMPTY &&
        ret->val.type == XRLT_VALUE_EMPTY)
    {
        xrltTransformError(NULL, sheet, node, "Element has no name and value");
        goto error;
    }

    if (ret->body.type == XRLT_VALUE_TEXT) {
        ret->body.type = XRLT_VALUE_INT;

        ret->body.intval = \
            xmlStrEqual(ret->body.textval, XRLT_VALUE_YES) ? TRUE : FALSE;

        xmlFree(ret->body.textval);
        ret->body.textval = NULL;
    }

    return ret;

  error:
    if (ret != NULL) {
        CLEAR_XRLT_VALUE(ret->test);
        CLEAR_XRLT_VALUE(ret->body);
        CLEAR_XRLT_VALUE(ret->name);
        CLEAR_XRLT_VALUE(ret->val);

        xmlFree(ret);
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
    xrltBool                      hasxrlt;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltCompiledIncludeData*,
                sizeof(xrltCompiledIncludeData), NULL);

    tmp = node->children;

    while (tmp != NULL) {
        if (tmp->ns == NULL || !xmlStrEqual(tmp->ns->href, XRLT_NS)) {
            xrltTransformError(NULL, sheet, tmp, "Unexpected element\n");
            goto error;
        }

        if (ret->href.type == XRLT_VALUE_EMPTY &&
            xmlStrEqual(tmp->name, XRLT_ELEMENT_HREF))
        {
            if (!xrltCompileValue(sheet, tmp, tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, TRUE,
                                  &ret->href))
            {
                goto error;
            }

            if (ret->href.type == XRLT_VALUE_EMPTY) {
                xrltTransformError(NULL, sheet, tmp, "Element has no value");
                goto error;
            }
        }
        else if (ret->method.type == XRLT_VALUE_EMPTY &&
                 xmlStrEqual(tmp->name, XRLT_ELEMENT_METHOD))
        {
            if (!xrltCompileValue(sheet, tmp, tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, TRUE,
                                  &ret->method))
            {
                goto error;
            }

            if (ret->method.type == XRLT_VALUE_EMPTY) {
                xrltTransformError(NULL, sheet, tmp, "Element has no value");
                goto error;
            }

            if (ret->method.type == XRLT_VALUE_TEXT) {
                ret->method.type = XRLT_VALUE_INT;

                ret->method.intval = \
                    xrltIncludeMethodFromString(ret->method.textval);

                xmlFree(ret->method.textval);
                ret->method.textval = NULL;
            }
        }
        else if (ret->type.type == XRLT_VALUE_EMPTY &&
                 xmlStrEqual(tmp->name, XRLT_ELEMENT_TYPE))
        {
            if (!xrltCompileValue(sheet, tmp, tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, TRUE,
                                  &ret->type))
            {
                goto error;
            }

            if (ret->type.type == XRLT_VALUE_EMPTY) {
                xrltTransformError(NULL, sheet, tmp, "Element has no value");
                goto error;
            }

            if (ret->type.type == XRLT_VALUE_TEXT) {
                ret->type.type = XRLT_VALUE_INT;

                ret->type.intval = \
                    xrltIncludeTypeFromString(ret->type.textval);

                xmlFree(ret->type.textval);
                ret->type.textval = NULL;
            }
        }
        else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_HEADER))
        {
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
        }
        else if (xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_PARAM))
        {
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
        }
        else if (ret->body.type == XRLT_VALUE_EMPTY &&
                 xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_BODY))
        {
            if (!xrltCompileCheckSubnodes(sheet, tmp, XRLT_ELEMENT_TEST,
                                          XRLT_ELEMENT_VALUE, NULL, NULL, NULL,
                                          NULL, &hasxrlt))
            {
                goto error;
            }

            if (!xrltCompileValue(sheet, tmp, NULL, XRLT_ELEMENT_ATTR_TEST,
                                  NULL, XRLT_ELEMENT_TEST, FALSE,
                                  &ret->bodyTest))
            {
                goto error;
            }

            if (!xrltCompileValue(sheet, tmp, hasxrlt ? NULL : tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL,
                                  XRLT_ELEMENT_VALUE, TRUE, &ret->body))
            {
                goto error;
            }

            if (ret->body.type == XRLT_VALUE_EMPTY) {
                xrltTransformError(NULL, sheet, tmp, "Element has no value");
            }
        }
        else if (ret->success.type == XRLT_VALUE_EMPTY &&
                 xmlStrEqual(tmp->name, XRLT_ELEMENT_SUCCESS))
        {
            if (!xrltCompileValue(sheet, tmp, tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, FALSE,
                                  &ret->success))
            {
                goto error;
            }

            if (ret->success.type == XRLT_VALUE_EMPTY) {
                xrltTransformError(NULL, sheet, tmp, "Element has no value");
                goto error;
            }
        }
        else if (ret->failure.type == XRLT_VALUE_EMPTY &&
                 xmlStrEqual(tmp->name, XRLT_ELEMENT_FAILURE))
        {
            if (!xrltCompileValue(sheet, tmp, tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, FALSE,
                                  &ret->failure))
            {
                goto error;
            }

            if (ret->failure.type == XRLT_VALUE_EMPTY) {
                xrltTransformError(NULL, sheet, tmp, "Element has no value");
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
        CLEAR_XRLT_VALUE(ret->href);
        CLEAR_XRLT_VALUE(ret->method);
        CLEAR_XRLT_VALUE(ret->type);
        CLEAR_XRLT_VALUE(ret->bodyTest);
        CLEAR_XRLT_VALUE(ret->body);
        CLEAR_XRLT_VALUE(ret->success);
        CLEAR_XRLT_VALUE(ret->failure);

        param = ret->fheader;
        while (param != NULL) {
            CLEAR_XRLT_VALUE(param->test);
            CLEAR_XRLT_VALUE(param->body);
            CLEAR_XRLT_VALUE(param->name);
            CLEAR_XRLT_VALUE(param->val);

            tmp = param->next;
            xmlFree(param);
            param = tmp;
        }

        param = ret->fparam;
        while (param != NULL) {
            CLEAR_XRLT_VALUE(param->test);
            CLEAR_XRLT_VALUE(param->body);
            CLEAR_XRLT_VALUE(param->name);
            CLEAR_XRLT_VALUE(param->val);

            tmp = param->next;
            xmlFree(param);
            param = tmp;
        }

        xmlFree(ret);
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
            if (tdata->header[i].cbody != NULL) {
                xmlFree(tdata->header[i].cbody);
            }

            if (tdata->header[i].name != NULL) {
                xmlFree(tdata->header[i].name);
            }

            if (tdata->header[i].val != NULL) {
                xmlFree(tdata->header[i].val);
            }
        }

        for (i = 0; i < tdata->paramCount; i++) {
            if (tdata->param[i].cbody != NULL) {
                xmlFree(tdata->param[i].cbody);
            }

            if (tdata->param[i].name != NULL) {
                xmlFree(tdata->param[i].name);
            }

            if (tdata->param[i].val != NULL) {
                xmlFree(tdata->param[i].val);
            }
        }

        xmlFree(tdata);
    }
}


static xrltBool
xrltIncludeSubrequestHeader(xrltContextPtr ctx, size_t id,
                            xrltTransformValue *val, void *data)
{
    if (val != NULL) {
        fprintf(stderr, "HEADER: %zd, %s: %s\n",
                val->status, val->name.data, val->val.data);
    }

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
                    NULL, NULL, val->val.data, val->val.len,
                    (const char *)tdata->href
                );

                if (tdata->xmlparser == NULL) {
                    xrltTransformError(ctx, NULL, tdata->srcNode,
                                       "Failed to create parser\n");
                    return FALSE;
                }

                if (val->last) {
                    if (xmlParseChunk(tdata->xmlparser,
                                      val->val.data, 0, 1) != 0)
                    {
                        XRLT_INCLUDE_PARSING_FAILED;
                    }
                }

                break;

            case XRLT_SUBREQUEST_DATA_JSON:
                tdata->jsonparser = xrltJSON2XMLInit((xmlNodePtr)tdata->doc,
                                                     FALSE);

                if (tdata->jsonparser == NULL) {
                    xrltTransformError(ctx, NULL, tdata->srcNode,
                                       "Failed to create parser\n");
                    return FALSE;
                }

                if (!xrltJSON2XMLFeed(tdata->jsonparser, val->val.data,
                                      val->val.len))
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
                                               val->val.data, val->val.len,
                                               val->last))
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_TEXT:
                tmp = xmlNewTextLen((const xmlChar *)val->val.data,
                                    val->val.len);

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
                                  val->val.data, val->val.len, val->last) != 0)
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_JSON:
                if (!xrltJSON2XMLFeed(tdata->jsonparser, val->val.data,
                                      val->val.len))
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_QUERYSTRING:
                if (!xrltQueryStringParserFeed(tdata->qsparser,
                                               val->val.data, val->val.len,
                                               val->last))
                {
                    XRLT_INCLUDE_PARSING_FAILED;
                }

                break;

            case XRLT_SUBREQUEST_DATA_TEXT:
                tmp = xmlNewTextLen((const xmlChar *)val->val.data,
                                    val->val.len);

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
    xrltBool         cookie;
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
            cookie = data->header[i].cookie;
            // Use query and body variables as name and value to reduce
            // declared variables count.
            xrltStringSet(&query, (char *)data->header[i].name);
            xrltStringSet(&body, (char *)data->header[i].val);

            if (!xrltHeaderListPush(&header, cookie, &query, &body)) {
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
            if (data->param[i].body ||
                (data->param[i].cbody != NULL &&
                 xmlStrEqual(data->param[i].cbody, XRLT_VALUE_YES)))
            {
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
            ebody = (char *)xmlMalloc((blen * 3) + 1);
        }

        if (qlen > 0) {
            equery = (char *)xmlMalloc((qlen * 3) + 1);
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

    if (ebody != NULL) { xmlFree(ebody); }
    if (equery != NULL) { xmlFree(equery); }

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
    xrltValue                    *r;

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

        NEW_CHILD(ctx, node, node, "p");

        tdata->pnode = node;

        TRANSFORM_TO_STRING(ctx, node, &icomp->href, &tdata->href);

        if (icomp->method.type == XRLT_VALUE_INT) {
            tdata->method = (xrltHTTPMethod)icomp->method.intval;
        } else if (icomp->method.type != XRLT_VALUE_EMPTY) {
            TRANSFORM_TO_STRING(ctx, node, &icomp->method, &tdata->cmethod);
        }

        if (icomp->type.type == XRLT_VALUE_INT) {
            tdata->type = (xrltSubrequestDataType)icomp->type.intval;
        } else if (icomp->type.type != XRLT_VALUE_EMPTY) {
            TRANSFORM_TO_STRING(ctx, node, &icomp->type, &tdata->ctype);
        }

        if (icomp->bodyTest.type != XRLT_VALUE_EMPTY) {
            TRANSFORM_TO_BOOLEAN(ctx, node, &icomp->bodyTest, &tdata->bodyTest);
        } else {
            tdata->bodyTest = TRUE;
        }

        p = icomp->fheader;

        for (i = 0; i < tdata->headerCount; i++) {
            if (p->test.type != XRLT_VALUE_EMPTY)  {
                TRANSFORM_TO_BOOLEAN(
                    ctx, node, &p->test, &tdata->header[i].test
                );
            } else {
                tdata->header[i].test = TRUE;
            }
            p = p->next;
        }

        p = icomp->fparam;

        for (i = 0; i < tdata->paramCount; i++) {
            if (p->test.type != XRLT_VALUE_EMPTY)  {
                TRANSFORM_TO_BOOLEAN(
                    ctx, node, &p->test, &tdata->param[i].test
                );
            } else {
                tdata->param[i].test = TRUE;
            }
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
                            ctx, node, &p->name, &tdata->header[i].name
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, &p->val, &tdata->header[i].val
                        );
                    }
                    p = p->next;
                }

                p = icomp->fparam;

                for (i = 0; i < tdata->paramCount; i++) {
                    if (tdata->param[i].test) {
                        if (p->body.type == XRLT_VALUE_INT) {
                            tdata->param[i].body = p->body.intval;
                        } else {
                            TRANSFORM_TO_STRING(
                                ctx, node, &p->body, &tdata->param[i].cbody
                            );
                        }
                        TRANSFORM_TO_STRING(
                            ctx, node, &p->name, &tdata->param[i].name
                        );
                        TRANSFORM_TO_STRING(
                            ctx, node, &p->val, &tdata->param[i].val
                        );
                    }
                    p = p->next;
                }

                if (tdata->bodyTest) {
                    TRANSFORM_TO_STRING(ctx, node, &icomp->body, &tdata->body);
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
                        r = &icomp->success;
                    } else {
                        r = &icomp->failure;
                    }

                    NEW_CHILD(ctx, tdata->rnode, tdata->node, "tmp");

                    ASSERT_NODE_DATA(tdata->rnode, n);

                    n->root = tdata->doc;

                    switch (r->type) {
                        case XRLT_VALUE_TEXT:
                            node = xmlNewText(r->textval);

                            xmlAddChild(tdata->rnode, node);

                            break;

                        case XRLT_VALUE_NODELIST:
                            COUNTER_INCREASE(ctx, tdata->rnode);

                            TRANSFORM_SUBTREE(ctx, r->nodeval, tdata->rnode);

                            SCHEDULE_CALLBACK(ctx, &ctx->tcb,
                                              xrltIncludeTransform, comp,
                                              insert, data);
                            break;

                        case XRLT_VALUE_XPATH:
                            COUNTER_INCREASE(ctx, tdata->rnode);

                            TRANSFORM_XPATH_TO_NODE(
                                ctx, &r->xpathval, tdata->rnode
                            );

                            SCHEDULE_CALLBACK(ctx, &ctx->tcb,
                                              xrltIncludeTransform, comp,
                                              insert, data);
                            break;

                        case XRLT_VALUE_EMPTY:
                            if (tdata->stage == XRLT_INCLUDE_TRANSFORM_SUCCESS)
                            {
                                node = xmlDocCopyNodeList(tdata->rnode->doc,
                                                          tdata->doc->children);
                                if (node == NULL) {
                                    xrltTransformError(
                                        ctx, NULL, tdata->srcNode,
                                        "Failed to copy response node\n"
                                    );

                                    return FALSE;
                                }

                                if (xmlAddChildList(tdata->rnode, node) == NULL)
                                {
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

                        case XRLT_VALUE_INT:
                            break;
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
