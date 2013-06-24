/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_XPATHFUNCS_H__
#define __XRLT_XPATHFUNCS_H__


#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


xrltBool
        xrltRegisterFunctions   (xmlXPathContextPtr ctxt);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_XPATHFUNCS_H__ */
