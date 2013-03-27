#ifndef __XRLTSTRUCT_H__
#define __XRLTSTRUCT_H__


#include <string.h>

#include <xrltmemory.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    FALSE = 0,
    TRUE
} xrltBool;


typedef enum {
    XRLT_ERROR,
    XRLT_WARNING,
    XRLT_INFO,
    XRLT_DEBUG
} xrltLogType;


typedef struct {
    char    *data;
    size_t   len;
} xrltString;


typedef struct _xrltHeader xrltHeader;
typedef xrltHeader* xrltHeaderPtr;
struct _xrltHeader {
    xrltString      name;
    xrltString      value;
    xrltHeaderPtr   next;
};

typedef struct {
    xrltHeaderPtr   first;
    xrltHeaderPtr   last;
} xrltHeaderList;


typedef struct _xrltSubrequest xrltSubrequest;
typedef xrltSubrequest* xrltSubrequestPtr;
struct _xrltSubrequest {
    size_t              id;
    xrltHeaderList      header;
    xrltString          url;
    xrltString          query;
    xrltString          body;
    xrltSubrequestPtr   next;
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


inline xrltBool
        xrltStringInit            (xrltString *str, char *val);
inline xrltBool
        xrltStringCopy            (xrltString *dst, xrltString *src);

inline void
        xrltStringMove            (xrltString *dst, xrltString *src);

inline void
        xrltStringClear            (xrltString *str);


inline void
        xrltHeaderListInit        (xrltHeaderList *list);
inline xrltBool
        xrltHeaderListPush        (xrltHeaderList *list,
                                   xrltString *name, xrltString *value);
inline xrltBool
        xrltHeaderListShift       (xrltHeaderList *list,
                                   xrltString *name, xrltString *value);
inline void
        xrltHeaderListClear       (xrltHeaderList *list);


inline void
        xrltSubrequestListInit    (xrltSubrequestList *list);
inline xrltBool
        xrltSubrequestListPush    (xrltSubrequestList *list, size_t id,
                                   xrltHeaderList *header, xrltString *url,
                                   xrltString *query, xrltString *body);
inline xrltBool
        xrltSubrequestListShift   (xrltSubrequestList *list, size_t *id,
                                   xrltHeaderList *header, xrltString *url,
                                   xrltString *query, xrltString *body);
inline void
        xrltSubrequestListClear   (xrltSubrequestList *list);


inline void
        xrltChunkListInit         (xrltChunkList *list);
inline xrltBool
        xrltChunkListPush         (xrltChunkList *list, xrltString *chunk);
inline xrltBool
        xrltChunkListShift        (xrltChunkList *list, xrltString *chunk);
inline void
        xrltChunkListClear        (xrltChunkList *list);


inline void
        xrltLogListInit           (xrltLogList *list);
inline xrltBool
        xrltLogListPush           (xrltLogList *list,
                                   xrltLogType type, xrltString *msg);
inline xrltBool
        xrltLogListShift          (xrltLogList *list,
                                   xrltLogType *type, xrltString *msg);
inline void
        xrltLogListClear          (xrltLogList *list);



inline xrltBool
xrltStringInit(xrltString *str, char *val)
{
    str->len = strlen(val);
    str->data = strndup(val, str->len);
    return str->data == NULL ? FALSE : TRUE;
}


inline xrltBool
xrltStringCopy(xrltString *dst, xrltString *src)
{
    //if (src == NULL || src->data == NULL || dst == NULL) { return FALSE; }
    dst->data = strndup(src->data, src->len);
    dst->len = src->len;
    return dst->data == NULL ? FALSE : TRUE;
}


inline void
xrltStringMove(xrltString *dst, xrltString *src)
{
    dst->data = src->data;
    dst->len = src->len;
}


inline void
xrltStringClear(xrltString *str)
{
    if (str != NULL && str->data != NULL) {
        xrltFree(str->data);
        str->data = NULL;
        str->len = 0;
    }
}


inline void
xrltHeaderListInit(xrltHeaderList *list)
{
    memset(list, 0, sizeof(xrltHeaderList));
}


inline xrltBool
xrltHeaderListPush(xrltHeaderList *list, xrltString *name, xrltString *value)
{
    if (list == NULL || name == NULL || value == NULL) { return FALSE; }

    xrltHeaderPtr h;

    h = xrltMalloc(sizeof(xrltHeader));

    if (h == NULL) { return FALSE; }

    memset(h, 0, sizeof(xrltHeader));

    if (!xrltStringCopy(&h->name, name)) { goto error; }
    if (!xrltStringCopy(&h->value, value)) { goto error; }

    if (list->last == NULL) {
        list->first = h;
    } else {
        list->last->next = h;
    }
    list->last = h;

    return TRUE;

  error:
    xrltStringClear(&h->name);
    xrltStringClear(&h->value);
    xrltFree(h);

    return FALSE;
}


inline xrltBool
xrltHeaderListShift(xrltHeaderList *list, xrltString *name, xrltString *value)
{
    if (list == NULL || name == NULL || value == NULL) { return FALSE; }

    xrltHeaderPtr h = list->first;

    if (h == NULL) { return FALSE; }

    xrltStringMove(name, &h->name);
    xrltStringMove(value, &h->value);

    if (h->next == NULL) {
        list->first = NULL;
        list->last = NULL;
    } else {
        list->first = h->next;
    }

    xrltFree(h);

    return TRUE;
}


inline void
xrltHeaderListClear(xrltHeaderList *list)
{
    xrltString   name;
    xrltString   value;

    while (xrltHeaderListShift(list, &name, &value)) {
        xrltStringClear(&name);
        xrltStringClear(&value);
    }
}


inline void
xrltSubrequestListInit(xrltSubrequestList *list)
{
    memset(list, 0, sizeof(xrltSubrequestList));
}


inline xrltBool
xrltSubrequestListPush(xrltSubrequestList *list,
                       size_t id, xrltHeaderList *header, xrltString *url,
                       xrltString *query, xrltString *body)
{
    if (list == NULL || id == 0 || url == NULL) { return FALSE; }

    xrltSubrequestPtr sr;

    sr = xrltMalloc(sizeof(xrltSubrequest));

    if (sr == NULL) { return FALSE; }

    memset(sr, 0, sizeof(xrltSubrequest));

    sr->id = id;

    if (!xrltStringCopy(&sr->url, url)) { goto error; }

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

    xrltFree(sr);

    return FALSE;
}


inline xrltBool
xrltSubrequestListShift(xrltSubrequestList *list,
                        size_t *id, xrltHeaderList *header, xrltString *url,
                        xrltString *query, xrltString *body)
{
    if (list == NULL || id == NULL || header == NULL || url == NULL ||
        query == NULL || body == NULL)
    {
        return FALSE;
    }

    xrltSubrequestPtr sr = list->first;

    if (sr == NULL) { return FALSE; }

    *id = sr->id;

    header->first = sr->header.first;
    header->last = sr->header.last;

    xrltStringMove(url, &sr->url);
    xrltStringMove(query, &sr->query);
    xrltStringMove(body, &sr->body);

    if (sr->next == NULL) {
        list->first = NULL;
        list->last = NULL;
    } else {
        list->first = sr->next;
    }

    xrltFree(sr);

    return TRUE;
}


inline void
xrltSubrequestListClear(xrltSubrequestList *list)
{
    size_t           id;
    xrltHeaderList   header;
    xrltString       url;
    xrltString       query;
    xrltString       body;

    while (xrltSubrequestListShift(list, &id, &header, &url, &query, &body)) {
        xrltHeaderListClear(&header);

        xrltStringClear(&url);
        xrltStringClear(&query);
        xrltStringClear(&body);
    }
}


inline void
xrltChunkListInit(xrltChunkList *list)
{
    memset(list, 0, sizeof(xrltChunkList));
}


inline xrltBool
xrltChunkListPush(xrltChunkList *list, xrltString *chunk)
{
    if (list == NULL || chunk == NULL) { return FALSE; }

    xrltChunkPtr c;

    c = xrltMalloc(sizeof(xrltChunk));

    if (c == NULL) { return FALSE; }

    memset(c, 0, sizeof(xrltChunk));

    if (!xrltStringCopy(&c->data, chunk)) {
        xrltFree(c);
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


inline xrltBool
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

    xrltFree(c);

    return TRUE;
}


inline void
xrltChunkListClear(xrltChunkList *list)
{
    xrltString   chunk;

    while (xrltChunkListShift(list, &chunk)) {
        xrltStringClear(&chunk);
    }
}


inline void
xrltLogListInit(xrltLogList *list)
{
    memset(list, 0, sizeof(xrltLogList));
}


inline xrltBool
xrltLogListPush(xrltLogList *list, xrltLogType type, xrltString *msg)
{
    if (list == NULL || msg == NULL) { return FALSE; }

    xrltLogPtr l;

    l = xrltMalloc(sizeof(xrltLog));

    if (l == NULL) { return FALSE; }

    memset(l, 0, sizeof(xrltLog));

    if (!xrltStringCopy(&l->msg, msg)) {
        xrltFree(l);
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


inline xrltBool
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

    xrltFree(l);

    return TRUE;
}


inline void
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

#endif /* __XRLTSTRUCT_H__ */
