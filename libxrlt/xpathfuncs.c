#include "transform.h"
#include "include.h"
#include "xpathfuncs.h"


inline static xrltContextPtr
xrltXPathGetTransformContext(xmlXPathParserContextPtr ctxt)
{
    if (ctxt == NULL || ctxt->context == NULL) {
        return NULL;
    }

    return (xrltContextPtr)ctxt->context->extra;
}


static void
xrltStatusFunction(xmlXPathParserContextPtr ctxt, int nargs)
{
    xrltContextPtr                xctx;
    xmlNodePtr                    node;
    xrltNodeDataPtr               n;
    xrltIncludeTransformingData  *sr;

    CHECK_ARITY(0);

    xctx = xrltXPathGetTransformContext(ctxt);

    if (xctx == NULL) {
        XP_ERROR(XPATH_INVALID_CTXT);
    }

    node = xctx->xpathContext;

    do {
        n = (xrltNodeDataPtr)node->_private;
        node = node->parent;
    } while (node != NULL && n->sr == NULL);

    sr = n->sr == NULL ?
            NULL : (xrltIncludeTransformingData *)n->sr;

    if (sr == NULL) {
        XP_ERROR(XPATH_UNKNOWN_FUNC_ERROR);
    }

    xmlXPathReturnNumber(ctxt, sr->status);
}


xrltBool
xrltRegisterFunctions(xmlXPathContextPtr ctxt)
{
    if (xmlXPathRegisterFunc(ctxt, (const xmlChar *)"status",
                             xrltStatusFunction) != 0)
    {
        return FALSE;
    }

    return TRUE;
}
