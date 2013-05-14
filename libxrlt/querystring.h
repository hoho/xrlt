/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_QUERYSTRING_H__
#define __XRLT_QUERYSTRING_H__


#include <libxml/tree.h>
#include <ctype.h>
#include <xrlt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _xrltQueryStringParser xrltQueryStringParser;
typedef xrltQueryStringParser* xrltQueryStringParserPtr;
struct _xrltQueryStringParser {
    xmlNodePtr   parent;
    char        *name;
    size_t       nameLen;
    size_t       nameSize;
    char        *val;
    size_t       valLen;
    size_t       valSize;
    xrltBool     isVal;
};


xrltQueryStringParserPtr
        xrltQueryStringParserInit   (xmlNodePtr parent);
xrltBool
        xrltQueryStringParserFeed   (xrltQueryStringParserPtr parser,
                                     char *data, size_t len, xrltBool last);
void
        xrltQueryStringParserFree   (xrltQueryStringParserPtr parser);


static inline xmlChar
xrltFromHex(char ch)
{
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}


static inline char
xrltToHex(char code)
{
    static char hex[] = "0123456789ABCDEF";
    return hex[code & 15];
}


static inline size_t
xrltURLEncode(char *str, char *out)
{
    if (str == NULL || out == NULL) { return 0; }

    char *pstr = str;
    char *pbuf = out;

    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' ||
                *pstr == '~')
        {
            *pbuf++ = *pstr;
        } else if (*pstr == ' ') {
            *pbuf++ = '+';
        } else {
            *pbuf++ = '%';
            *pbuf++ = xrltToHex(*pstr >> 4);
            *pbuf++ = xrltToHex(*pstr & 15);
        }

        pstr++;
    }

    *pbuf = '\0';

    return pbuf - out;
}


static inline xmlChar *
xrltURLDecode(char *str, size_t len)
{
    if (str == NULL) { return NULL; }

    if (len == 0) { len = strlen(str); }

    xmlChar *pstr = (xmlChar *)str;
    xmlChar *buf = (xmlChar *)xrltMalloc(len + 1);
    xmlChar *pbuf = buf;

    if (buf == NULL) { return NULL; }

    while (len) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf++ = xrltFromHex(pstr[1]) << 4 | xrltFromHex(pstr[2]);
                pstr += 2;
                len -= 2;
            }
        } else if (*pstr == '+') {
            *pbuf++ = ' ';
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
        len--;
    }

    *pbuf = '\0';

    return buf;
}


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_QUERYSTRING_H__ */
