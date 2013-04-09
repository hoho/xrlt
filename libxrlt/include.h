#ifndef __XRLT_INCLUDE_H__
#define __XRLT_INCLUDE_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltCompiledIncludeParam xrltCompiledIncludeParam;
typedef xrltCompiledIncludeParam* xrltCompiledIncludeParamPtr;
struct _xrltCompiledIncludeParam {
    xmlChar                      *test;
    xmlNodePtr                    ntest;
    xmlXPathCompExprPtr           xtest;

    xrltBool                      body;
    xmlNodePtr                    nbody;
    xmlXPathCompExprPtr           xbody;

    xmlChar                      *name;
    xmlNodePtr                    nname;
    xmlXPathCompExprPtr           xname;

    xmlChar                      *value;
    xmlNodePtr                    nvalue;
    xmlXPathCompExprPtr           xvalue;

    xrltCompiledIncludeParamPtr   next;
};


typedef struct {
    xmlNodePtr                    node;

    xmlChar                      *href;
    xmlNodePtr                    nhref;
    xmlXPathCompExprPtr           xhref;

    xrltCompiledIncludeParamPtr   fheader;
    xrltCompiledIncludeParamPtr   lheader;

    xrltCompiledIncludeParamPtr   fparam;
    xrltCompiledIncludeParamPtr   lparam;

    xmlChar                      *body;
    xmlNodePtr                    nbody;
    xmlXPathCompExprPtr           xbody;

    xmlNodePtr                    nsuccess;
    xmlXPathCompExprPtr           xsuccess;

    xmlNodePtr                    nfailure;
    xmlXPathCompExprPtr           xfailure;
} xrltCompiledIncludeData;


void *
        xrltIncludeCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                void *prevcomp);
void
        xrltIncludeFree        (void *comp);
xrltBool
        xrltIncludeTransform   (xrltContextPtr ctx, void *comp,
                                xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_INCLUDE_H__ */
