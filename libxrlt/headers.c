#include "transform.h"
#include "headers.h"


void *
xrltResponseHeaderCompile(xrltRequestsheetPtr sheet, xmlNodePtr node,
                          void *prevcomp)
{
    xrltResponseHeaderData  *ret = NULL;
    int                      _test;

    XRLT_MALLOC(ret, xrltResponseHeaderData*, sizeof(xrltResponseHeaderData),
                "xrltResponseHeaderCompile", NULL);

    if (!xrltCompileTestNameValueNode(sheet, node,
                                      XRLT_TESTNAMEVALUE_TEST_ATTR |
                                      XRLT_TESTNAMEVALUE_TEST_NODE |
                                      XRLT_TESTNAMEVALUE_NAME_ATTR |
                                      XRLT_TESTNAMEVALUE_NAME_NODE |
                                      XRLT_TESTNAMEVALUE_VALUE_ATTR |
                                      XRLT_TESTNAMEVALUE_VALUE_NODE |
                                      XRLT_TESTNAMEVALUE_NAME_REQUIRED,
                                      &_test, &ret->ntest, &ret->xtest,
                                      NULL, NULL, NULL,
                                      &ret->name, &ret->nname, &ret->xname,
                                      &ret->val, &ret->nval, &ret->xval))
    {
        goto error;
    }

    if (_test > 0) {
        ret->test = _test == 1 ? TRUE : FALSE;
    } else if (ret->ntest == NULL && ret->xtest == NULL) {
        ret->test = TRUE;
    }

    ret->node = node;

    return ret;

  error:
    xrltResponseHeaderFree(ret);

    return NULL;
}


void
xrltResponseHeaderFree(void *comp)
{
    xrltResponseHeaderData  *ret = (xrltResponseHeaderData *)comp;
    if (ret != NULL) {
        if (ret->xtest) { xmlXPathFreeCompExpr(ret->xtest); }

        if (ret->name != NULL) { xmlFree(ret->name); }
        if (ret->xname != NULL) { xmlXPathFreeCompExpr(ret->xname); }

        if (ret->val != NULL) { xmlFree(ret->val); }
        if (ret->xval != NULL) { xmlXPathFreeCompExpr(ret->xval); }

        xrltFree(ret);
    }
}


static void
xrltResponseHeaderTransformingFree(void *data)
{
    xrltResponseHeaderTransformingData  *tdata;

    if (data != NULL) {
        tdata = (xrltResponseHeaderTransformingData *)data;

        if (tdata->name != NULL) { xmlFree(tdata->name); }
        if (tdata->val != NULL) { xmlFree(tdata->val); }

        xrltFree(data);
    }
}


xrltBool
xrltResponseHeaderTransform(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                            void *data)
{
    if (ctx == NULL || comp == NULL || insert == NULL) { return FALSE; }

    xmlNodePtr                           node;
    xrltResponseHeaderData              *hcomp = (xrltResponseHeaderData *)comp;
    xrltNodeDataPtr                      n;
    xrltResponseHeaderTransformingData  *tdata;

    if (data == NULL) {
        NEW_CHILD(ctx, node, insert, "r-h");

        ASSERT_NODE_DATA(node, n);

        XRLT_MALLOC(tdata, xrltResponseHeaderTransformingData*,
                    sizeof(xrltResponseHeaderTransformingData),
                    "xrltResponseHeaderTransform", FALSE);

        n->data = tdata;
        n->free = xrltResponseHeaderTransformingFree;

        COUNTER_INCREASE(ctx, node);

        if (hcomp->test) {
            TRANSFORM_TO_STRING(
                ctx, node, hcomp->name, hcomp->nname, hcomp->xname, &tdata->name
            );

            TRANSFORM_TO_STRING(
                ctx, node, hcomp->val, hcomp->nval, hcomp->xval, &tdata->val
            );
        } else {
            TRANSFORM_TO_BOOLEAN(
                ctx, node, hcomp->test, hcomp->ntest, hcomp->xtest, &tdata->test
            );
        }

        SCHEDULE_CALLBACK(
            ctx, &ctx->tcb, xrltResponseHeaderTransform, comp, insert, tdata
        );
    } else {

    }

    return TRUE;
}
