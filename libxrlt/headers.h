#ifndef __XRLT_HEADERS_H__
#define __XRLT_HEADERS_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xmlNodePtr   node;

    xrltBool              test;
    xmlNodePtr            ntest;
    xmlXPathCompExprPtr   xtest;

    xmlChar              *name;
    xmlNodePtr            nname;
    xmlXPathCompExprPtr   xname;

    xmlChar              *val;
    xmlNodePtr            nval;
    xmlXPathCompExprPtr   xval;

} xrltResponseHeaderData;


typedef struct {
    xmlNodePtr   node;

    xrltBool              test;
    xmlChar              *name;
    xmlChar              *val;
} xrltResponseHeaderTransformingData;


void *
        xrltResponseHeaderCompile     (xrltRequestsheetPtr sheet,
                                       xmlNodePtr node, void *prevcomp);
void
        xrltResponseHeaderFree        (void *comp);
xrltBool
        xrltResponseHeaderTransform   (xrltContextPtr ctx, void *comp,
                                       xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_HEADERS_H__ */
