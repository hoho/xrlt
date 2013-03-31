#ifndef __XRLT_JSON2XML_H__
#define __XRLT_JSON2XML_H__


#include <libxml/tree.h>

#include <xrltstruct.h>


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_JSON_DEPTH 20

#define XRLT_JSON2XML_DEFAULT_NAME (const xmlChar *)"item"
#define XRLT_JSON2XML_ATTR_TYPE (const xmlChar *)"type"
#define XRLT_JSON2XML_ATTR_TYPE_STRING (const xmlChar *)"string"
#define XRLT_JSON2XML_ATTR_TYPE_NUMBER (const xmlChar *)"number"
#define XRLT_JSON2XML_ATTR_TYPE_BOOLEAN (const xmlChar *)"boolean"
#define XRLT_JSON2XML_ATTR_TYPE_OBJECT (const xmlChar *)"object"
#define XRLT_JSON2XML_ATTR_TYPE_ARRAY (const xmlChar *)"array"
#define XRLT_JSON2XML_ATTR_TYPE_NULL (const xmlChar *)"null"
#define XRLT_JSON2XML_ATTR_VALUE_YES (const xmlChar *)"yes"


typedef enum {
    XRLT_JSON2XML_STRING,
    XRLT_JSON2XML_NUMBER,
    XRLT_JSON2XML_BOOLEAN,
    XRLT_JSON2XML_OBJECT,
    XRLT_JSON2XML_ARRAY,
    XRLT_JSON2XML_NULL,
    XRLT_JSON2XML_UNKNOWN = 404
} xrltJSON2XMLType;


typedef struct _xrltJSON2XMLStackItem xrltJSON2XMLStackItem;
typedef xrltJSON2XMLStackItem* xrltJSON2XMLStackItemPtr;
struct _xrltJSON2XMLStackItem {
    xrltJSON2XMLType   type;
    xrltBool           hasContent;
    xmlChar           *key;
    xmlNodePtr         insert;
};


typedef struct _xrltJSON2XML xrltJSON2XML;
typedef xrltJSON2XML* xrltJSON2XMLPtr;
struct _xrltJSON2XML {
    xmlNodePtr              cur;
    xmlNodePtr              insert;
    xmlNsPtr                ns;
    xrltJSON2XMLStackItem   stack[MAX_JSON_DEPTH];
    int                     stackPos;
    void                   *parser;
};


xrltJSON2XMLPtr
        xrltJSON2XMLInit           (xmlNodePtr insert);
xrltBool
        xrltJSON2XMLFree           (xrltJSON2XMLPtr json2xml);
xrltBool
        xrltJSON2XMLFeed           (xrltJSON2XMLPtr json2xml, xmlChar *chunk,
                                    size_t l);
xmlChar *
        xrltJSON2XMLGetError       (xrltJSON2XMLPtr json2xml, xmlChar *chunk,
                                    size_t l);


#ifdef __cplusplus
}
#endif

#endif /* __XRLT_JSON2XML_H__ */
