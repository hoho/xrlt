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
    xrltValue                     test;
    xrltValue                     body;
    xrltValue                     name;
    xrltValue                     val;

    xrltCompiledIncludeParamPtr   next;
};


typedef struct {
    xmlNodePtr                    node;

    xrltValue                     href;
    xrltValue                     method;
    xrltValue                     type;

    xrltCompiledIncludeParamPtr   fheader;
    xrltCompiledIncludeParamPtr   lheader;
    size_t                        headerCount;

    xrltCompiledIncludeParamPtr   fparam;
    xrltCompiledIncludeParamPtr   lparam;
    size_t                        paramCount;

    xrltValue                     bodyTest;
    xrltValue                     body;

    xrltValue                     success;
    xrltValue                     failure;
} xrltCompiledIncludeData;


typedef struct {
    xrltBool   cookie;
    xrltBool   body;
    xmlChar   *cbody;
    xrltBool   test;
    xmlChar   *name;
    xmlChar   *val;
} xrltTransformingParam;


typedef enum {
    XRLT_INCLUDE_TRANSFORM_PARAMS_BEGIN = 0,
    XRLT_INCLUDE_TRANSFORM_PARAMS_END,
    XRLT_INCLUDE_TRANSFORM_READ_RESPONSE,
    XRLT_INCLUDE_TRANSFORM_SUCCESS,
    XRLT_INCLUDE_TRANSFORM_FAILURE,
    XRLT_INCLUDE_TRANSFORM_END
} xrltIncludeTransformStage;


typedef struct {
    xmlNodePtr                  srcNode;    // Include node in source document.

    xmlNodePtr                  node;       // Include node in response
                                            // document.
    xmlNodePtr                  pnode;      // Parent node to transform params
                                            // to.
    xmlNodePtr                  rnode;      // Parent node to transform the
                                            // result to.
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
        xrltIncludeCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                void *prevcomp);
void
        xrltIncludeFree        (void *comp);
xrltBool
        xrltIncludeTransform   (xrltContextPtr ctx, void *comp,
                                xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_INCLUDE_H__ */
