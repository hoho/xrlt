/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_VALUEOF_H__
#define __XRLT_VALUEOF_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xmlNodePtr          node;
    xrltCompiledValue   select;
} xrltValueOfData;


typedef struct {
    xmlNodePtr   node;
    xmlNodePtr   dataNode;
    xmlChar     *val;
} xrltValueOfTransformingData;


void *
        xrltValueOfCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                void *prevcomp);
void
        xrltValueOfFree        (void *comp);
xrltBool
        xrltValueOfTransform   (xrltContextPtr ctx, void *comp,
                                xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_VALUEOF_H__ */
