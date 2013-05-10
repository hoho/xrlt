#include <xrlt.h>
#include "json2xml.h"


#define ASSERT_json2xml                                                       \
    xrltJSON2XMLPtr json2xml = (xrltJSON2XMLPtr)ctx;                          \
    if (json2xml == NULL) { return 0; }


inline const xmlChar*
xrltJSON2XMLGetNodeName(xrltJSON2XMLPtr json2xml)
{
    if (json2xml->stackPos >= 0) {
        xrltJSON2XMLStackItemPtr stackItem =
            &json2xml->stack[json2xml->stackPos];

        // We're getting name, so, current stack item has content.
        stackItem->hasContent = TRUE;

        if (stackItem->type == XRLT_JSON2XML_OBJECT) {
            // We're inside JSON object.
            return stackItem->key;
        } else if ((stackItem->type == XRLT_JSON2XML_ARRAY) &&
                   (json2xml->stackPos - 1 >= 0))
        {
            // We're inside JSON array, check if previous stack item is object.
            stackItem = &json2xml->stack[json2xml->stackPos - 1];
            if (stackItem == NULL) { return NULL; }

            if (stackItem->type == XRLT_JSON2XML_OBJECT) {
                // Previous stack item is object, return it's key.
                return stackItem->key;
            }
        }
    }
    // Default name for an item.
    return XRLT_JSON2XML_DEFAULT_NAME;
}


inline xrltBool
xrltJSON2XMLStackPush(xrltJSON2XMLPtr json2xml, xrltJSON2XMLType type)
{
    if (json2xml->stackPos + 1 <= MAX_JSON_DEPTH) {
        int i = ++json2xml->stackPos;

        memset(&json2xml->stack[i], 0, sizeof(xrltJSON2XMLStackItem));

        json2xml->stack[i].insert = json2xml->cur;
        json2xml->stack[i].type = type;

        return TRUE;
    } else {
        return FALSE;
    }
}


inline xrltBool
xrltJSON2XMLStackPop(xrltJSON2XMLPtr json2xml)
{
    int i = json2xml->stackPos;
    if (i >= 0) {
        if (json2xml->stack[i].key) {
            xmlFree(json2xml->stack[i].key);
        }
        json2xml->cur = json2xml->stack[i - 1].insert;
        json2xml->stackPos--;

        return TRUE;
    } else {
        return FALSE;
    }
}


inline void
xrltJSON2XMLSetType(xrltJSON2XMLPtr json2xml, xmlNodePtr node,
                    xrltJSON2XMLType type)
{
    if (json2xml == NULL || node == NULL) {
        return;
    }

    const xmlChar *val;
    switch (type) {
        case XRLT_JSON2XML_NUMBER:
            val = XRLT_JSON2XML_ATTR_TYPE_NUMBER;
            break;
        case XRLT_JSON2XML_BOOLEAN:
            val = XRLT_JSON2XML_ATTR_TYPE_BOOLEAN;
            break;
        case XRLT_JSON2XML_STRING:
            val = XRLT_JSON2XML_ATTR_TYPE_STRING;
            break;
        case XRLT_JSON2XML_NULL:
            val = XRLT_JSON2XML_ATTR_TYPE_NULL;
            break;
        case XRLT_JSON2XML_ARRAY:
            val = XRLT_JSON2XML_ATTR_TYPE_ARRAY;
            break;
        case XRLT_JSON2XML_OBJECT:
            val = XRLT_JSON2XML_ATTR_TYPE_OBJECT;
            break;
        default:
            val = NULL;
    }

    xmlSetNsProp(node, json2xml->ns, XRLT_JSON2XML_ATTR_TYPE, val);
}


int
xrltJSON2XMLNull(void *ctx)
{
    ASSERT_json2xml;

    const xmlChar *name = xrltJSON2XMLGetNodeName(json2xml);

    if (name == NULL || json2xml->cur == NULL) { return 0; }

    xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, NULL);
    if (node == NULL) { return 0; }

    xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_NULL);

    return 1;
}


int
xrltJSON2XMLBoolean(void *ctx, int value)
{
    ASSERT_json2xml;

    xmlNodePtr      node, parent;
    const xmlChar  *val = (const xmlChar *)(value ? "true" : "false");

    if (json2xml->stackPos < 0) {
        parent = json2xml->cur;
    } else {
        const xmlChar *name = xrltJSON2XMLGetNodeName(json2xml);

        if (name == NULL) {
            return 0;
        }

        parent = xmlNewChild(json2xml->cur, NULL, name, NULL);
    }

    if (parent == NULL) {
        return 0;
    }

    xrltJSON2XMLSetType(json2xml, parent, XRLT_JSON2XML_BOOLEAN);

    node = xmlNewText(val);

    if (node == NULL) {
        return 0;
    }

    if (xmlAddChild(parent, node) == NULL) {
        xmlFreeNode(node);

        return 0;
    }

    return 1;
}


int
xrltJSON2XMLNumber(void *ctx, const char *s, size_t l)
{
    ASSERT_json2xml;

    xmlNodePtr   node, parent;

    if (json2xml->stackPos < 0) {
        parent = json2xml->cur;
    } else {
        const xmlChar *name = xrltJSON2XMLGetNodeName(json2xml);

        if (name == NULL) {
            return 0;
        }

        parent = xmlNewChild(json2xml->cur, NULL, name, NULL);
    }

    if (parent == NULL) {
        return 0;
    }

    xrltJSON2XMLSetType(json2xml, parent, XRLT_JSON2XML_NUMBER);

    node = xmlNewTextLen((const xmlChar *)s, l);

    if (node == NULL) {
        return 0;
    }

    if (xmlAddChild(parent, node) == NULL) {
        xmlFreeNode(node);

        return 0;
    }

    return 1;
}


int
xrltJSON2XMLString(void *ctx, const unsigned char *s, size_t l)
{
    ASSERT_json2xml;

    xmlNodePtr   node, parent;

    if (json2xml->stackPos < 0) {
        parent = json2xml->cur;
    } else {
        const xmlChar *name = xrltJSON2XMLGetNodeName(json2xml);

        if (name == NULL) {
            return 0;
        }

        parent = xmlNewChild(json2xml->cur, NULL, name, NULL);
    }

    if (parent == NULL) {
        return 0;
    }

    xrltJSON2XMLSetType(json2xml, parent, XRLT_JSON2XML_STRING);

    node = xmlNewTextLen(s, l);

    if (node == NULL) {
        return 0;
    }

    if (xmlAddChild(parent, node) == NULL) {
        xmlFreeNode(node);

        return 0;
    }

    return 1;
}


int
xrltJSON2XMLMapStart(void *ctx)
{
    ASSERT_json2xml;

    int i = json2xml->stackPos;
    if (i < 0) {
        xrltJSON2XMLSetType(json2xml, json2xml->cur, XRLT_JSON2XML_OBJECT);
    } else {
        if (json2xml->stack[i].type == XRLT_JSON2XML_ARRAY ||
            json2xml->stack[i].type == XRLT_JSON2XML_OBJECT)
        {
            const xmlChar *name = xrltJSON2XMLGetNodeName(json2xml);

            if (name == NULL || json2xml->cur == NULL) { return 0; }

            xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, NULL);

            if (node == NULL) { return 0; }

            json2xml->cur = node;

            xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_OBJECT);
        }
    }

    return xrltJSON2XMLStackPush(json2xml, XRLT_JSON2XML_OBJECT) ? 1 : 0;
}


int
xrltJSON2XMLMapKey(void *ctx, const unsigned char *s, size_t l)
{
    ASSERT_json2xml;

    int       i = json2xml->stackPos;
    xmlChar  *key = json2xml->stack[i].key;

    if (key) { xmlFree(key); }

    json2xml->stack[i].key = xmlStrndup(s, l);

    return 1;
}


int
xrltJSON2XMLMapEnd(void *ctx)
{
    ASSERT_json2xml;
    return xrltJSON2XMLStackPop(json2xml) ? 1 : 0;
}


int
xrltJSON2XMLArrayStart(void *ctx)
{
    ASSERT_json2xml;

    xrltBool hasContent = FALSE; // To handle {"prop": []} case.

    int i = json2xml->stackPos;
    if (i < 0) {
        xrltJSON2XMLSetType(json2xml, json2xml->cur, XRLT_JSON2XML_ARRAY);
    } else {
        if (json2xml->stack[i].type == XRLT_JSON2XML_ARRAY) {
            const xmlChar *name = xrltJSON2XMLGetNodeName(json2xml);

            if (name == NULL || json2xml->cur == NULL) { return 0; }

            xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, NULL);

            if (node == NULL) {
                return 0;
            } else {
                xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_ARRAY);
            }

            json2xml->cur = node;

            hasContent = TRUE;
        }
    }

    xrltBool ret = xrltJSON2XMLStackPush(json2xml, XRLT_JSON2XML_ARRAY);

    if (ret) {
        json2xml->stack[json2xml->stackPos].hasContent = hasContent;
        return 1;
    } else {
        return 0;
    }
}


int
xrltJSON2XMLArrayEnd(void *ctx)
{
    ASSERT_json2xml;

    xrltBool hasContent, ret;

    if (json2xml->stackPos >= 0) {
        hasContent = json2xml->stack[json2xml->stackPos].hasContent;
    }

    ret = xrltJSON2XMLStackPop(json2xml);

    if (ret) {
        if (!hasContent) {
            const xmlChar *name = xrltJSON2XMLGetNodeName(json2xml);

            if (name == NULL || json2xml->cur == NULL) { return 0; }

            xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, NULL);

            if (node == NULL) {
                return 0;
            } else {
                xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_ARRAY);
            }
        }

        return 1;
    }

    return 0;
}


static yajl_callbacks xrltJSON2XMLCallbacks = {
    xrltJSON2XMLNull,
    xrltJSON2XMLBoolean,
    NULL,
    NULL,
    xrltJSON2XMLNumber,
    xrltJSON2XMLString,
    xrltJSON2XMLMapStart,
    xrltJSON2XMLMapKey,
    xrltJSON2XMLMapEnd,
    xrltJSON2XMLArrayStart,
    xrltJSON2XMLArrayEnd
};


xrltJSON2XMLPtr
xrltJSON2XMLInit(xmlNodePtr insert, xrltBool noparser)
{
    xrltJSON2XMLPtr   ret = NULL;

    if (insert == NULL) { goto error; }

    ret = (xrltJSON2XMLPtr)xmlMalloc(sizeof(xrltJSON2XML));
    if (ret == NULL) { goto error; }

    memset(ret, 0, sizeof(xrltJSON2XML));

    ret->insert = insert;
    ret->cur = insert;
    ret->ns = xmlSearchNsByHref(insert->doc, insert, XRLT_NS);
    ret->stackPos = -1;

    if (!noparser) {
        ret->parser = yajl_alloc(&xrltJSON2XMLCallbacks, NULL, (void *)ret);
        if (ret->parser == NULL) { goto error; }

        yajl_config(ret->parser, yajl_allow_comments, 1);
    }

    return ret;

  error:
    if (ret) { xmlFree(ret); }

    return NULL;
}


void
xrltJSON2XMLFree(xrltJSON2XMLPtr json2xml)
{
    if (json2xml == NULL) { return; }

    int i;

    if (json2xml->parser != NULL) {
        yajl_complete_parse(json2xml->parser);
        yajl_free(json2xml->parser);
    }

    for (i = 0; i <= json2xml->stackPos; i++) {
        if (json2xml->stack[i].key != NULL) {
            xmlFree(json2xml->stack[i].key);
        }
    }

    xmlFree(json2xml);
}


xrltBool
xrltJSON2XMLFeed(xrltJSON2XMLPtr json2xml, char *chunk, size_t l)
{
    if (json2xml == NULL || json2xml->parser == NULL) { return FALSE; }

    if (yajl_parse(json2xml->parser,
                   (const unsigned char *)chunk, l) != yajl_status_ok)
    {
        return FALSE;
    }

    return TRUE;
}

xmlChar *
xrltJSON2XMLGetError(xrltJSON2XMLPtr json2xml, char *chunk, size_t l)
{
    xmlChar        *ret;
    unsigned char  *err;

    if (json2xml == NULL) { return NULL; }

    if (chunk == NULL) { l = 0; }

    err = yajl_get_error(json2xml->parser, l, (const unsigned char *)chunk, l);

    ret = xmlStrdup((const xmlChar *)err);

    yajl_free_error(json2xml->parser, err);

    return ret;
}
