#ifndef __XRLT_H__
#define __XRLT_H__


#include <libxml/tree.h>
#include <xrltstruct.h>
#include <xrltexports.h>


#ifdef __cplusplus
extern "C" {
#endif


#define XRLT_NS (const xmlChar *)"http://xrlt.net/Transform"
#define XRLT_ROOT_NAME (const xmlChar *)"requestsheet"


#define XRLT_STATUS_UNKNOWN      0
#define XRLT_STATUS_ERROR        2
#define XRLT_STATUS_DONE         4
#define XRLT_STATUS_HEADER       8
#define XRLT_STATUS_SUBREQUEST   16
#define XRLT_STATUS_CHUNK        32
#define XRLT_STATUS_LOG          64


#define XRLT_REGISTER_TOPLEVEL   2
#define XRLT_COMPILE_PASS1       4
#define XRLT_COMPILE_PASS2       8


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
    void       *_private;
};


typedef struct _xrltContext xrltContext;
typedef xrltContext* xrltContextPtr;
struct _xrltContext {
    xrltRequestsheetPtr   sheet;

    xrltBool              error;

    xrltHeaderList        header;
    xrltSubrequestList    sr;
    xrltChunkList         chunk;
    xrltLogList           log;

    void                 *_private;
};


typedef void *   (*xrltCompileFunction)     (xrltRequestsheetPtr sheet,
                                             xmlNodePtr node, void *prevcomp);
typedef void     (*xrltFreeFunction)        (void *data);
typedef xrltBool (*xrltTransformFunction)   (xrltContextPtr ctx, void *data);


typedef xrltBool (*xrltNodeReadyCallback)   (xrltContextPtr ctx,
                                             xmlNodePtr node, void *data);
typedef xrltBool (*xrltTransformCallback)   (xrltContextPtr ctx,
                                             xrltTransformValue *data);


XRLTPUBFUN xrltBool XRLTCALL
        xrltElementRegister       (const xmlChar *ns, const xmlChar *name,
                                   int flags, xrltCompileFunction compile,
                                   xrltFreeFunction free,
                                   xrltTransformFunction transform);
XRLTPUBFUN xrltBool XRLTCALL
        xrltElementCompile        (xrltRequestsheetPtr sheet, xmlNodePtr first);
XRLTPUBFUN xrltBool XRLTCALL
        xrltElementTransform      (xrltRequestsheetPtr sheet, xmlNodePtr first);


XRLTPUBFUN void XRLTCALL
        xrltCleanup               (void);


XRLTPUBFUN xrltRequestsheetPtr XRLTCALL
        xrltRequestsheetCreate    (xmlDocPtr doc);
XRLTPUBFUN void XRLTCALL
        xrltRequestsheetFree      (xrltRequestsheetPtr sheet);


XRLTPUBFUN xrltContextPtr XRLTCALL
        xrltContextCreate         (xrltRequestsheetPtr sheet,
                                   xrltHeaderList *header);
XRLTPUBFUN void XRLTCALL
        xrltContextFree           (xrltContextPtr ctx);
XRLTPUBFUN int XRLTCALL
        xrltTransform             (xrltContextPtr ctx,
                                   xrltTransformValue *data);


XRLTPUBFUN xrltBool XRLTCALL
        xrltOutputNodeInsert      (xrltContextPtr ctx, xmlNodePtr node,
                                   xrltNodeReadyCallback ready,
                                   xrltFreeFunction free, void *data);
XRLTPUBFUN xrltBool XRLTCALL
        xrltOutputNodeSubscribe   (xrltContextPtr ctx, xmlNodePtr node,
                                   xrltNodeReadyCallback ready,
                                   xrltFreeFunction free, void *data);
XRLTPUBFUN xrltBool XRLTCALL
        xrltOutputNodeReady       (xrltContextPtr ctx, xmlNodePtr node);


XRLTPUBFUN xrltBool XRLTCALL
        xrltTransformSubscribe    (xrltContextPtr ctx,
                                   xrltTransformValueType type, size_t id,
                                   xrltTransformCallback callback);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_H__ */
