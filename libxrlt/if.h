/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_IF_H__
#define __XRLT_IF_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xmlNodePtr          node;
    xrltCompiledValue   test;
    xmlNodePtr          children;
} xrltIfData;


typedef struct {
    DEFAULT_TRANSFORMING_PARAMS;

    xmlNodePtr   testNode;
    xrltBool     test;
} xrltIfTransformingData;


void *
        xrltIfCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                           void *prevcomp);
void
        xrltIfFree        (void *comp);
xrltBool
        xrltIfTransform   (xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                           void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_IF_H__ */
