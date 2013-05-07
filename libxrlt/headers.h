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

    xrltBool        test;
    xmlNodePtr      ntest;
    xrltXPathExpr   xtest;

    xmlChar        *name;
    xmlNodePtr      nname;
    xrltXPathExpr   xname;

    xmlChar        *val;
    xmlNodePtr      nval;
    xrltXPathExpr   xval;

} xrltResponseHeaderData;


typedef enum {
    XRLT_RESPONSE_HEADER_TRANSFORM_TEST = 0,
    XRLT_RESPONSE_HEADER_TRANSFORM_NAMEVALUE
} xrltResponseHeaderTransformStage;


typedef struct {
    xmlNodePtr                         node;
    xmlNodePtr                         dataNode;

    xrltResponseHeaderTransformStage   stage;

    xrltBool                           test;
    xmlChar                           *name;
    xmlChar                           *val;
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
