#ifndef __XRLT_TRANSFORM_H__
#define __XRLT_TRANSFORM_H__


#include <libxml/tree.h>
#include <libxml/hash.h>

#include <xrltstruct.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltElement xrltElement;
typedef xrltElement* xrltElementPtr;
struct _xrltElement {
    xrltCompileFunction     compile;
    xrltFreeFunction        free;
    xrltTransformFunction   transform;
};


typedef struct _xrltCompiledElement xrltCompiledElement;
typedef xrltCompiledElement* xrltCompiledElementPtr;
struct _xrltCompiledElement {
    xrltTransformFunction   transform;
    xrltFreeFunction        free;
    void                   *data;
};


typedef struct {
    xrltCompiledElementPtr   compiled;
    size_t                   compiledLen;
    size_t                   compiledSize;
    xmlNodePtr               response;
} xrltRequestsheetPrivate;


void
        xrltUnregisterBuiltinElements   (void);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_TRANSFORM_H__ */
