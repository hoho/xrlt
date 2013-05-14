/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_ERROR_H__
#define	__XRLT_ERROR_H__


#include <libxml/xmlerror.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * XRLT specific error reporting functions.
 */
XRLTPUBVAR xmlGenericErrorFunc      xrltGenericError;
XRLTPUBVAR void                    *xrltGenericErrorContext;

XRLTPUBFUN void XRLTCALL
        xrltPrintErrorContext       (xrltContextPtr ctxt,
                                     xrltRequestsheetPtr sheet,
                                     xmlNodePtr node);
XRLTPUBFUN void XRLTCALL
        xrltSetGenericErrorFunc     (void *ctx, xmlGenericErrorFunc handler);
XRLTPUBFUN void XRLTCALL
        xrltTransformError          (xrltContextPtr ctxt,
                                     xrltRequestsheetPtr sheet,
                                     xmlNodePtr node, const char *msg, ...);


#ifdef	__cplusplus
}
#endif

#endif	/* __XRLT_ERROR_H__ */
