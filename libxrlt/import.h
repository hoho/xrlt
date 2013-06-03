/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_IMPORT_H__
#define __XRLT_IMPORT_H__


#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


#define XRLT_MAX_IMPORT_DEEPNESS 10


xrltBool
        xrltProcessImports   (xrltRequestsheetPtr sheet, xmlNodePtr node,
                              int level);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_IMPORT_H__ */
