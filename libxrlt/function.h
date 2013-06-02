/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_FUNCTION_H__
#define __XRLT_FUNCTION_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>
#include <xrlt.h>
#include "variable.h"
#ifndef __XRLT_NO_JAVASCRIPT__
    #include "js.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    XRLT_TRANSFORMATION_FUNCTION = 0,
    XRLT_TRANSFORMATION_XSLT_STRINGIFY,  // XSLT with string result.
    XRLT_TRANSFORMATION_XSLT,            // XSLT with XML result.
    XRLT_TRANSFORMATION_JSON_STRINGIFY,  // Serialize to JSON string.
    XRLT_TRANSFORMATION_JSON_PARSE,      // Parse JSON string.
    XRLT_TRANSFORMATION_XML_STRINGIFY,   // Serialize to XML string.
    XRLT_TRANSFORMATION_XML_PARSE,       // Parse XML string.
    XRLT_TRANSFORMATION_CUSTOM           // Custom transformation.
} xrltTransformationType;


typedef struct {
    xmlNodePtr               node;
    xmlChar                 *name;

    xrltBool                 js;
    xrltTransformationType   transformation;

    xrltVariableDataPtr     *param;
    size_t                   paramLen;
    size_t                   paramSize;

    xmlNodePtr               children;

    xsltStylesheetPtr        xslt;
} xrltFunctionData;


typedef struct {
    xmlNodePtr            node;
    xrltFunctionData     *func;

    xrltBool              transform;
    xrltBool              hasSyncParam;

    xrltVariableDataPtr  *param;
    size_t                paramLen;
    size_t                paramSize;

    xrltVariableDataPtr  *merged;
    size_t                mergedLen;

    xmlNodePtr            children;
    xrltCompiledValue     self;
} xrltApplyData;


//typedef enum {
//} xrltFunctionTransformStage;


typedef struct {
    xmlNodePtr   node;
    xmlNodePtr   paramNode;
    xmlNodePtr   retNode;
    xmlDocPtr    self;
    xrltBool     finalize;
} xrltApplyTransformingData;


void *
        xrltFunctionCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltFunctionFree        (void *comp);

void *
        xrltApplyCompile        (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltApplyFree           (void *comp);
xrltBool
        xrltApplyTransform      (xrltContextPtr ctx, void *comp,
                                 xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_FUNCTION_H__ */
