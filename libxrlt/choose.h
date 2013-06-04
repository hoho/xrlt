/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_CHOOSE_H__
#define __XRLT_CHOOSE_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xrltCompiledValue   test;
    xmlNodePtr          children;
} xrltChooseCaseData;


typedef struct {
    xmlNodePtr           node;
    xrltChooseCaseData  *cases;
    int                  len;
} xrltChooseData;


typedef struct {
    DEFAULT_TRANSFORMING_PARAMS;

    xmlNodePtr   testNode;
    int          pos;
    xrltBool     test;
} xrltChooseTransformingData;


void *
        xrltChooseCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                               void *prevcomp);
void
        xrltChooseFree        (void *comp);
xrltBool
        xrltChooseTransform   (xrltContextPtr ctx, void *comp,
                               xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_CHOOSE_H__ */
