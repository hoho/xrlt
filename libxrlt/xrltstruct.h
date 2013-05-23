/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_STRUCT_H__
#define __XRLT_STRUCT_H__


#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>


#ifdef __cplusplus
extern "C" {
#endif


#define xrltBool   int

#ifndef TRUE
#define TRUE       1
#endif  /* TRUE */

#ifndef FALSE
#define FALSE      0
#endif  /* FALSE */


typedef enum {
    XRLT_ERROR,
    XRLT_WARNING,
    XRLT_INFO,
    XRLT_DEBUG
} xrltLogType;


typedef enum {
    XRLT_METHOD_GET,
    XRLT_METHOD_HEAD,
    XRLT_METHOD_POST,
    XRLT_METHOD_PUT,
    XRLT_METHOD_DELETE,
    XRLT_METHOD_TRACE,
    XRLT_METHOD_OPTIONS
} xrltHTTPMethod;


typedef enum {
    //XRLT_SUBREQUEST_DATA_AUTO,
    XRLT_SUBREQUEST_DATA_XML = 0,
    XRLT_SUBREQUEST_DATA_JSON,
    XRLT_SUBREQUEST_DATA_TEXT,
    XRLT_SUBREQUEST_DATA_QUERYSTRING
} xrltSubrequestDataType;


typedef enum {
    XRLT_VALUE_EMPTY = 0,
    XRLT_VALUE_TEXT,
    XRLT_VALUE_NODELIST,
    XRLT_VALUE_XPATH,
    XRLT_VALUE_INT
} xrltValueType;


typedef enum {
    XRLT_HEADER_TYPE_HEADER = 0,
    XRLT_HEADER_TYPE_COOKIE,
    XRLT_HEADER_TYPE_STATUS
} xrltHeaderType;


typedef struct {
    char    *data;
    size_t   len;
} xrltString;


typedef struct _xrltHeader xrltHeader;
typedef xrltHeader* xrltHeaderPtr;
struct _xrltHeader {
    xrltHeaderType   type;
    xrltString       name;
    xrltString       val;
    xrltHeaderPtr    next;
};


typedef struct {
    xrltHeaderPtr   first;
    xrltHeaderPtr   last;
} xrltHeaderList;


typedef struct _xrltSubrequest xrltSubrequest;
typedef xrltSubrequest* xrltSubrequestPtr;
struct _xrltSubrequest {
    size_t                   id;
    xrltBool                 proxy;    // Indicates that parent request headers
                                       // should be sent with this subrequest.
    xrltHeaderList           header;
    xrltHTTPMethod           method;
    xrltSubrequestDataType   type;
    xrltString               url;
    xrltString               query;
    xrltString               body;
    xrltSubrequestPtr        next;
};


typedef struct {
    xrltSubrequestPtr   first;
    xrltSubrequestPtr   last;
} xrltSubrequestList;


typedef struct _xrltChunk xrltChunk;
typedef xrltChunk* xrltChunkPtr;
struct _xrltChunk {
    xrltString     data;
    xrltChunkPtr   next;
};

typedef struct {
    xrltChunkPtr   first;
    xrltChunkPtr   last;
} xrltChunkList;


typedef struct _xrltLog xrltLog;
typedef xrltLog* xrltLogPtr;
struct _xrltLog {
    xrltLogType   type;
    xrltString    msg;
    xrltLogPtr    next;
};

typedef struct {
    xrltLogPtr   first;
    xrltLogPtr   last;
} xrltLogList;


typedef struct {
    xmlNodePtr            src;
    xmlNodePtr            scope;
    xmlXPathCompExprPtr   expr;
} xrltXPathExpr;


typedef struct _xrltValue xrltValue;
typedef xrltValue* xrltValuePtr;
struct _xrltValue {
    xrltValueType   type;
    xmlChar        *textval;
    xmlNodePtr      nodeval;
    xrltXPathExpr   xpathval;
    int             intval;
};


#define CLEAR_XRLT_VALUE(val) {                                               \
    if (val.textval != NULL) {                                                \
        xmlFree(val.textval);                                                 \
    }                                                                         \
    if (val.xpathval.expr != NULL) {                                          \
        xmlXPathFreeCompExpr(val.xpathval.expr);                              \
    }                                                                         \
}


static inline xrltBool
        xrltStringInit            (xrltString *str, char *val);
static inline xrltBool
        xrltStringCopy            (xrltString *dst, xrltString *src);

static inline void
        xrltStringMove            (xrltString *dst, xrltString *src);

static inline void
        xrltStringClear           (xrltString *str);
static inline void
        xrltStringSet             (xrltString *str, char *val);


static inline void
        xrltHeaderListInit        (xrltHeaderList *list);
static inline xrltBool
        xrltHeaderListPush        (xrltHeaderList *list, xrltHeaderType type,
                                   xrltString *name, xrltString *val);
static inline xrltBool
        xrltHeaderListShift       (xrltHeaderList *list, xrltHeaderType *type,
                                   xrltString *name, xrltString *val);
static inline void
        xrltHeaderListClear       (xrltHeaderList *list);


static inline void
        xrltSubrequestListInit    (xrltSubrequestList *list);
static inline xrltBool
        xrltSubrequestListPush    (xrltSubrequestList *list, size_t id,
                                   xrltHTTPMethod method,
                                   xrltSubrequestDataType type,
                                   xrltHeaderList *header, xrltString *url,
                                   xrltString *query, xrltString *body);
static inline xrltBool
        xrltSubrequestListShift   (xrltSubrequestList *list, size_t *id,
                                   xrltHTTPMethod *method,
                                   xrltSubrequestDataType *type,
                                   xrltHeaderList *header, xrltString *url,
                                   xrltString *query, xrltString *body);
static inline void
        xrltSubrequestListClear   (xrltSubrequestList *list);


static inline void
        xrltChunkListInit         (xrltChunkList *list);
static inline xrltBool
        xrltChunkListPush         (xrltChunkList *list, xrltString *chunk);
static inline xrltBool
        xrltChunkListShift        (xrltChunkList *list, xrltString *chunk);
static inline void
        xrltChunkListClear        (xrltChunkList *list);


static inline void
        xrltLogListInit           (xrltLogList *list);
static inline xrltBool
        xrltLogListPush           (xrltLogList *list,
                                   xrltLogType type, xrltString *msg);
static inline xrltBool
        xrltLogListShift          (xrltLogList *list,
                                   xrltLogType *type, xrltString *msg);
static inline void
        xrltLogListClear          (xrltLogList *list);



static inline xrltBool
xrltStringInit(xrltString *str, char *val)
{
    str->len = strlen(val);
    str->data = strndup(val, str->len);
    return str->data == NULL ? FALSE : TRUE;
}


static inline xrltBool
xrltStringCopy(xrltString *dst, xrltString *src)
{
    //if (src == NULL || src->data == NULL || dst == NULL) { return FALSE; }
    dst->len = src->len;
    dst->data = src->len > 0 ? strndup(src->data, src->len) : NULL;
    return dst->data == NULL && dst->len > 0 ? FALSE : TRUE;
}


static inline void
xrltStringMove(xrltString *dst, xrltString *src)
{
    dst->data = src->data;
    dst->len = src->len;
}


static inline void
xrltStringClear(xrltString *str)
{
    if (str != NULL && str->data != NULL) {
        xmlFree(str->data);
        str->data = NULL;
        str->len = 0;
    }
}


static inline void
xrltStringSet(xrltString *str, char *val)
{
    str->len = strlen(val);
    str->data = val;
}


static inline void
xrltHeaderListInit(xrltHeaderList *list)
{
    memset(list, 0, sizeof(xrltHeaderList));
}


static inline xrltBool
xrltHeaderListPush(xrltHeaderList *list, xrltHeaderType type, xrltString *name,
                   xrltString *val)
{
    if (list == NULL || val == NULL) { return FALSE; }

    xrltHeaderPtr h;

    h = (xrltHeaderPtr)xmlMalloc(sizeof(xrltHeader));

    if (h == NULL) { return FALSE; }

    memset(h, 0, sizeof(xrltHeader));

    h->type = type;
    if (type != XRLT_HEADER_TYPE_STATUS) {
        if (!xrltStringCopy(&h->name, name)) { goto error; }
    }
    if (!xrltStringCopy(&h->val, val)) { goto error; }

    if (list->last == NULL) {
        list->first = h;
    } else {
        list->last->next = h;
    }
    list->last = h;

    return TRUE;

  error:
    xrltStringClear(&h->name);
    xrltStringClear(&h->val);
    xmlFree(h);

    return FALSE;
}


static inline xrltBool
xrltHeaderListShift(xrltHeaderList *list, xrltHeaderType *type,
                    xrltString *name, xrltString *val)
{
    if (list == NULL || type == NULL || name == NULL || val == NULL) {
        return FALSE;
    }

    xrltHeaderPtr h = list->first;

    if (h == NULL) { return FALSE; }

    *type = h->type;
    xrltStringMove(name, &h->name);
    xrltStringMove(val, &h->val);

    if (h->next == NULL) {
        list->first = NULL;
        list->last = NULL;
    } else {
        list->first = h->next;
    }

    xmlFree(h);

    return TRUE;
}


static inline void
xrltHeaderListClear(xrltHeaderList *list)
{
    xrltHeaderType   type;
    xrltString       name;
    xrltString       val;

    while (xrltHeaderListShift(list, &type, &name, &val)) {
        xrltStringClear(&name);
        xrltStringClear(&val);
    }
}


static inline void
xrltSubrequestListInit(xrltSubrequestList *list)
{
    memset(list, 0, sizeof(xrltSubrequestList));
}


static inline xrltBool
xrltSubrequestListPush(xrltSubrequestList *list, size_t id,
                       xrltHTTPMethod method, xrltSubrequestDataType type,
                       xrltHeaderList *header, xrltString *url,
                       xrltString *query, xrltString *body)
{
    if (list == NULL || id == 0 || url == NULL) { return FALSE; }

    xrltSubrequestPtr sr;

    sr = (xrltSubrequestPtr)xmlMalloc(sizeof(xrltSubrequest));

    if (sr == NULL) { return FALSE; }

    memset(sr, 0, sizeof(xrltSubrequest));

    sr->id = id;

    if (!xrltStringCopy(&sr->url, url)) { goto error; }

    sr->method = method;
    sr->type = type;

    if (query != NULL && !xrltStringCopy(&sr->query, query)) { goto error; }

    if (body != NULL && !xrltStringCopy(&sr->body, body)) { goto error; }

    if (header != NULL) {
        sr->header.first = header->first;
        sr->header.last = header->last;
        header->first = header->last = NULL;
    }

    if (list->last == NULL) {
        list->first = sr;
    } else {
        list->last->next = sr;
    }
    list->last = sr;

    return TRUE;

  error:
    xrltStringClear(&sr->url);
    xrltStringClear(&sr->query);
    xrltStringClear(&sr->body);

    xmlFree(sr);

    return FALSE;
}


static inline xrltBool
xrltSubrequestListShift(xrltSubrequestList *list, size_t *id,
                        xrltHTTPMethod *method, xrltSubrequestDataType *type,
                        xrltHeaderList *header, xrltString *url,
                        xrltString *query, xrltString *body)
{
    if (list == NULL || id == NULL || method == NULL || type == NULL ||
        header == NULL || url == NULL || query == NULL || body == NULL)
    {
        return FALSE;
    }

    xrltSubrequestPtr sr = list->first;

    if (sr == NULL) { return FALSE; }

    *id = sr->id;

    header->first = sr->header.first;
    header->last = sr->header.last;

    *method = sr->method;
    *type = sr->type;
    xrltStringMove(url, &sr->url);
    xrltStringMove(query, &sr->query);
    xrltStringMove(body, &sr->body);

    if (sr->next == NULL) {
        list->first = NULL;
        list->last = NULL;
    } else {
        list->first = sr->next;
    }

    xmlFree(sr);

    return TRUE;
}


static inline void
xrltSubrequestListClear(xrltSubrequestList *list)
{
    size_t                   id;
    xrltHTTPMethod           method;
    xrltSubrequestDataType   type;
    xrltHeaderList           header;
    xrltString               url;
    xrltString               query;
    xrltString               body;

    while (xrltSubrequestListShift(list, &id, &method, &type, &header, &url,
                                   &query, &body))
    {
        xrltHeaderListClear(&header);

        xrltStringClear(&url);
        xrltStringClear(&query);
        xrltStringClear(&body);
    }
}


static inline void
xrltChunkListInit(xrltChunkList *list)
{
    memset(list, 0, sizeof(xrltChunkList));
}


static inline xrltBool
xrltChunkListPush(xrltChunkList *list, xrltString *chunk)
{
    if (list == NULL || chunk == NULL) { return FALSE; }

    xrltChunkPtr c;

    c = (xrltChunkPtr)xmlMalloc(sizeof(xrltChunk));

    if (c == NULL) { return FALSE; }

    memset(c, 0, sizeof(xrltChunk));

    if (!xrltStringCopy(&c->data, chunk)) {
        xmlFree(c);
        return FALSE;
    }

    if (list->last == NULL) {
        list->first = c;
    } else {
        list->last->next = c;
    }
    list->last = c;

    return TRUE;
}


static inline xrltBool
xrltChunkListShift(xrltChunkList *list, xrltString *chunk)
{
    if (list == NULL || chunk == NULL) { return FALSE; }

    xrltChunkPtr c = list->first;

    if (c == NULL) { return FALSE; }

    xrltStringMove(chunk, &c->data);

    if (c->next == NULL) {
        list->first = NULL;
        list->last = NULL;
    } else {
        list->first = c->next;
    }

    xmlFree(c);

    return TRUE;
}


static inline void
xrltChunkListClear(xrltChunkList *list)
{
    xrltString   chunk;

    while (xrltChunkListShift(list, &chunk)) {
        xrltStringClear(&chunk);
    }
}


static inline void
xrltLogListInit(xrltLogList *list)
{
    memset(list, 0, sizeof(xrltLogList));
}


static inline xrltBool
xrltLogListPush(xrltLogList *list, xrltLogType type, xrltString *msg)
{
    if (list == NULL || msg == NULL) { return FALSE; }

    xrltLogPtr l;

    l = (xrltLogPtr)xmlMalloc(sizeof(xrltLog));

    if (l == NULL) { return FALSE; }

    memset(l, 0, sizeof(xrltLog));

    if (!xrltStringCopy(&l->msg, msg)) {
        xmlFree(l);
        return FALSE;
    }

    l->type = type;

    if (list->last == NULL) {
        list->first = l;
    } else {
        list->last->next = l;
    }
    list->last = l;

    return TRUE;
}


static inline xrltBool
xrltLogListShift(xrltLogList *list, xrltLogType *type, xrltString *msg)
{
    if (list == NULL || type == NULL || msg == NULL) { return FALSE; }

    xrltLogPtr l = list->first;

    if (l == NULL) { return FALSE; }

    *type = l->type;

    xrltStringMove(msg, &l->msg);

    if (l->next == NULL) {
        list->first = NULL;
        list->last = NULL;
    } else {
        list->first = l->next;
    }

    xmlFree(l);

    return TRUE;
}


static inline void
xrltLogListClear(xrltLogList *list)
{
    xrltLogType   type;
    xrltString    msg;

    while (xrltLogListShift(list, &type, &msg)) {
        xrltStringClear(&msg);
    }
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_STRUCT_H__ */
