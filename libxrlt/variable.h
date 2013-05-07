#ifndef __XRLT_VARIABLE_H__
#define __XRLT_VARIABLE_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltVariableData xrltVariableData;
typedef xrltVariableData* xrltVariableDataPtr;
struct _xrltVariableData {
    xmlNodePtr      node;

    xrltBool        ownName;    // For parameters only.
    xmlChar        *name;

    xrltBool        ownJsname;  // For parameters only.
    xmlChar        *jsname;

    xrltBool        ownNval;    // For parameters only.
    xmlNodePtr      nval;

    xrltBool        ownXval;    // For parameters only.
    xrltXPathExpr   xval;
};


typedef struct {
    xmlNodePtr                  node;
} xrltVariableTransformingData;


void *
        xrltVariableCompile     (xrltRequestsheetPtr sheet, xmlNodePtr node,
                                 void *prevcomp);
void
        xrltVariableFree        (void *comp);
xrltBool
        xrltVariableTransform   (xrltContextPtr ctx, void *comp,
                                 xmlNodePtr insert, void *data);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_VARIABLE_H__ */
