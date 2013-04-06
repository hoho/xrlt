#include <string.h>

#include "xrlt.h"
#include "transform.h"
#include "xrlterror.h"



typedef struct {
    xrltHeaderList   header;
    xmlDocPtr        responseDoc;
} xrltContextPrivate;


xrltRequestsheetPtr
xrltRequestsheetCreate(xmlDocPtr doc)
{
    if (doc == NULL) { return NULL; }

    xrltRequestsheetPtr       ret;
    xrltRequestsheetPrivate  *priv;
    xmlNodePtr                root;

    ret = (xrltRequestsheetPtr)xrltMalloc(sizeof(xrltRequestsheet) +
                                          sizeof(xrltRequestsheetPrivate));
    if (ret == NULL) {
        xrltTransformError(NULL, NULL, NULL,
                           "xrltRequestsheetCreate: Out of memory\n");
        return NULL;
    }

    memset(ret, 0, sizeof(xrltRequestsheet) + sizeof(xrltRequestsheetPrivate));

    priv = (xrltRequestsheetPrivate *)(ret + 1);

    ret->_private = priv;

    root = xmlDocGetRootElement(doc);

    if (root == NULL ||
        !xmlStrEqual(root->name, XRLT_ROOT_NAME) ||
        root->ns == NULL ||
        !xmlStrEqual(root->ns->href, XRLT_NS))
    {
        xrltTransformError(NULL, NULL, root, "Unexpected element\n");
        goto error;
    }

    if (!xrltElementCompile(ret, root->children)) {
        goto error;
    }

    ret->doc = doc;

    return ret;

  error:
    xrltRequestsheetFree(ret);

    return NULL;
}


void
xrltRequestsheetFree(xrltRequestsheetPtr sheet)
{
    if (sheet == NULL) { return; }

    xrltRequestsheetPrivate  *priv = sheet->_private;
    size_t                    i;

    if (priv->compiled != NULL) {
        for (i = 1; i < priv->compiledLen; i++) {
            if (priv->compiled[i].data != NULL) {
                priv->compiled[i].free(priv->compiled[i].data);
            }
        }

        xrltFree(priv->compiled);
    }

    if (sheet->doc != NULL) {
        xmlFreeDoc(sheet->doc);
    }

    xrltFree(sheet);
}


xrltContextPtr
xrltContextCreate(xrltRequestsheetPtr sheet, xrltHeaderList *header)
{
    xrltContextPtr       ret;
    xrltContextPrivate  *priv;

    if (sheet == NULL) { return NULL; }

    ret = xrltMalloc(sizeof(xrltContext) + sizeof(xrltContextPrivate));
    if (ret == NULL) {
        xrltTransformError(NULL, NULL, NULL,
                           "xrltContextCreate: Out of memory\n");
        goto error;
    }

    memset(ret, 0, sizeof(xrltContext) + sizeof(xrltContextPrivate));

    priv = (xrltContextPrivate *)(ret + 1);

    ret->sheet = sheet;
    ret->_private = priv;

    if (header != NULL) {
        priv->header.first = header->first;
        priv->header.last = header->last;
        header->first = NULL;
        header->last = NULL;
    }

    return ret;

  error:
    if (ret != NULL) {
        xrltFree(ret);
    }

    return NULL;
}


void
xrltContextFree(xrltContextPtr ctx)
{
    xrltContextPrivate  *priv;

    if (ctx == NULL) { return; }

    priv = ctx->_private;

    if (priv != NULL) {
        xrltHeaderListClear(&priv->header);
    }

    xrltFree(ctx);
}


int
xrltTransform(xrltContextPtr ctx, xrltTransformValue *data)
{
    return XRLT_STATUS_DONE;
}


void
xrltCleanup(void)
{
    xrltUnregisterBuiltinElements();
}
