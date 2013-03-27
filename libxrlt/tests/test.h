#ifndef __XRLT_TEST_H__
#define __XRLT_TEST_H__


#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif


char   _msgbuf[256];


#define TEST_PASSED printf("."); return;

#define _ASSERT(cond, msg)                                                    \
    if (cond) {                                                               \
        memset(_msgbuf, 0, 256);                                              \
        msg;                                                                  \
        xrltTestFailurePush(_msgbuf);                                         \
        printf("F");                                                          \
        return;                                                               \
    }

#define ASSERT_INT(r, e)                                                      \
    _ASSERT(!((r) == (e)),                                                    \
            snprintf(_msgbuf, 255, "%s:%d, expected %d, got %d",              \
                     __FILE__, __LINE__, e, r));                              \

#define ASSERT_TRUE(r)                                                        \
    _ASSERT(!(r),                                                             \
            snprintf(_msgbuf, 255, "%s:%d, expected TRUE",                    \
                     __FILE__, __LINE__));                                    \

#define ASSERT_FALSE(r)                                                       \
    _ASSERT(r,                                                                \
            snprintf(_msgbuf, 255, "%s:%d, expected FALSE",                   \
                     __FILE__, __LINE__));                                    \

#define ASSERT_NULL(r)                                                        \
    _ASSERT((r) != NULL,                                                      \
            snprintf(_msgbuf, 255, "%s:%d, expected NULL",                    \
                     __FILE__, __LINE__));                                    \

#define ASSERT_NOT_NULL(r)                                                    \
    _ASSERT((r) == NULL,                                                      \
            snprintf(_msgbuf, 255, "%s:%d, expected NOT NULL",                \
                     __FILE__, __LINE__));                                    \

#define ASSERT_STR(r, e)                                                      \
    _ASSERT(strcmp(r.data, e) || r.len != strlen(e),                          \
            snprintf(_msgbuf, 255, "%s:%d, expected '%s', got '%s'",          \
                     __FILE__, __LINE__, e, r.data));                         \


typedef struct _xrltTestFailure xrltTestFailure;
typedef xrltTestFailure* xrltTestFailurePtr;
struct _xrltTestFailure {
    char                *msg;
    xrltTestFailurePtr   next;
};


xrltTestFailurePtr   xrltTestFailureFirst = NULL;
xrltTestFailurePtr   xrltTestFailureLast = NULL;


void
xrltTestFailurePush(char *msg)
{
    xrltTestFailurePtr   f;

    f = malloc(sizeof(xrltTestFailure));
    f->msg = strdup(msg);
    f->next = NULL;

    if (xrltTestFailureFirst == NULL) {
        xrltTestFailureFirst = f;
        xrltTestFailureLast = f;
    } else {
        xrltTestFailureLast->next = f;
        xrltTestFailureLast = f;
    }
}


void
xrltTestFailurePrint(void)
{
    xrltTestFailurePtr   f, f2;

    f = xrltTestFailureFirst;

    if (f != NULL) {
        printf("\n");
        xrltTestFailureFirst = NULL;
        xrltTestFailureLast = NULL;
    }

    while (f != NULL) {
        printf("Assertion error: %s\n", f->msg);
        free(f->msg);
        f2 = f->next;
        free(f);
        f = f2;
    }
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_TEST_H__ */
