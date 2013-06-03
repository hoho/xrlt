/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include "transform.h"
#include "builtins.h"


void *
xrltResponseCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    if (sheet->response == NULL) {
        sheet->response = node;
        return node;
    } else {
        xrltTransformError(NULL, sheet, node, "Duplicate response element\n");
        return NULL;
    }
}


void
xrltResponseFree(void *comp)
{
    (void)comp;
}


xrltBool
xrltResponseTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                      void *data)
{
    if (ctx == NULL) { return FALSE; }

    xmlNodePtr           response;

    if (data == NULL) {
        ctx->varScope = ++ctx->maxVarScope;

        TRANSFORM_SUBTREE(ctx, ((xmlNodePtr)comp)->children, ctx->response);

        // Schedule the next call.
        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltResponseTransform, comp, NULL, (void *)0x1
        );
    } else {
        // On the second call, check if something is ready to be sent and send
        // it if it is.
        xrltNodeDataPtr   n;
        xrltString        chunk;
        xrltBool          pushed;

        response = ctx->response;

        if (data == (void *)0x1) {
            // It's the second call.
            ctx->responseCur = response->children;
        }

        while (ctx->responseCur != NULL) {
            ASSERT_NODE_DATA(ctx->responseCur, n);

            if (n->count > 0) {
                break;
            }

            // Send response chunk out.
            // TODO: Gather as many response chunks as possible into one buffer.
            chunk.data = (char *)xmlXPathCastNodeToString(ctx->responseCur);

            if (chunk.data != NULL) {
                chunk.len = strlen(chunk.data);

                pushed = chunk.len > 0
                    ?
                    xrltChunkListPush(&ctx->chunk, &chunk)
                    :
                    TRUE;

                xmlFree(chunk.data);

                if (!pushed) {
                    xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                                       "Failed to push response chunk\n");
                    return FALSE;
                }

                if (chunk.len > 0) {
                    ctx->cur |= XRLT_STATUS_CHUNK;
                }
            }

            ctx->responseCur = ctx->responseCur->next;
        }

        if (ctx->responseCur != NULL) {
            // We still have some data that's not ready, schedule the next call.
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltResponseTransform, comp,
                              response, (void *)0x2);
        }
    }

    return TRUE;
}


void *
xrltLogCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltLogData  *ret = NULL;
    xmlChar      *level = NULL;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltLogData*, sizeof(xrltLogData),
                NULL);

    level = xmlGetProp(node, (const xmlChar *)"level");

    if (level == NULL) {
        ret->type = XRLT_INFO;
    } else {
        if (xmlStrEqual(level, (const xmlChar *)"warning")) {
            ret->type = XRLT_WARNING;
        } else if (xmlStrEqual(level, (const xmlChar *)"error")) {
            ret->type = XRLT_ERROR;
        } else if (xmlStrEqual(level, (const xmlChar *)"info")) {
            ret->type = XRLT_INFO;
        } else if (xmlStrEqual(level, (const xmlChar *)"debug")) {
            ret->type = XRLT_DEBUG;
        } else {
            xrltTransformError(NULL, sheet, node,
                               "Unknown 'level' attribute value\n");
            xmlFree(level);
            xrltLogFree(ret);
            return NULL;
        }

        xmlFree(level);
    }

    ret->node = node;

    return ret;
}


void
xrltLogFree(void *comp)
{
    if (comp != NULL) {
        xmlFree(comp);
    }
}


xrltBool
xrltLogTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert, void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr           log;
    xrltLogData         *logcomp = (xrltLogData *)comp;
    xrltNodeDataPtr      n;

    if (data == NULL) {
        // On the first call, create log parent node.

        if (logcomp->node->children == NULL) {
            // Just skip empty log element.
            return TRUE;
        }

        NEW_CHILD(ctx, log, insert, "log");

        // Schedule log content transforms.
        TRANSFORM_SUBTREE(ctx, logcomp->node->children, log);

        COUNTER_INCREASE(ctx, log);

        // Schedule the next call.
        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltLogTransform, comp, insert, (void *)log
        );
    } else {
        log = (xmlNodePtr)data;

        ASSERT_NODE_DATA(log, n);

        if (n->count > 0) {
            COUNTER_DECREASE(ctx, log);

            if (n->count > 0) {
                // Node is not ready. Schedule the third call of this function
                // inside node's personal ready queue and return.
                SCHEDULE_CALLBACK(
                    ctx, &n->tcb, xrltLogTransform, comp, insert, (void *)log
                );

                return TRUE;
            }
        }

        // Node is ready, sent it's contents out.
        xrltString   msg;
        xrltBool     pushed;

        msg.data = (char *)xmlXPathCastNodeToString(log);

        REMOVE_RESPONSE_NODE(ctx, log);

        if (msg.data != NULL) {
            msg.len = strlen(msg.data);

            pushed = msg.len > 0
                ?
                xrltLogListPush(&ctx->log, logcomp->type, &msg)
                :
                TRUE;

            xmlFree(msg.data);

            if (!pushed) {
                xrltTransformError(ctx, NULL, (xmlNodePtr)comp,
                                   "Failed to push log message\n");
                return FALSE;
            }

            if (msg.len > 0) {
                ctx->cur |= XRLT_STATUS_LOG;
            }
        }
    }

    return TRUE;
}


void *
xrltIfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltIfData  *ret = NULL;
    xmlChar     *expr = NULL;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltIfData*, sizeof(xrltIfData), NULL);

    expr = xmlGetProp(node, XRLT_ELEMENT_ATTR_TEST);

    if (expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "No 'test' attribute\n");
        goto error;
    }

    ret->test.src = node;
    ret->test.scope = node->parent;
    ret->test.expr = xmlXPathCompile(expr);

    if (ret->test.expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to compile expression\n");
        goto error;
    }

    xmlFree(expr);

    ret->children = node->children;

    return ret;

  error:
    if (expr) { xmlFree(expr); }
    xrltIfFree(ret);

    return NULL;
}


void
xrltIfFree(void *comp)
{
    if (comp != NULL) {
        xmlXPathFreeCompExpr(((xrltIfData *)comp)->test.expr);
        xmlFree(comp);
    }
}


xrltBool
xrltIfTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert, void *data)
{
    return TRUE;
}


void *
xrltChooseCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltChooseData      *ret = NULL;
    xmlChar             *expr = NULL;
    int                  i;
    xmlNodePtr           tmp;
    xrltBool             otherwise = FALSE;
    xrltNodeDataPtr      n;

    if (node->children == NULL) {
        xrltTransformError(NULL, sheet, node, "Element is empty\n");
    }

    i = 0;
    tmp = node->children;

    while (tmp != NULL) {
        i++;
        tmp = tmp->next;
    }

    XRLT_MALLOC(NULL, sheet, node, ret, xrltChooseData*,
                sizeof(xrltChooseData) + sizeof(xrltChooseCaseData) * i, NULL);

    ret->node = node;

    ret->cases = (xrltChooseCaseData *)(ret + 1);
    ret->len = i;

    i = 0;
    tmp = node->children;

    while (tmp != NULL) {
        if (otherwise || tmp->ns == NULL ||
            !xmlStrEqual(tmp->ns->href, XRLT_NS))
        {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
            goto error;
        }

        if (xmlStrEqual(tmp->name, (const xmlChar *)"otherwise")) {
            if (node->children == tmp) {
                ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
                goto error;
            }

            otherwise = TRUE;
        } else if (xmlStrEqual(tmp->name, (const xmlChar *)"when")) {
            expr = xmlGetProp(tmp, XRLT_ELEMENT_ATTR_TEST);

            if (expr == NULL) {
                xrltTransformError(NULL, sheet, tmp, "No 'test' attribute\n");
                goto error;
            }

            ret->cases[i].test.xpathval.expr = xmlXPathCompile(expr);

            xmlFree(expr);

            ret->cases[i].test.type = XRLT_VALUE_XPATH;

            if (ret->cases[i].test.xpathval.expr == NULL) {
                xrltTransformError(NULL, sheet, tmp,
                                   "Failed to compile expression\n");
                goto error;
            }

            ret->cases[i].test.xpathval.src = tmp;
            ret->cases[i].test.xpathval.scope = node->parent;
        } else {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, tmp);
            goto error;
        }

        ret->cases[i].children = tmp->children;

        ASSERT_NODE_DATA_GOTO(tmp, n);

        n->xrlt = TRUE;

        i++;
        tmp = tmp->next;
    }

    return ret;

  error:
    xrltChooseFree(ret);

    return NULL;
}


void
xrltChooseFree(void *comp)
{
    if (comp != NULL) {
        xrltChooseData  *d;
        int              i;

        d = (xrltChooseData *)comp;

        for (i = 0; i < d->len; i++)  {
            if (d->cases[i].test.xpathval.expr != NULL) {
                xmlXPathFreeCompExpr(d->cases[i].test.xpathval.expr);
            }
        }

        xmlFree(d);
    }

}


static void
xrltChooseTransformingFree(void *data)
{
    if (data != NULL) {
        xmlFree(data);
    }
}


xrltBool
xrltChooseTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                    void *data)
{
    xrltChooseData              *ccomp = (xrltChooseData *)comp;
    xrltChooseTransformingData  *tdata;
    xmlNodePtr                   node;
    xrltNodeDataPtr              n;

    if (data == NULL) {
        if (ccomp->len == 0) {
            return TRUE;
        }

        NEW_CHILD(ctx, node, insert, "c");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, ccomp->node, tdata, xrltChooseTransformingData*,
                    sizeof(xrltChooseTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltChooseTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "tmp");

        tdata->testNode = node;

        TRANSFORM_TO_BOOLEAN(ctx, node, &ccomp->cases[0].test, &tdata->testRet);

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltChooseTransform, comp, insert,
                          tdata);
    } else {
        tdata = (xrltChooseTransformingData *)data;

        node = tdata->retNode == NULL ? tdata->testNode : tdata->retNode;

        ASSERT_NODE_DATA(node, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltChooseTransform, comp, insert,
                              tdata);
            return TRUE;
        }

        if (tdata->retNode == NULL) {
            if (tdata->testRet) {
                NEW_CHILD(ctx, tdata->retNode, tdata->node, "r");

                TRANSFORM_SUBTREE(
                    ctx, ccomp->cases[tdata->pos].children, tdata->retNode
                );

                SCHEDULE_CALLBACK(
                    ctx, &ctx->tcb, xrltChooseTransform, comp, insert, tdata
                );

                return TRUE;
            } else {
                tdata->pos++;

                if (tdata->pos < ccomp->len) {
                    if (ccomp->cases[tdata->pos].test.type == XRLT_VALUE_EMPTY)
                    {
                        tdata->testRet = TRUE;

                        return xrltChooseTransform(ctx, comp, insert, data);
                    } else {
                        TRANSFORM_TO_BOOLEAN(
                            ctx, node, &ccomp->cases[tdata->pos].test,
                            &tdata->testRet
                        );

                        SCHEDULE_CALLBACK(
                            ctx, &ctx->tcb, xrltChooseTransform, comp, insert,
                            tdata
                        );

                        return TRUE;
                    }
                } else {
                    COUNTER_DECREASE(ctx, tdata->node);

                    REMOVE_RESPONSE_NODE(ctx, tdata->node);
                }
            }
        } else {
            COUNTER_DECREASE(ctx, tdata->node);

            REPLACE_RESPONSE_NODE(
                ctx, tdata->node, tdata->retNode->children, ccomp->node
            );
        }
    }

    return TRUE;
}


void *
xrltValueOfCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{
    xrltValueOfData  *ret = NULL;
    xmlChar          *select = NULL;

    if (node->children != NULL) {
        xrltTransformError(NULL, sheet, node, "Element can't have content\n");
        return NULL;
    }

    XRLT_MALLOC(NULL, sheet, node, ret, xrltValueOfData*,
                sizeof(xrltValueOfData), NULL);

    select = xmlGetProp(node, XRLT_ELEMENT_ATTR_SELECT);

    if (select == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to get 'select' attribute\n");
        goto error;
    }

    ret->select.type = XRLT_VALUE_XPATH;
    ret->select.xpathval.src = node;
    ret->select.xpathval.scope = node->parent;
    ret->select.xpathval.expr = xmlXPathCompile(select);

    if (ret->select.xpathval.expr == NULL) {
        xrltTransformError(NULL, sheet, node,
                           "Failed to compile expression\n");
        goto error;
    }

    xmlFree(select);

    ret->node = node;

    return ret;

  error:
    if (select) { xmlFree(select); }
    xrltValueOfFree(ret);

    return NULL;
}


void
xrltValueOfFree(void *comp)
{
    if (comp != NULL) {
        CLEAR_XRLT_VALUE(((xrltValueOfData *)comp)->select);
        xmlFree(comp);
    }
}


static void
xrltValueOfTransformingFree(void *data)
{
    xrltValueOfTransformingData  *tdata;

    if (data != NULL) {
        tdata = (xrltValueOfTransformingData *)data;

        if (tdata->val != NULL) { xmlFree(tdata->val); }

        xmlFree(data);
    }
}


xrltBool
xrltValueOfTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                     void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr                    node;
    xrltValueOfData              *vcomp = (xrltValueOfData *)comp;
    xrltNodeDataPtr               n;
    xrltValueOfTransformingData  *tdata;

    if (data == NULL) {
        NEW_CHILD(ctx, node, insert, "v-o");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(ctx, NULL, vcomp->node, tdata, xrltValueOfTransformingData*,
                    sizeof(xrltValueOfTransformingData), FALSE);

        n->data = tdata;
        n->free = xrltValueOfTransformingFree;

        COUNTER_INCREASE(ctx, node);

        tdata->node = node;

        NEW_CHILD(ctx, node, node, "tmp");

        tdata->dataNode = node;

        TRANSFORM_TO_STRING(ctx, node, &vcomp->select, &tdata->val);

        SCHEDULE_CALLBACK(ctx, &ctx->tcb, xrltValueOfTransform, comp, insert,
                          tdata);
    } else {
        tdata = (xrltValueOfTransformingData *)data;

        ASSERT_NODE_DATA(tdata->dataNode, n);

        if (n->count > 0) {
            SCHEDULE_CALLBACK(ctx, &n->tcb, xrltValueOfTransform, comp, insert,
                              data);

            return TRUE;
        }

        node = xmlNewText(tdata->val);

        if (node == NULL) {
            ERROR_CREATE_NODE(ctx, NULL, vcomp->node);
            return FALSE;
        }

        if (xmlAddNextSibling(tdata->node, node) == NULL) {
            ERROR_ADD_NODE(ctx, NULL, vcomp->node);
            xmlFreeNode(node);
            return FALSE;
        }

        COUNTER_DECREASE(ctx, tdata->node);

        REMOVE_RESPONSE_NODE(ctx, tdata->node);
    }

    return TRUE;
}


void *
xrltTextCompile(xrltRequestsheetPtr sheet, xmlNodePtr node, void *prevcomp)
{

    xrltTextData  *ret = NULL;
    xmlNodePtr     tmp;
    xmlBufferPtr   buf = NULL;

    XRLT_MALLOC(NULL, sheet, node, ret, xrltTextData*, sizeof(xrltTextData),
                NULL);

    tmp = node->children;

    while (tmp != NULL) {
        if (tmp->type != XML_TEXT_NODE && tmp->type != XML_CDATA_SECTION_NODE)
        {
            ERROR_UNEXPECTED_ELEMENT(NULL, sheet, node);
            goto error;
        }
        tmp = tmp->next;
    }

    buf = xmlBufferCreateSize(64);
    if (buf == NULL) {
        xrltTransformError(NULL, sheet, node, "Failed to create buffer\n");
        goto error;
    }

    tmp = node->children;

    while (tmp != NULL) {
        xmlBufferCat(buf, tmp->content);
        tmp = tmp->next;
    }

    ret->text = xmlStrdup(xmlBufferContent(buf));

    xmlBufferFree(buf);
    buf = NULL;

    ret->node = node;

    return ret;

  error:
    if (buf != NULL) { xmlBufferFree(buf); }

    xrltTextFree(ret);

    return NULL;

}


void
xrltTextFree(void *comp)
{
    if (comp) {
        xrltTextData  *tcomp = (xrltTextData *)comp;

        if (tcomp->text != NULL) { xmlFree(tcomp->text); }

        xmlFree(tcomp);
    }
}


xrltBool
xrltTextTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                  void *data)
{
    xrltTextData  *tcomp = (xrltTextData *)comp;
    xmlNodePtr     node;

    if (tcomp->text != NULL) {
        node = xmlNewText(tcomp->text);

        if (node == NULL) {
            ERROR_CREATE_NODE(ctx, NULL, tcomp->node);
            return FALSE;
        }

        if (xmlAddChild(insert, node) == NULL) {
            ERROR_ADD_NODE(ctx, NULL, tcomp->node);
            xmlFreeNode(node);
            return FALSE;
        }
    }

    return TRUE;
}
