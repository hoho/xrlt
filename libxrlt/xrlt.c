#include <string.h>

#include <xrlt.h>


typedef struct {
    xrltHeaderList   header;
} xrltContextInternal;


xrltContextPtr
xrltContextCreate(xrltRequestsheetPtr sheet, xrltHeaderList *header)
{
    xrltContextPtr        ret;
    xrltContextInternal  *internal;

    if (sheet == NULL) { return NULL; }

    ret = xrltMalloc(sizeof(xrltContext) + sizeof(xrltContextInternal));
    if (ret == NULL) { goto error; }

    memset(ret, 0, sizeof(xrltContext) + sizeof(xrltContextInternal));

    internal = (xrltContextInternal *)(ret + 1);

    ret->sheet = sheet;
    ret->_internal = internal;

    if (header != NULL) {
        internal->header.first = header->first;
        internal->header.last = header->last;
        header->first = header->last = NULL;
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
    xrltContextInternal  *internal;

    if (ctx == NULL) { return; }

    internal = ctx->_internal;

    if (internal != NULL) {
        xrltHeaderListClear(&internal->header);
    }

    xrltFree(ctx);
}


int
xrltTransform(xrltContextPtr ctx, xrltTransformValue *data)
{
    return XRLT_STATUS_DONE;
}
