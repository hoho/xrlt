/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_FOREACH_H__
#define __XRLT_FOREACH_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xmlNodePtr          node;
    xrltCompiledValue   select;
    xmlNodePtr          children;
} xrltForEachData;


typedef struct {
    DEFAULT_TRANSFORMING_PARAMS;

    xrltBool        xpathEvaluation;
    xmlNodeSetPtr   val;
} xrltForEachTransformingData;


void *
        xrltForEachCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                void *prevcomp);
void
        xrltForEachFree        (void *comp);
xrltBool
        xrltForEachTransform   (xrltContextPtr ctx, void *comp,
                                xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_FOREACH_H__ */
