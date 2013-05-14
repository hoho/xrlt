/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_FUNCTION_H__
#define __XRLT_FUNCTION_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>
#include "variable.h"
#include "js.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xmlNodePtr            node;

    xmlChar              *name;

    xrltBool              js;

    xrltVariableDataPtr  *param;
    size_t                paramLen;
    size_t                paramSize;

    xmlNodePtr            children;
} xrltFunctionData;


typedef struct {
    xmlNodePtr            node;
    xrltFunctionData     *func;

    xrltBool              hasSyncParam;

    xrltVariableDataPtr  *param;
    xrltVariableDataPtr  *merged;
    size_t                paramLen;
    size_t                paramSize;
} xrltApplyData;


//typedef enum {
//} xrltFunctionTransformStage;


typedef struct {
    xmlNodePtr   node;
    xmlNodePtr   paramNode;
    xmlNodePtr   retNode;
    xrltBool     finalize;
} xrltApplyTransformingData;


void *
        xrltFunctionCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltFunctionFree        (void *comp);
xrltBool
        xrltFunctionTransform   (xrltContextPtr ctx, void *comp,
                                 xmlNodePtr insert, void *data);

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
