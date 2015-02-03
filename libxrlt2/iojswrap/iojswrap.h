/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __IOJSWRAP_H__
#define __IOJSWRAP_H__


#include <iojswrapexports.h>


#ifdef __cplusplus
extern "C" {
#endif


IOJSWRAPPUBFUN int IOJSWRAPCALL   iojsInit(void);
IOJSWRAPPUBFUN int IOJSWRAPCALL   iojsRun(void);
IOJSWRAPPUBFUN int IOJSWRAPCALL   iojsFree(void);


#ifdef __cplusplus
}
#endif

#endif /* __IOJSWRAP_H__ */
