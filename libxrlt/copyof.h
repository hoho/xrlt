/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_COPYOF_H__
#define __XRLT_COPYOF_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xmlNodePtr          node;
    xrltCompiledValue   select;
} xrltCopyOfData;


typedef struct {
    DEFAULT_TRANSFORMING_PARAMS;
} xrltCopyOfTransformingData;


void *
        xrltCopyOfCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                               void *prevcomp);
void
        xrltCopyOfFree        (void *comp);
xrltBool
        xrltCopyOfTransform   (xrltContextPtr ctx, void *comp,
                               xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_COPYOF_H__ */
