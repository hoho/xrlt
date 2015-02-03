/*
 * Summary: macros for marking symbols as exportable/importable.
 * Description: macros for marking symbols as exportable/importable.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Igor Zlatkovic <igor@zlatkovic.com>
 */

#ifndef __IOJSWRAP_EXPORTS_H__
#define __IOJSWRAP_EXPORTS_H__

/**
 * IOJSWRAPPUBFUN:
 * IOJSWRAPPUBFUN, IOJSWRAPPUBVAR, IOJSWRAPCALL
 *
 * Macros which declare an exportable function, an exportable variable and
 * the calling convention used for functions.
 *
 * Please use an extra block for every platform/compiler combination when
 * modifying this, rather than overlong #ifdef lines. This helps
 * readability as well as the fact that different compilers on the same
 * platform might need different definitions.
 */

/**
 * IOJSWRAPPUBFUN:
 *
 * Macros which declare an exportable function
 */
#define IOJSWRAPPUBFUN
/**
 * IOJSWRAPPUBVAR:
 *
 * Macros which declare an exportable variable
 */
#define IOJSWRAPPUBVAR extern
/**
 * IOJSWRAPCALL:
 *
 * Macros which declare the called convention for exported functions
 */
#define IOJSWRAPCALL

/** DOC_DISABLE */

/* Windows platform with MS compiler */
#if defined(_WIN32) && defined(_MSC_VER)
  #undef IOJSWRAPPUBFUN
  #undef IOJSWRAPPUBVAR
  #undef IOJSWRAPCALL
  #if defined(IN_LIBIOJSWRAP) && !defined(LIBIOJSWRAP_STATIC)
    #define IOJSWRAPPUBFUN __declspec(dllexport)
    #define IOJSWRAPPUBVAR __declspec(dllexport)
  #else
    #define IOJSWRAPPUBFUN
    #if !defined(LIBIOJSWRAP_STATIC)
      #define IOJSWRAPPUBVAR __declspec(dllimport) extern
    #else
      #define IOJSWRAPPUBVAR extern
    #endif
  #endif
  #define IOJSWRAPCALL __cdecl
  #if !defined _REENTRANT
    #define _REENTRANT
  #endif
#endif

/* Windows platform with Borland compiler */
#if defined(_WIN32) && defined(__BORLANDC__)
  #undef IOJSWRAPPUBFUN
  #undef IOJSWRAPPUBVAR
  #undef IOJSWRAPCALL
  #if defined(IN_LIBIOJSWRAP) && !defined(LIBIOJSWRAP_STATIC)
    #define IOJSWRAPPUBFUN __declspec(dllexport)
    #define IOJSWRAPPUBVAR __declspec(dllexport) extern
  #else
    #define IOJSWRAPPUBFUN
    #if !defined(LIBIOJSWRAP_STATIC)
      #define IOJSWRAPPUBVAR __declspec(dllimport) extern
    #else
      #define IOJSWRAPPUBVAR extern
    #endif
  #endif
  #define IOJSWRAPCALL __cdecl
  #if !defined _REENTRANT
    #define _REENTRANT
  #endif
#endif

/* Windows platform with GNU compiler (Mingw) */
#if defined(_WIN32) && defined(__MINGW32__)
  #undef IOJSWRAPPUBFUN
  #undef IOJSWRAPPUBVAR
  #undef IOJSWRAPCALL
/*
  #if defined(IN_LIBIOJSWRAP) && !defined(LIBIOJSWRAP_STATIC)
*/
  #if !defined(LIBIOJSWRAP_STATIC)
    #define IOJSWRAPPUBFUN __declspec(dllexport)
    #define IOJSWRAPPUBVAR __declspec(dllexport) extern
  #else
    #define IOJSWRAPPUBFUN
    #if !defined(LIBIOJSWRAP_STATIC)
      #define IOJSWRAPPUBVAR __declspec(dllimport) extern
    #else
      #define IOJSWRAPPUBVAR extern
    #endif
  #endif
  #define IOJSWRAPCALL __cdecl
  #if !defined _REENTRANT
    #define _REENTRANT
  #endif
#endif

/* Cygwin platform, GNU compiler */
#if defined(_WIN32) && defined(__CYGWIN__)
  #undef IOJSWRAPPUBFUN
  #undef IOJSWRAPPUBVAR
  #undef IOJSWRAPCALL
  #if defined(IN_LIBIOJSWRAP) && !defined(LIBIOJSWRAP_STATIC)
    #define IOJSWRAPPUBFUN __declspec(dllexport)
    #define IOJSWRAPPUBVAR __declspec(dllexport)
  #else
    #define IOJSWRAPPUBFUN
    #if !defined(LIBIOJSWRAP_STATIC)
      #define IOJSWRAPPUBVAR __declspec(dllimport) extern
    #else
      #define IOJSWRAPPUBVAR
    #endif
  #endif
  #define IOJSWRAPCALL __cdecl
#endif

/* Compatibility */
#if !defined(LIBIOJSWRAP_PUBLIC)
#define LIBIOJSWRAP_PUBLIC IOJSWRAPPUBVAR
#endif

#endif /* __IOJSWRAP_EXPORTS_H__ */
