#ifndef __XRLT_H__
#define __XRLT_H__


#include <libxml/tree.h>

#include <xrltstruct.h>
#include <xrltexports.h>


#ifdef __cplusplus
extern "C" {
#endif


#define XRLT_NS (const xmlChar *)"http://xrlt.net/Transform"


#define XRLT_STATUS_UNKNOWN      0
#define XRLT_STATUS_ERROR        2
#define XRLT_STATUS_DONE         4
#define XRLT_STATUS_HEADER       8
#define XRLT_STATUS_SUBREQUEST   16
#define XRLT_STATUS_CHUNK        32
#define XRLT_STATUS_LOG          64


typedef enum {
    XRLT_PROCESS_REQUEST_BODY,
    XRLT_PROCESS_SUBREQUEST_HEADER,
    XRLT_PROCESS_SUBREQUEST_BODY
} xrltTransformValueType;


typedef struct {
    xrltTransformValueType   type;
    size_t                   id;
    xrltBool                 last;
    xrltString               data;
} xrltTransformValue;


typedef struct _xrltRequestsheet xrltRequestsheet;
typedef xrltRequestsheet* xrltRequestsheetPtr;
struct _xrltRequestsheet {
    xmlDocPtr   doc;
};


typedef struct _xrltContext xrltContext;
typedef xrltContext* xrltContextPtr;
struct _xrltContext {
    xrltRequestsheetPtr   sheet;

    xrltHeaderList        header;
    xrltSubrequestList    sr;
    xrltChunkList         chunk;
    xrltLogList           log;

    void                *_internal;
};


XRLTPUBFUN xrltContextPtr XRLTCALL
        xrltContextCreate   (xrltRequestsheetPtr sheet, xrltHeaderList *header);
XRLTPUBFUN void XRLTCALL
        xrltContextFree     (xrltContextPtr ctx);
XRLTPUBFUN int XRLTCALL
        xrltTransform       (xrltContextPtr ctx, xrltTransformValue *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_H__ */
