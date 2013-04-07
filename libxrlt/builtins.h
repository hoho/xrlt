#ifndef __XRLT_BUILTINS_H__
#define __XRLT_BUILTINS_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


#define XRLT_ELEMENT_ATTR_TEST    (const xmlChar *)"test"
#define XRLT_ELEMENT_ATTR_NAME    (const xmlChar *)"name"
#define XRLT_ELEMENT_PARAM        (const xmlChar *)"param"
#define XRLT_ELEMENT_WITH_PARAM   (const xmlChar *)"with-param"


typedef struct {
    xmlChar      *name;
    xrltBool      js;
    xmlNodePtr    children;
} xrltFunctionData;


typedef struct {
    xrltFunctionData  *func;
} xrltApplyData;


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
        xrltFunctionCompile        (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                    void *prevcomp);
void
        xrltFunctionFree           (void *comp);
xrltBool
        xrltFunctionTransform      (xrltContextPtr ctx, void *comp,
                                    xmlNodePtr insert, void *data);


void *
        xrltApplyCompile           (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                    void *prevcomp);
void
        xrltApplyFree              (void *comp);
xrltBool
        xrltApplyTransform         (xrltContextPtr ctx, void *comp,
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
