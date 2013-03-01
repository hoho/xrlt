#ifndef __XRLT_H__
#define __XRLT_H__


#include <libxml/tree.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltRequestsheet xrltRequestsheet;
typedef xrltRequestsheet* xrltRequestsheetPtr;
struct _xrltRequestsheet {
    xmlDocPtr doc;
};


#define XRLT_NS (const xmlChar *)"http://xrlt.net/Transform"


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_H__ */
