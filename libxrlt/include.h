/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_INCLUDE_H__
#define __XRLT_INCLUDE_H__


#include <libxml/tree.h>
#include <xrlt.h>
#include "json2xml.h"
#include "querystring.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltCompiledIncludeParam xrltCompiledIncludeParam;
typedef xrltCompiledIncludeParam* xrltCompiledIncludeParamPtr;
struct _xrltCompiledIncludeParam {
    xrltCompiledValue   test;
    xrltCompiledValue   body;
    xrltCompiledValue   name;
    xrltCompiledValue   val;

    xrltCompiledIncludeParamPtr   next;
};


typedef struct {
    xmlNodePtr                    node;

    xrltCompiledValue             href;
    xrltCompiledValue             method;
    xrltCompiledValue             type;

    xrltCompiledIncludeParamPtr   fheader;
    xrltCompiledIncludeParamPtr   lheader;
    size_t                        headerCount;

    xrltCompiledIncludeParamPtr   fparam;
    xrltCompiledIncludeParamPtr   lparam;
    size_t                        paramCount;

    xrltCompiledValue bodyTest;
    xrltCompiledValue body;

    xrltCompiledValue success;
    xrltCompiledValue failure;
} xrltCompiledIncludeData;


typedef struct {
    xrltHeaderOutType headerType;
    xrltBool         body;
    xmlChar         *cbody;
    xrltBool         test;
    xmlChar         *name;
    xmlChar         *val;
} xrltTransformingParam;


typedef enum {
    XRLT_INCLUDE_TRANSFORM_PARAMS_BEGIN = 0,
    XRLT_INCLUDE_TRANSFORM_PARAMS_END,
    XRLT_INCLUDE_TRANSFORM_READ_RESPONSE,
    XRLT_INCLUDE_TRANSFORM_SUCCESS,
    XRLT_INCLUDE_TRANSFORM_FAILURE,
    XRLT_INCLUDE_TRANSFORM_END
} xrltIncludeTransformStage;


typedef enum {
    XRLT_PROCESS_INPUT_ERROR = 0,
    XRLT_PROCESS_INPUT_AGAIN,
    XRLT_PROCESS_INPUT_REFUSE,
    XRLT_PROCESS_INPUT_DONE
} xrltProcessInputResult;


typedef struct {
    xmlNodePtr                  srcNode;    // Include node in source document.

    xmlNodePtr                  node;       // Include node in response
                                            // document.
    xmlNodePtr                  pnode;      // Parent node for params.
    xmlNodePtr                  rnode;      // Parent node for the result.
    xmlNodePtr                  hnode;      // Parent node for headers.
    xmlNodePtr                  cnode;      // Parent node for cookies.

    size_t                      status;

    xmlParserCtxtPtr            xmlparser;  // Push parser for XML includes.
    xrltJSON2XMLPtr             jsonparser;
    xrltQueryStringParserPtr    qsparser;
    xmlDocPtr                   doc;        // Document to parse include result
                                            // to.
    xmlNodePtr                  insert;
    void                       *comp;

    xrltIncludeTransformStage   stage;

    xmlChar                    *href;

    xrltHTTPMethod              method;
    xmlChar                    *cmethod;

    xrltSubrequestDataType      type;
    xmlChar                    *ctype;

    xrltTransformingParam      *header;
    size_t                      headerCount;

    xrltTransformingParam      *param;
    size_t                      paramCount;

    xrltBool                    bodyTest;
    xmlChar                    *body;
} xrltIncludeTransformingData;


void *
        xrltIncludeCompile            (xrltRequestsheetPtr sheet,
                                       xmlNodePtr node,
                                       void *prevcomp);
void
        xrltIncludeFree               (void *comp);
xrltBool
        xrltIncludeTransform          (xrltContextPtr ctx, void *comp,
                                       xmlNodePtr insert, void *data);
xrltProcessInputResult
        xrltProcessInput              (xrltContextPtr ctx,
                                       xrltTransformValue *val,
                                       xrltIncludeTransformingData *data);
void
        xrltIncludeTransformingFree   (void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_INCLUDE_H__ */
