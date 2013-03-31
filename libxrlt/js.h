#ifndef __XRLT_JS_H__
#define __XRLT_JS_H__


#include <xrltstruct.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltJSContext xrltJSContext;
typedef xrltJSContext* xrltJSContextPtr;
struct _xrltJSContext {
    void *_private;
};


typedef struct _xrltJSArgument xrltJSArgument;
typedef xrltJSArgument* xrltJSArgumentPtr;
struct _xrltJSArgument {
    char               *name;
    //xmlXPathObjectPtr   val;
    xrltJSArgumentPtr   next;
};


typedef struct {
    xrltJSArgumentPtr   first;
    xrltJSArgumentPtr   last;
} xrltJSArgumentList;


static inline void
        xrltJSArgumentListInit    (xrltJSArgumentList *list);
static inline xrltBool
        xrltJSArgumentListPush    (xrltJSArgumentList *list,
                                       char *arg);
static inline void
        xrltJSArgumentListClear   (xrltJSArgumentList *list);

void
        xrltJSInit                (void);

xrltJSContextPtr
        xrltJSContextCreate       (void);
void
        xrltJSContextFree         (xrltJSContextPtr jsctx);
xrltBool
        xrltJSSlice               (xrltJSContextPtr jsctx, char *name,
                                   xrltJSArgumentList *args, char *code);
xrltBool
        xrltJSApply               (xrltJSContextPtr jsctx, char *name,
                                   xrltJSArgumentList *args);


static inline void
xrltJSArgumentListInit(xrltJSArgumentList *list)
{
    if (list != NULL) {
        memset(list, 0, sizeof(xrltJSArgumentList));
    }
}


static inline xrltBool
xrltJSArgumentListPush(xrltJSArgumentList *list, char *arg)
{
    if (list == NULL || arg == NULL) { return FALSE; }

    xrltJSArgumentPtr   a;
    size_t              len = strlen(arg);

    a = (xrltJSArgumentPtr)xrltMalloc(sizeof(xrltJSArgument) + len + 1);

    if (a == NULL) { return FALSE; }

    memset(a, 0, sizeof(xrltJSArgument) + len + 1);

    a->name = (char *)(a + 1);
    memcpy(a->name, arg, len);

    if (list->first == NULL) {
        list->first = a;
    } else {
        list->last->next = a;
    }
    list->last = a;

    return TRUE;
}


static inline void
xrltJSArgumentListClear(xrltJSArgumentList *list)
{
    xrltJSArgumentPtr   a, a2;

    a = list->first;

    while (a != NULL) {
        a2 = a->next;
        xrltFree(a);
        a = a2;
    }

    xrltJSArgumentListInit(list);
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_JS_H__ */
