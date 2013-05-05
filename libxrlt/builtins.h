#ifndef __XRLT_BUILTINS_H__
#define __XRLT_BUILTINS_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xrltLogType   type;
    xmlNodePtr    node;
} xrltLogData;


typedef struct {
    xmlXPathCompExprPtr   test;
    xmlNodePtr            children;
} xrltIfData;


void *
        xrltResponseCompile        (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                    void *prevcomp);
void
        xrltResponseFree           (void *comp);
xrltBool
        xrltResponseTransform      (xrltContextPtr ctx, void *comp,
                                    xmlNodePtr insert, void *data);


void *
        xrltLogCompile             (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                    void *prevcomp);
void
        xrltLogFree                (void *comp);
xrltBool
        xrltLogTransform           (xrltContextPtr ctx, void *comp,
                                    xmlNodePtr insert, void *data);

void *
        xrltIfCompile              (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                    void *prevcomp);
void
        xrltIfFree                 (void *comp);
xrltBool
        xrltIfTransform            (xrltContextPtr ctx, void *comp,
                                    xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_BUILTINS_H__ */
