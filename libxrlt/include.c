/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "include.h"
#include "variable.h"


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
                                      NULL, NULL, NULL, NULL, NULL, &hasxrlt))
        {
            goto error;
        }
    } else {
        if (!xrltCompileCheckSubnodes(sheet, node, XRLT_ELEMENT_NAME,
                                      XRLT_ELEMENT_VALUE, XRLT_ELEMENT_TEST,
                                      XRLT_ELEMENT_BODY, NULL, NULL, NULL,
                                      NULL, &hasxrlt))
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
        xrltTransformError(NULL, sheet, node,
                           "Element has no name and value\n");
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


#define XRLT_INCLUDE_NO_VALUE                                                 \
    xrltTransformError(NULL, sheet, tmp, "Element has no value\n");           \
    goto error;


void *
xrltIncludeCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltCompiledIncludeData      *ret = NULL;
    xmlNodePtr                    tmp;
    xrltNodeDataPtr               n;
    xrltCompiledIncludeParamPtr   param;
    xrltBool                      hasxrlt;
    xrltBool                      ok;
    xmlChar                      *type;
    xrltBool                      toplevel;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltCompiledIncludeData*,
                sizeof(xrltCompiledIncludeData), NULL);

    toplevel = node->parent == NULL || node->parent->parent == NULL ||
               node->parent->parent->parent == NULL;

    ret->name = xmlGetProp(node, XRLT_ELEMENT_ATTR_NAME);

    if (ret->name == NULL && toplevel) {
        // Top-level includes have to have variable name.
        xrltTransformError(NULL, sheet, node, "Name is required\n");
        goto error;
    }

    if (ret->name != NULL) {
        ret->declScope = node->parent;
    }

    if (xmlStrEqual(node->name, (const xmlChar *)"querystring")) {
        if (sheet->querystringNode != NULL) {
            xrltTransformError(NULL, sheet, node,
                              "Duplicate 'querystring' element\n");
            goto error;
        }

        if (!toplevel) {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, node);
            goto error;
        }

        sheet->querystringNode = node;
        sheet->querystringComp = ret;
        ret->includeType = XRLT_INCLUDE_TYPE_QUERYSTRING;
    } else if (xmlStrEqual(node->name, (const xmlChar *)"body")) {
        if (sheet->bodyNode != NULL) {
            xrltTransformError(NULL, sheet, node,
                               "Duplicate 'body' element\n");
            goto error;
        }

        if (!toplevel) {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, node);
            goto error;
        }

        sheet->bodyNode = node;
        sheet->bodyComp = ret;
        ret->includeType = XRLT_INCLUDE_TYPE_BODY;
    } else {
        ret->includeType = XRLT_INCLUDE_TYPE_INCLUDE;
    }

    type = xmlGetProp(node, XRLT_ELEMENT_ATTR_TYPE);

    if (type != NULL) {
        ret->type.type = XRLT_VALUE_INT;

        ret->type.intval = \
            xrltIncludeTypeFromString(type);

        xmlFree(type);
    }

    tmp = node->children;

    while (tmp != NULL) {
        if (tmp->ns == NULL || !xmlStrEqual(tmp->ns->href, XRLT_NS)) {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
            goto error;
        }

        ok = FALSE;

        if (ret->type.type == XRLT_VALUE_EMPTY &&
            xmlStrEqual(tmp->name, XRLT_ELEMENT_TYPE))
        {
            if (!xrltCompileValue(sheet, tmp, tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, TRUE,
                                  &ret->type))
            {
                goto error;
            }

            if (ret->type.type == XRLT_VALUE_EMPTY) {
                XRLT_INCLUDE_NO_VALUE;
            }

            if (ret->type.type == XRLT_VALUE_TEXT) {
                ret->type.type = XRLT_VALUE_INT;

                ret->type.intval = \
                    xrltIncludeTypeFromString(ret->type.textval);

                xmlFree(ret->type.textval);
                ret->type.textval = NULL;
            }

            ok = TRUE;
        }

        if (ret->includeType == XRLT_INCLUDE_TYPE_INCLUDE) {
            // <xrl:querystring> and <xrl:body> don't have these.
            if (ret->href.type == XRLT_VALUE_EMPTY &&
                xmlStrEqual(tmp->name, XRLT_ELEMENT_HREF))
            {
                if (!xrltCompileValue(sheet, tmp, tmp->children,
                                      XRLT_ELEMENT_ATTR_SELECT, NULL, NULL,
                                      TRUE, &ret->href))
                {
                    goto error;
                }

                if (ret->href.type == XRLT_VALUE_EMPTY) {
                    XRLT_INCLUDE_NO_VALUE;
                }

                ok = TRUE;
            }
            else if (ret->method.type == XRLT_VALUE_EMPTY &&
                     xmlStrEqual(tmp->name, XRLT_ELEMENT_METHOD))
            {
                if (!xrltCompileValue(sheet, tmp, tmp->children,
                                      XRLT_ELEMENT_ATTR_SELECT, NULL, NULL,
                                      TRUE, &ret->method))
                {
                    goto error;
                }

                if (ret->method.type == XRLT_VALUE_EMPTY) {
                    XRLT_INCLUDE_NO_VALUE;
                }

                if (ret->method.type == XRLT_VALUE_TEXT) {
                    ret->method.type = XRLT_VALUE_INT;

                    ret->method.intval = \
                        xrltIncludeMethodFromString(ret->method.textval);

                    xmlFree(ret->method.textval);
                    ret->method.textval = NULL;
                }

                ok = TRUE;
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

                ok = TRUE;
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

                ok = TRUE;
            }
            else if (ret->body.type == XRLT_VALUE_EMPTY &&
                     xmlStrEqual(tmp->name, XRLT_ELEMENT_WITH_BODY))
            {
                if (!xrltCompileCheckSubnodes(sheet, tmp, XRLT_ELEMENT_TEST,
                                              XRLT_ELEMENT_VALUE, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              &hasxrlt))
                {
                    goto error;
                }

                if (!xrltCompileValue(sheet, tmp, NULL, XRLT_ELEMENT_ATTR_TEST,
                                      NULL, XRLT_ELEMENT_TEST, FALSE,
                                      &ret->bodyTest))
                {
                    goto error;
                }

                if (!xrltCompileValue(sheet, tmp,
                                      hasxrlt ? NULL : tmp->children,
                                      XRLT_ELEMENT_ATTR_SELECT, NULL,
                                      XRLT_ELEMENT_VALUE, TRUE, &ret->body))
                {
                    goto error;
                }

                if (ret->body.type == XRLT_VALUE_EMPTY) {
                    XRLT_INCLUDE_NO_VALUE;
                }

                ok = TRUE;
            }
        }

        if (ret->success.type == XRLT_VALUE_EMPTY &&
                 xmlStrEqual(tmp->name, XRLT_ELEMENT_SUCCESS))
        {
            if (!xrltCompileValue(sheet, tmp, tmp->children,
                                  XRLT_ELEMENT_ATTR_SELECT, NULL, NULL, FALSE,
                                  &ret->success))
            {
                goto error;
            }

            if (ret->success.type == XRLT_VALUE_EMPTY) {
                XRLT_INCLUDE_NO_VALUE;
            }

            ok = TRUE;
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
                XRLT_INCLUDE_NO_VALUE;
            }

            ok = TRUE;
        }

        if (!ok) {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
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
        if (ret->name != NULL) { xmlFree(ret->name); }

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


void
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


static inline xrltProcessInputResult
xrltProcessHeader(xrltContextPtr ctx, xrltTransformValue *val,
                  xrltIncludeTransformingData *data)
{
    xmlNodePtr   tmp;
    xmlChar     *n = NULL;
    xmlChar     *v = NULL;

    if (val->type != XRLT_TRANSFORM_VALUE_STATUS) {
        if (val->type == XRLT_TRANSFORM_VALUE_HEADER) {
            if (data->hnode == NULL) {
                NEW_CHILD_GOTO(ctx, tmp, data->node, "h");
                data->hnode = tmp;
            } else {
                tmp = data->hnode;
            }
        } else { // val->type == XRLT_TRANSFORM_VALUE_COOKIE
            if (data->cnode == NULL) {
                NEW_CHILD_GOTO(ctx, tmp, data->node, "c");
                data->cnode = tmp;
            } else {
                tmp = data->cnode;
            }
        }

        n = xmlStrndup((const xmlChar *)val->headerval.name.data,
                       val->headerval.name.len);
        v = xmlStrndup((const xmlChar *)val->headerval.val.data,
                       val->headerval.val.len);

        // TODO: Check xmlNewDocNodeEatName, it might fit better here.
        if (xmlNewChild(tmp, NULL, n, v) == NULL) {
            ERROR_CREATE_NODE(ctx, NULL, data->srcNode);

            goto error;
        }

        xmlFree(n);
        xmlFree(v);
    } else { // val->type == XRLT_TRANSFORM_VALUE_STATUS
        data->status = val->statusval.status;
    }

    return XRLT_PROCESS_INPUT_AGAIN;

  error:
    if (n != NULL) { xmlFree(n); }
    if (v != NULL) { xmlFree(v); }

    return XRLT_PROCESS_INPUT_ERROR;
}


static inline xrltProcessInputResult
xrltProcessBody(xrltContextPtr ctx, xrltTransformValueSubrequestBody *val,
                xrltIncludeTransformingData *data)
{
    xmlNodePtr   tmp;

    if (data->doc == NULL && data->xmlparser == NULL) {
        // On the first call create a doc for the result to and a parser.

        if (data->type != XRLT_SUBREQUEST_DATA_XML) {
            // XRLT_SUBREQUEST_DATA_XML type will use the doc from xmlparser.
            data->doc = xmlNewDoc(NULL);

            if (data->doc == NULL) {
                ERROR_CREATE_NODE(ctx, NULL, data->srcNode);

                return XRLT_PROCESS_INPUT_ERROR;
            }
        }

        switch (data->type) {
            case XRLT_SUBREQUEST_DATA_XML:
                data->xmlparser = xmlCreatePushParserCtxt(
                    NULL, NULL, val->val.data, val->val.len,
                    (const char *)data->href
                );

                if (data->xmlparser == NULL) {
                    xrltTransformError(ctx, NULL, data->srcNode,
                                       "Failed to create parser\n");
                    return XRLT_PROCESS_INPUT_ERROR;
                }

                if (val->last) {
                    if (xmlParseChunk(data->xmlparser,
                                      val->val.data, 0, 1) != 0)
                    {
                        return XRLT_PROCESS_INPUT_REFUSE;
                    }
                }

                break;

            case XRLT_SUBREQUEST_DATA_JSON:
                data->jsonparser = xrltJSON2XMLInit((xmlNodePtr)data->doc,
                                                    FALSE);

                if (data->jsonparser == NULL) {
                    xrltTransformError(ctx, NULL, data->srcNode,
                                       "Failed to create parser\n");
                    return XRLT_PROCESS_INPUT_ERROR;
                }

                if (!xrltJSON2XMLFeed(data->jsonparser, val->val.data,
                                      val->val.len))
                {
                    return XRLT_PROCESS_INPUT_REFUSE;
                }

                break;

            case XRLT_SUBREQUEST_DATA_QUERYSTRING:
                data->qsparser = \
                    xrltQueryStringParserInit((xmlNodePtr)data->doc);

                if (data->qsparser == NULL) {
                    xrltTransformError(ctx, NULL, data->srcNode,
                                       "Failed to create parser\n");
                    return XRLT_PROCESS_INPUT_ERROR;
                }

                if (!xrltQueryStringParserFeed(data->qsparser,
                                               val->val.data, val->val.len,
                                               val->last))
                {
                    return XRLT_PROCESS_INPUT_REFUSE;
                }

                break;

            case XRLT_SUBREQUEST_DATA_TEXT:
                tmp = xmlNewTextLen((const xmlChar *)val->val.data,
                                    val->val.len);

                if (tmp == NULL ||
                    xmlAddChild((xmlNodePtr)data->doc, tmp) == NULL)
                {
                    if (tmp != NULL) { xmlFreeNode(tmp); }

                    ERROR_OUT_OF_MEMORY(ctx, NULL, data->srcNode);

                    return XRLT_PROCESS_INPUT_ERROR;
                }

                break;
        }
    } else {
        switch (data->type) {
            case XRLT_SUBREQUEST_DATA_XML:
                if (xmlParseChunk(data->xmlparser,
                                  val->val.data, val->val.len, val->last) != 0)
                {
                    return XRLT_PROCESS_INPUT_REFUSE;
                }

                break;

            case XRLT_SUBREQUEST_DATA_JSON:
                if (!xrltJSON2XMLFeed(data->jsonparser, val->val.data,
                                      val->val.len))
                {
                    return XRLT_PROCESS_INPUT_REFUSE;
                }

                break;

            case XRLT_SUBREQUEST_DATA_QUERYSTRING:
                if (!xrltQueryStringParserFeed(data->qsparser,
                                               val->val.data, val->val.len,
                                               val->last))
                {
                    return XRLT_PROCESS_INPUT_REFUSE;
                }

                break;

            case XRLT_SUBREQUEST_DATA_TEXT:
                tmp = xmlNewTextLen((const xmlChar *)val->val.data,
                                    val->val.len);

                if (tmp == NULL ||
                    xmlAddChild((xmlNodePtr)data->doc, tmp) == NULL)
                {
                    if (tmp != NULL) { xmlFreeNode(tmp); }

                    ERROR_OUT_OF_MEMORY(ctx, NULL, data->srcNode);

                    return XRLT_PROCESS_INPUT_ERROR;
                }

                break;
        }
    }

    if (val->last) {
        if (data->type == XRLT_SUBREQUEST_DATA_XML) {
            if (data->xmlparser->wellFormed == 0) {
                data->stage = XRLT_INCLUDE_TRANSFORM_FAILURE;
            } else {
                data->stage = XRLT_INCLUDE_TRANSFORM_SUCCESS;

                data->doc = data->xmlparser->myDoc;
                data->xmlparser->myDoc = NULL;
            }
        } else {
            data->stage = XRLT_INCLUDE_TRANSFORM_SUCCESS;
        }

        //xmlDocFormatDump(stderr, data->doc, 1);

        return XRLT_PROCESS_INPUT_DONE;
    }

    return XRLT_PROCESS_INPUT_AGAIN;
}


xrltProcessInputResult
xrltProcessInput(xrltContextPtr ctx, xrltTransformValue *val,
                 xrltIncludeTransformingData *data)
{
    if (val == NULL || data == NULL) { return XRLT_PROCESS_INPUT_ERROR; }

    if (data->stage != XRLT_INCLUDE_TRANSFORM_READ_RESPONSE &&
        data->comp != NULL &&
        data->comp->includeType == XRLT_INCLUDE_TYPE_INCLUDE)
    {
        xrltTransformError(ctx, NULL, data->srcNode,
                           "Wrong transformation stage\n");
        return XRLT_PROCESS_INPUT_ERROR;
    }

    switch (val->type) {
        case XRLT_TRANSFORM_VALUE_HEADER:
        case XRLT_TRANSFORM_VALUE_COOKIE:
        case XRLT_TRANSFORM_VALUE_STATUS:
            return xrltProcessHeader(ctx, val, data);

        case XRLT_TRANSFORM_VALUE_QUERYSTRING:
            if (ctx->querystring.data != NULL) {
                xrltTransformError(ctx, NULL, data->srcNode,
                                   "Querystring is already set\n");
                return XRLT_PROCESS_INPUT_ERROR;
            }

            if (val->querystringval.val.len > 0) {
                ctx->querystring.data = (char *)xmlStrndup(
                    (xmlChar *)val->querystringval.val.data,
                    val->querystringval.val.len
                );

                if (ctx->querystring.data == NULL) {
                    ERROR_OUT_OF_MEMORY(ctx, NULL, NULL);

                    return XRLT_PROCESS_INPUT_ERROR;
                }

                ctx->querystring.len = val->querystringval.val.len;
            }
            return XRLT_PROCESS_INPUT_AGAIN;

        case XRLT_TRANSFORM_VALUE_BODY:
            if (data->comp != NULL &&
                data->comp->includeType == XRLT_INCLUDE_TYPE_BODY &&
                data->stage != XRLT_INCLUDE_TRANSFORM_READ_RESPONSE)
            {
                return XRLT_PROCESS_INPUT_AGAIN;
            }

            return xrltProcessBody(ctx, &val->bodyval, data);

        case XRLT_TRANSFORM_VALUE_ERROR:
        case XRLT_TRANSFORM_VALUE_EMPTY:
            // These are processed earlier. It is an error if we've got here.
            xrltTransformError(ctx, NULL, data->srcNode, "Strange type\n");
            break;
    }

    return XRLT_PROCESS_INPUT_ERROR;
}


static xrltBool
xrltIncludeInputFunc(xrltContextPtr ctx, xrltTransformValue *val, void *data)
{
    xrltIncludeTransformingData  *tdata = (xrltIncludeTransformingData *)data;

    if (data == NULL) { return FALSE; }

    if (val->type == XRLT_TRANSFORM_VALUE_ERROR) {
        tdata->stage = XRLT_INCLUDE_TRANSFORM_FAILURE;

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform,
                          tdata->comp, tdata->insert, tdata);

        ctx->cur |= XRLT_STATUS_REFUSE_SUBREQUEST;

        return TRUE;
    }

    switch (xrltProcessInput(ctx, val, (xrltIncludeTransformingData *)data)) {
        case XRLT_PROCESS_INPUT_ERROR:
            return FALSE;

        case XRLT_PROCESS_INPUT_AGAIN:
            ctx->cur |= XRLT_STATUS_WAITING;

            break;

        case XRLT_PROCESS_INPUT_REFUSE:
            tdata->stage = XRLT_INCLUDE_TRANSFORM_FAILURE;

            ctx->cur |= XRLT_STATUS_REFUSE_SUBREQUEST;

            // No break here, schedule callback from XRLT_PROCESS_INPUT_DONE.

        case XRLT_PROCESS_INPUT_DONE:
            SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform,
                              tdata->comp, tdata->insert, tdata);
            break;
    }

    return TRUE;
}


static inline xrltBool
xrltIncludeAddSubrequest(xrltContextPtr ctx, xrltIncludeTransformingData *data)
{
    size_t              id;
    xrltHeaderOutType   htype;
    size_t              i, blen, qlen;
    xrltHeaderOutList   header;
    xrltString          href;
    xrltString          query;
    xrltString          body;
    char               *bp, *qp;
    char               *ebody = NULL;
    char               *equery = NULL;
    xrltBool            ret = TRUE;

    xrltHeaderOutListInit(&header);

    for (i = 0; i < data->headerCount; i++) {
        if (data->header[i].test) {
            htype = data->header[i].headerType;
            // Use query and body variables as name and value to reduce
            // declared variables count.
            xrltStringSet(&query, (char *)data->header[i].name);
            xrltStringSet(&body, (char *)data->header[i].val);

            if (!xrltHeaderOutListPush(&header, htype, &query, &body)) {
                ERROR_OUT_OF_MEMORY(ctx, NULL, data->srcNode);
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
            ERROR_OUT_OF_MEMORY(ctx, NULL, data->srcNode);
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

    id = xrltInputSubscribe(ctx, xrltIncludeInputFunc, data);

    if (id == 0) {
        ret = FALSE;
        goto error;
    }

    if (!xrltSubrequestListPush(&ctx->sr, id, data->method, data->type,
                                &header, &href, &query, &body))
    {
        ERROR_OUT_OF_MEMORY(ctx, NULL, data->srcNode);
        ret = FALSE;
        goto error;
    }

    ctx->cur |= XRLT_STATUS_SUBREQUEST;

  error:
    xrltHeaderOutListClear(&header);

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
    xrltCompiledValue            *r;

    if (data == NULL) {
        // First call.
        NEW_CHILD(ctx, node, insert, "i");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, icomp->node, tdata,
                    xrltIncludeTransformingData*,
                    sizeof(xrltIncludeTransformingData) +
                    sizeof(xrltTransformingParam) * icomp->headerCount +
                    sizeof(xrltTransformingParam) * icomp->paramCount, FALSE);

        tdata->header = (xrltTransformingParam*)(tdata + 1);

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
            TRANSFORM_TO_STRING(
                ctx, node, &icomp->method, &tdata->cmethod
            );
        }

        if (icomp->type.type == XRLT_VALUE_INT) {
            tdata->type = (xrltSubrequestDataType)icomp->type.intval;
        } else if (icomp->type.type != XRLT_VALUE_EMPTY) {
            TRANSFORM_TO_STRING(ctx, node, &icomp->type, &tdata->ctype);
        }

        if (icomp->bodyTest.type != XRLT_VALUE_EMPTY) {
            TRANSFORM_TO_BOOLEAN(
                ctx, node, &icomp->bodyTest, &tdata->bodyTest
            );
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
        tdata->comp = (xrltCompiledIncludeData *)comp;

        // Schedule the next call.
        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltIncludeTransform, comp, insert, tdata
        );
    } else {
        tdata = (xrltIncludeTransformingData*)data;

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
                    tdata->method = xrltIncludeMethodFromString(
                        tdata->cmethod
                    );
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
                    TRANSFORM_TO_STRING(
                        ctx, node, &icomp->body, &tdata->body
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
                        r = &icomp->success;
                    } else {
                        r = &icomp->failure;
                    }

                    NEW_CHILD(ctx, tdata->rnode, tdata->node, "tmp");

                    ASSERT_NODE_DATA(tdata->rnode, n);

                    n->root = tdata->doc;
                    n->sr = tdata;

                    switch (r->type) {
                        case XRLT_VALUE_TEXT:
                            node = xmlNewText(r->textval);

                            xmlAddChild(tdata->rnode, node);

                            tdata->stage = XRLT_INCLUDE_TRANSFORM_END;

                            SCHEDULE_CALLBACK(ctx, &ctx->tcb,
                                              xrltIncludeTransform, comp,
                                              insert, data);

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
                                if (tdata->doc->children != NULL) {
                                    node = xmlDocCopyNodeList(
                                        tdata->rnode->doc, tdata->doc->children
                                    );

                                    if (node == NULL) {
                                        ERROR_CREATE_NODE(
                                            ctx, NULL, tdata->srcNode
                                        );

                                        return FALSE;
                                    }

                                    if (xmlAddChildList(tdata->rnode,
                                                                 node) == NULL)
                                    {
                                        ERROR_ADD_NODE(
                                            ctx, NULL, tdata->srcNode
                                        );

                                        xmlFreeNodeList(node);

                                        return FALSE;
                                    }
                                }
                            }

                            tdata->stage = XRLT_INCLUDE_TRANSFORM_END;

                            return xrltIncludeTransform(ctx, comp, insert,
                                                        data);

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

                //xmlDocFormatDump(stderr, (xmlDocPtr)tdata->node, 1);

                REPLACE_RESPONSE_NODE(
                    ctx, tdata->node, tdata->rnode->children, tdata->srcNode
                );

                break;

            case XRLT_INCLUDE_TRANSFORM_READ_RESPONSE:
                xrltTransformError(ctx, NULL, tdata->srcNode,
                                   "Wrong transformation stage\n");
                return FALSE;
        }

    }

    return TRUE;
}


#define XRLT_REQUEST_INPUT_TRANSFORM_CASE                                     \
    case XRLT_PROCESS_INPUT_ERROR:                                            \
        return FALSE;                                                         \
    case XRLT_PROCESS_INPUT_AGAIN:                                            \
        ctx->cur |= XRLT_STATUS_WAITING;                                      \
        break;                                                                \
    case XRLT_PROCESS_INPUT_REFUSE:                                           \
        tdata->stage = XRLT_INCLUDE_TRANSFORM_FAILURE;                        \
        /* No break here, schedule callback from the next case. */            \
    case XRLT_PROCESS_INPUT_DONE:                                             \
        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltIncludeTransform,               \
                          tdata->comp, tdata->insert, tdata);                 \
        break;


xrltBool
xrltRequestInputTransform(xrltContextPtr ctx, void *val, xmlNodePtr insert,
                          void *data)
{
    xrltIncludeTransformingData       *tdata;
    xmlDocPtr                          vdoc;
    xmlChar                            id[sizeof(xmlNodePtr) * 7];
    xmlXPathObjectPtr                  varval = NULL;
    xmlNodePtr                         node;
    xrltTransformValue                *tval;
    xrltNodeDataPtr                    n;
    xrltTransformValueSubrequestBody   tmp;

    tdata = (xrltIncludeTransformingData *)data;

    if (val == NULL) {
        if (insert == NULL) {
            if (tdata->comp != NULL && tdata->comp->name != NULL) {
                // Initial call, need to create a variable.
                vdoc = xmlNewDoc(NULL);

                if (vdoc == NULL) {
                    ERROR_CREATE_NODE(ctx, NULL, tdata->srcNode);

                    return FALSE;
                }

                if (xmlAddChild(ctx->var, (xmlNodePtr)vdoc) == NULL) {
                    ERROR_ADD_NODE(ctx, NULL, tdata->srcNode);

                    xmlFreeDoc(vdoc);

                    return FALSE;
                }

                XRLT_SET_VARIABLE(
                    id, tdata->srcNode, tdata->comp->name,
                    tdata->comp->declScope, ctx->varScope, vdoc, varval
                );

                node = (xmlNodePtr)vdoc;

                NEW_CHILD(ctx, node, node, "r");

                tdata->node = node;

                COUNTER_INCREASE(ctx, node);

                if (tdata->comp->type.type == XRLT_VALUE_INT) {
                    tdata->type = \
                        (xrltSubrequestDataType)tdata->comp->type.intval;

                    tdata->stage = XRLT_INCLUDE_TRANSFORM_READ_RESPONSE;
                } else if (tdata->comp->type.type != XRLT_VALUE_EMPTY) {
                    NEW_CHILD(ctx, tdata->pnode, node, "t");

                    COUNTER_INCREASE(ctx, tdata->pnode);

                    TRANSFORM_TO_STRING(
                        ctx, tdata->pnode, &tdata->comp->type, &tdata->ctype
                    );

                    SCHEDULE_CALLBACK(
                        ctx, &ctx->tcb, xrltRequestInputTransform, NULL, node,
                        data
                    );
                }
            }

            return TRUE;
        } else {
            // We should get here if we needed to evaluate tdata->comp->type.
            node = tdata->pnode;

            ASSERT_NODE_DATA(node, n);

            if (n->count > 0) {
                COUNTER_DECREASE(ctx, node);
            }

            if (n->count > 0) {
                SCHEDULE_CALLBACK(
                    ctx, &n->tcb, xrltRequestInputTransform, NULL,
                    node, data
                );

                return TRUE;
            }

            // Type should be ready.
            if (tdata->ctype != NULL) {
                tdata->type = xrltIncludeTypeFromString(tdata->ctype);
            }

            tdata->stage = XRLT_INCLUDE_TRANSFORM_READ_RESPONSE;

            if (tdata->comp->includeType == XRLT_INCLUDE_TYPE_QUERYSTRING) {
                tmp.last = TRUE;
                memcpy(&tmp.val, &ctx->querystring, sizeof(xrltString));

                switch (xrltProcessBody(ctx, &tmp, tdata)) {
                    XRLT_REQUEST_INPUT_TRANSFORM_CASE
                }

                return TRUE;
            } else { // XRLT_INCLUDE_TYPE_BODY
                if (ctx->bodyBuf != NULL) {
                    tmp.last = ctx->bodyBufComplete;
                    tmp.val.data = (char *)xmlBufferContent(ctx->bodyBuf);
                    tmp.val.len = (size_t)xmlBufferLength(ctx->bodyBuf);

                    switch (xrltProcessBody(ctx, &tmp, tdata)) {
                        XRLT_REQUEST_INPUT_TRANSFORM_CASE
                    }

                    xmlBufferFree(ctx->bodyBuf);
                    ctx->bodyBuf = NULL;
                    ctx->bodyBufComplete = FALSE;

                    return TRUE;
                }
            }
        }
    } else {
        tval = (xrltTransformValue *)val;

        switch (xrltProcessInput(ctx, tval, tdata)) {
            XRLT_REQUEST_INPUT_TRANSFORM_CASE
        }

        if (tdata->stage != XRLT_INCLUDE_TRANSFORM_READ_RESPONSE &&
            tdata->comp != NULL &&
            tdata->comp->includeType == XRLT_INCLUDE_TYPE_BODY)
        {
            if (ctx->bodyBuf == NULL) {
                ctx->bodyBuf = xmlBufferCreate();

                if (ctx->bodyBuf == NULL) {
                    xrltTransformError(ctx, NULL, tdata->srcNode,
                                       "Failed to create body buffer\n");
                    return FALSE;
                }
            }

            if (tval->bodyval.val.len > 0 &&
                xmlBufferAdd(ctx->bodyBuf, (xmlChar *)tval->bodyval.val.data,
                             tval->bodyval.val.len) != 0)
            {
                xrltTransformError(ctx, NULL, tdata->srcNode,
                                   "Failed to add body to buffer\n");
                return FALSE;
            }

            ctx->bodyBufComplete = tval->bodyval.last;
        }
        else if (tval->type == XRLT_TRANSFORM_VALUE_QUERYSTRING &&
                 tdata->stage == XRLT_INCLUDE_TRANSFORM_READ_RESPONSE)
        {
            tmp.last = TRUE;
            memcpy(&tmp.val, &ctx->querystring, sizeof(xrltString));

            switch (xrltProcessBody(ctx, &tmp, tdata)) {
                XRLT_REQUEST_INPUT_TRANSFORM_CASE
            }
        }
    }

    return TRUE;
}
