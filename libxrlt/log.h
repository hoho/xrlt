/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_LOG_H__
#define __XRLT_LOG_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xrltLogType   type;
    xmlNodePtr    node;
} xrltLogData;


void *
        xrltLogCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                            void *prevcomp);
void
        xrltLogFree        (void *comp);
xrltBool
        xrltLogTransform   (xrltContextPtr ctx, void *comp,
                            xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_LOG_H__ */
