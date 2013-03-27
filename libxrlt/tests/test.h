#ifndef __XRLT_TEST_H__
#define __XRLT_TEST_H__


#include <stdlib.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


char   _msgbuf[128];


#define TEST_PASSED printf("."); return;

#define TEST_FAILED sprintf(_msgbuf, "%s:%d", __FILE__, __LINE__);            \
                    xrltTestFailurePush(_msgbuf);                             \
                    printf("F");                                              \
                    return;

#define ASSERT(cond) if (!(cond)) { TEST_FAILED; }


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
