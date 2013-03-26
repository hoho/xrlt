/*
 * Summary: macros for marking symbols as exportable/importable.
 * Description: macros for marking symbols as exportable/importable.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Igor Zlatkovic <igor@zlatkovic.com>
 */

#ifndef __XRLT_EXPORTS_H__
#define __XRLT_EXPORTS_H__

/**
 * XRLTPUBFUN:
 * XRLTPUBFUN, XRLTPUBVAR, XRLTCALL
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
 * XRLTPUBFUN:
 *
 * Macros which declare an exportable function
 */
#define XRLTPUBFUN
/**
 * XRLTPUBVAR:
 *
 * Macros which declare an exportable variable
 */
#define XRLTPUBVAR extern
/**
 * XRLTCALL:
 *
 * Macros which declare the called convention for exported functions
 */
#define XRLTCALL

/** DOC_DISABLE */

/* Windows platform with MS compiler */
#if defined(_WIN32) && defined(_MSC_VER)
  #undef XRLTPUBFUN
  #undef XRLTPUBVAR
  #undef XRLTCALL
  #if defined(IN_LIBXRLT) && !defined(LIBXRLT_STATIC)
    #define XRLTPUBFUN __declspec(dllexport)
    #define XRLTPUBVAR __declspec(dllexport)
  #else
    #define XRLTPUBFUN
    #if !defined(LIBXRLT_STATIC)
      #define XRLTPUBVAR __declspec(dllimport) extern
    #else
      #define XRLTPUBVAR extern
    #endif
  #endif
  #define XRLTCALL __cdecl
  #if !defined _REENTRANT
    #define _REENTRANT
  #endif
#endif

/* Windows platform with Borland compiler */
#if defined(_WIN32) && defined(__BORLANDC__)
  #undef XRLTPUBFUN
  #undef XRLTPUBVAR
  #undef XRLTCALL
  #if defined(IN_LIBXRLT) && !defined(LIBXRLT_STATIC)
    #define XRLTPUBFUN __declspec(dllexport)
    #define XRLTPUBVAR __declspec(dllexport) extern
  #else
    #define XRLTPUBFUN
    #if !defined(LIBXRLT_STATIC)
      #define XRLTPUBVAR __declspec(dllimport) extern
    #else
      #define XRLTPUBVAR extern
    #endif
  #endif
  #define XRLTCALL __cdecl
  #if !defined _REENTRANT
    #define _REENTRANT
  #endif
#endif

/* Windows platform with GNU compiler (Mingw) */
#if defined(_WIN32) && defined(__MINGW32__)
  #undef XRLTPUBFUN
  #undef XRLTPUBVAR
  #undef XRLTCALL
/*
  #if defined(IN_LIBXRLT) && !defined(LIBXRLT_STATIC)
*/
  #if !defined(LIBXRLT_STATIC)
    #define XRLTPUBFUN __declspec(dllexport)
    #define XRLTPUBVAR __declspec(dllexport) extern
  #else
    #define XRLTPUBFUN
    #if !defined(LIBXRLT_STATIC)
      #define XRLTPUBVAR __declspec(dllimport) extern
    #else
      #define XRLTPUBVAR extern
    #endif
  #endif
  #define XRLTCALL __cdecl
  #if !defined _REENTRANT
    #define _REENTRANT
  #endif
#endif

/* Cygwin platform, GNU compiler */
#if defined(_WIN32) && defined(__CYGWIN__)
  #undef XRLTPUBFUN
  #undef XRLTPUBVAR
  #undef XRLTCALL
  #if defined(IN_LIBXRLT) && !defined(LIBXRLT_STATIC)
    #define XRLTPUBFUN __declspec(dllexport)
    #define XRLTPUBVAR __declspec(dllexport)
  #else
    #define XRLTPUBFUN
    #if !defined(LIBXRLT_STATIC)
      #define XRLTPUBVAR __declspec(dllimport) extern
    #else
      #define XRLTPUBVAR
    #endif
  #endif
  #define XRLTCALL __cdecl
#endif

/* Compatibility */
#if !defined(LIBXRLT_PUBLIC)
#define LIBXRLT_PUBLIC XRLTPUBVAR
#endif

#endif /* __XRLT_EXPORTS_H__ */
