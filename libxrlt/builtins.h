#ifndef __XRLT_BUILTINS_H__
#define __XRLT_BUILTINS_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


#define XRLT_ELEMENT_ATTR_TEST (const xmlChar *)"test"


typedef struct {
    xmlXPathCompExprPtr   test;
    xmlNodePtr            children;
} xrltIfData;


void *
        xrltResponseCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltResponseFree        (void *data);
xrltBool
        xrltResponseTransform   (xrltContextPtr ctx, void *data);

void *
        xrltIfCompile           (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltIfFree              (void *data);
xrltBool
        xrltIfTransform         (xrltContextPtr ctx, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_BUILTINS_H__ */
