/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_HEADERS_H__
#define __XRLT_HEADERS_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    xmlNodePtr          node;
    xrltHeaderOutType   type;
    xrltCompiledValue   test;
    xrltCompiledValue   name;
    xrltCompiledValue   val;

    xrltCompiledValue   path;      // For cookies.
    xrltCompiledValue   domain;    // For cookies.
    xrltCompiledValue   expires;   // For cookies.
    xrltCompiledValue   secure;    // For cookies.
    xrltCompiledValue   httponly;  // For cookies.

    xrltCompiledValue   permanent; // For redirects.
} xrltResponseHeaderData;


typedef struct {
    xmlNodePtr          node;
    xrltHeaderOutType   type;
    xrltCompiledValue   name;
    xrltCompiledValue   val;
    xrltBool            main;

} xrltHeaderElementData;


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
    xmlChar                           *path;
    xmlChar                           *domain;
    xmlChar                           *expires;
    xrltBool                           secure;
    xrltBool                           httponly;
    xrltBool                           permanent;
} xrltResponseHeaderTransformingData;


typedef enum {
    XRLT_HEADER_ELEMENT_TRANSFORM_NAME = 0,
    XRLT_HEADER_ELEMENT_TRANSFORM_VALUE
} xrltHeaderElementTransformStage;


typedef struct {
    xmlNodePtr                        node;
    xmlNodePtr                        dataNode;
    xrltHeaderElementData            *comp;

    xrltHeaderElementTransformStage   stage;

    xmlChar                          *name;
    xmlChar                          *val;
} xrltHeaderElementTransformingData;


void *
        xrltResponseHeaderCompile     (xrltRequestsheetPtr sheet,
                                       xmlNodePtr node, void *prevcomp);
void
        xrltResponseHeaderFree        (void *comp);
xrltBool
        xrltResponseHeaderTransform   (xrltContextPtr ctx, void *comp,
                                       xmlNodePtr insert, void *data);


void *
        xrltHeaderElementCompile      (xrltRequestsheetPtr sheet,
                                       xmlNodePtr node, void *prevcomp);
void
        xrltHeaderElementFree         (void *comp);
xrltBool
        xrltHeaderElementTransform    (xrltContextPtr ctx, void *comp,
                                       xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_HEADERS_H__ */
