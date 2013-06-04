/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_RESPONSE_H__
#define __XRLT_RESPONSE_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


void *
        xrltResponseCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltResponseFree        (void *comp);
xrltBool
        xrltResponseTransform   (xrltContextPtr ctx, void *comp,
                                 xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_RESPONSE_H__ */
