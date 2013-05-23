/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_H__
#define __XRLT_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrltstruct.h>
#include <xrltexports.h>


#ifdef __cplusplus
extern "C" {
#endif


#define XRLT_NS          (const xmlChar *)"http://xrlt.net/Transform"
#define XRLT_ROOT_NAME   (const xmlChar *)"requestsheet"


#define XRLT_STATUS_UNKNOWN             0
#define XRLT_STATUS_ERROR               2
#define XRLT_STATUS_WAITING             4
#define XRLT_STATUS_DONE                8
#define XRLT_STATUS_STATUS              16
#define XRLT_STATUS_HEADER              32
#define XRLT_STATUS_SUBREQUEST          64
#define XRLT_STATUS_CHUNK               128
#define XRLT_STATUS_LOG                 256
#define XRLT_STATUS_REFUSE_SUBREQUEST   512


#define XRLT_REGISTER_TOPLEVEL   2
#define XRLT_COMPILE_PASS1       4
#define XRLT_COMPILE_PASS2       8


typedef enum {
    XRLT_PROCESS_HEADER = 1,
    XRLT_PROCESS_BODY = 2
} xrltTransformValueType;


typedef struct {
    xrltTransformValueType   type;
    size_t                   status;
    xrltString               name;
    xrltString               val;
    xrltBool                 last;
    xrltBool                 error;
} xrltTransformValue;


typedef enum {
    XRLT_PASS1 = 0,
    XRLT_PASS2,
    XRLT_COMPILED
} xrltCompilePass;


typedef struct _xrltTransformCallback   xrltTransformCallback;
typedef xrltTransformCallback*          xrltTransformCallbackPtr;

typedef struct _xrltInputCallback       xrltInputCallback;
typedef xrltInputCallback*              xrltInputCallbackPtr;

typedef struct _xrltRequestsheet        xrltRequestsheet;
typedef xrltRequestsheet*               xrltRequestsheetPtr;

typedef struct _xrltContext             xrltContext;
typedef xrltContext*                    xrltContextPtr;


typedef struct {
    xrltTransformCallbackPtr   first;
    xrltTransformCallbackPtr   last;
} xrltTransformCallbackQueue;


typedef struct {
    xrltInputCallbackPtr   first;
    xrltInputCallbackPtr   last;
} xrltInputCallbackQueue;


typedef struct {
    xrltInputCallbackQueue  *header;
    size_t                   headerSize;

    xrltInputCallbackQueue  *body;
    size_t                   bodySize;
} xrltInputCallbackQueues;


typedef void *   (*xrltCompileFunction)     (xrltRequestsheetPtr sheet,
                                             xmlNodePtr node, void *prevcomp);
typedef void     (*xrltFreeFunction)        (void *comp);
typedef xrltBool (*xrltTransformFunction)   (xrltContextPtr ctx, void *comp,
                                             xmlNodePtr insert, void *data);

typedef xrltBool (*xrltInputFunction)       (xrltContextPtr ctx, size_t id,
                                             xrltTransformValue *val,
                                             void *data);


struct _xrltRequestsheet {
    xmlDocPtr         doc;

    xrltCompilePass   pass;        // Indicates current compilation
                                   // pass.
    xmlNodePtr        response;    // Node to begin transformation
                                   // from.
    xmlHashTablePtr   funcs;       // Functions of this requestsheet.
    xmlHashTablePtr   transforms;  // Transformations of this requestsheet.

    void             *js;          // JavaScript context.
};


struct _xrltContext {
    xrltRequestsheetPtr          sheet;

    xrltBool                     error;
    int                          cur;          // Current combination of
                                               // XRLT_STATUS_*
    xrltHeaderList               header;       // Response headers.
    int                          headerCount;  // Wait for headers before
                                               // sending response chunks.
    xrltSubrequestList           sr;           // Subrequests to make.
    xrltChunkList                chunk;        // Response chunk.
    xrltLogList                  log;

    size_t                       includeId;
    size_t                       maxVarScope;

    xmlNodePtr                   sheetNode;
    xmlDocPtr                    responseDoc;
    xmlNodePtr                   response;
    xmlNodePtr                   responseCur;
    xmlNodePtr                   requestHeaders;
    xmlNodePtr                   var;
    xmlNodePtr                   varContext;
    size_t                       varScope;
    xmlDocPtr                    xpathDefault;
    xmlXPathContextPtr           xpath;
    xmlNodePtr                   xpathWait;

    xrltTransformCallbackQueue   tcb;
    xrltInputCallbackQueues      icb;
};


struct _xrltTransformCallback {
    xrltTransformFunction      func;    // Function to call.
    void                      *comp;    // Compiled element's data.
    xmlNodePtr                 insert;  // Place to insert the result.
    void                      *data;    // Data allocated by transform
                                        // function. These datas are stored in
                                        // the transformation context. They are
                                        // freed by free function of
                                        // xrltTransformingElement, when the
                                        // context is being freed.
    size_t                     varScope;
    xrltTransformCallbackPtr   next;    // Next callback in this queue.
};


struct _xrltInputCallback {
    xrltInputFunction      func;
    size_t                 varScope;
    void                  *data;
    xrltInputCallbackPtr   next;
};


XRLTPUBFUN xrltBool XRLTCALL
        xrltElementRegister       (const xmlChar *ns, const xmlChar *name,
                                   int flags, xrltCompileFunction compile,
                                   xrltFreeFunction free,
                                   xrltTransformFunction transform);
XRLTPUBFUN xrltBool XRLTCALL
        xrltElementCompile        (xrltRequestsheetPtr sheet, xmlNodePtr first);
XRLTPUBFUN xrltBool XRLTCALL
        xrltElementTransform      (xrltContextPtr ctx, xmlNodePtr first,
                                   xmlNodePtr insert);


XRLTPUBFUN void XRLTCALL
        xrltInit                  (void);
XRLTPUBFUN void XRLTCALL
        xrltCleanup               (void);


XRLTPUBFUN xrltRequestsheetPtr XRLTCALL
        xrltRequestsheetCreate    (xmlDocPtr doc);
XRLTPUBFUN void XRLTCALL
        xrltRequestsheetFree      (xrltRequestsheetPtr sheet);


XRLTPUBFUN xrltContextPtr XRLTCALL
        xrltContextCreate         (xrltRequestsheetPtr sheet);
XRLTPUBFUN void XRLTCALL
        xrltContextFree           (xrltContextPtr ctx);
XRLTPUBFUN int XRLTCALL
        xrltTransform             (xrltContextPtr ctx, size_t id,
                                   xrltTransformValue *val);

XRLTPUBFUN xrltBool XRLTCALL
        xrltXPathEval             (xrltContextPtr ctx, xmlNodePtr insert,
                                   xrltXPathExpr *expr, xmlXPathObjectPtr *ret);

XRLTPUBFUN xrltBool XRLTCALL
        xrltInputSubscribe        (xrltContextPtr ctx,
                                   xrltTransformValueType type, size_t id,
                                   xrltInputFunction callback, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_H__ */
