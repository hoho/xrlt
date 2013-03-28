#include <xrlt.h>
#include <rddm.h>

#include <yajl/yajl_parse.h>


#define ASSERT_json2xml                                                       \
    xrltJSON2XMLPtr json2xml = (xrltJSON2XMLPtr)ctx;                          \
    if (json2xml == NULL) { return 0; }


inline const xmlChar*
yajlCallbackGetNodeName(xrltJSON2XMLPtr json2xml)
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


static int
yajlCallbackNull(void *ctx)
{
    ASSERT_json2xml;

    const xmlChar *name = yajlCallbackGetNodeName(json2xml);

    if (name == NULL || json2xml->cur == NULL) { return 0; }

    xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, NULL);
    if (node == NULL) { return 0; }

    xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_NULL);

    return 1;
}


static int
yajlCallbackBoolean(void *ctx, int value)
{
    ASSERT_json2xml;

    const xmlChar *val = (const xmlChar *)(value ? "true" : "false");

    if (json2xml->stackPos < 0) {
        xrltJSON2XMLSetType(json2xml, json2xml->cur, XRLT_JSON2XML_BOOLEAN);
        xmlNodeAddContent(json2xml->cur, val);
    } else {
        const xmlChar *name = yajlCallbackGetNodeName(json2xml);

        if (name == NULL || json2xml->cur == NULL) { return 0; }

        xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, val);
        if (node == NULL) {
            return 0;
        } else {
            xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_BOOLEAN);
        }
    }

    return 1;
}


static int
yajlCallbackNumber(void *ctx, const char *s, size_t l)
{
    ASSERT_json2xml;

    int       ret = 1;
    xmlChar  *val = xmlCharStrndup(s, l);

    if (json2xml->stackPos < 0) {
        xrltJSON2XMLSetType(json2xml, json2xml->cur, XRLT_JSON2XML_NUMBER);
        xmlNodeAddContent(json2xml->cur, val);
    } else {
        const xmlChar *name = yajlCallbackGetNodeName(json2xml);

        if (name == NULL || json2xml->cur == NULL) {
            ret = 0;
        } else {
            xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, val);
            if (node == NULL) {
                ret = 0;
            } else {
                xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_NUMBER);
            }
        }
    }

    xmlFree(val);
    return ret;
}


static int
yajlCallbackString(void *ctx, const unsigned char *s, size_t l)
{
    ASSERT_json2xml;

    int       ret = 1;
    xmlChar  *val = xmlCharStrndup((const char *)s, l);

    if (json2xml->stackPos < 0) {
        xrltJSON2XMLSetType(json2xml, json2xml->cur, XRLT_JSON2XML_STRING);
        xmlNodeAddContent(json2xml->cur, val);
    } else {
        const xmlChar *name = yajlCallbackGetNodeName(json2xml);

        if (name == NULL || json2xml->cur == NULL) {
            ret = 0;
        } else {
            xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, val);
            if (node == NULL) {
                ret = 0;
            } else {
                xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_STRING);
            }
        }
    }

    xmlFree(val);
    return ret;
}


static int
yajlCallbackMapStart(void *ctx)
{
    ASSERT_json2xml;

    int i = json2xml->stackPos;
    if (i < 0) {
        xrltJSON2XMLSetType(json2xml, json2xml->cur, XRLT_JSON2XML_OBJECT);
    } else {
        if (json2xml->stack[i].type == XRLT_JSON2XML_ARRAY ||
            json2xml->stack[i].type == XRLT_JSON2XML_OBJECT)
        {
            const xmlChar *name = yajlCallbackGetNodeName(json2xml);

            if (name == NULL || json2xml->cur == NULL) { return 0; }

            xmlNodePtr node = xmlNewTextChild(json2xml->cur, NULL, name, NULL);

            if (node == NULL) { return 0; }

            json2xml->cur = node;

            xrltJSON2XMLSetType(json2xml, node, XRLT_JSON2XML_OBJECT);
        }
    }

    return xrltJSON2XMLStackPush(json2xml, XRLT_JSON2XML_OBJECT) ? 1 : 0;
}


static int
yajlCallbackMapKey(void *ctx, const unsigned char *s, size_t l)
{
    ASSERT_json2xml;

    int       i = json2xml->stackPos;
    xmlChar  *key = json2xml->stack[i].key;

    if (key) { xmlFree(key); }

    json2xml->stack[i].key = xmlStrndup(s, l);

    return 1;
}


static int
yajlCallbackMapEnd(void *ctx)
{
    ASSERT_json2xml;
    return xrltJSON2XMLStackPop(json2xml) ? 1 : 0;
}


static int
yajlCallbackArrayStart(void *ctx)
{
    ASSERT_json2xml;

    xrltBool hasContent = FALSE; // To handle {"prop": []} case.

    int i = json2xml->stackPos;
    if (i < 0) {
        xrltJSON2XMLSetType(json2xml, json2xml->cur, XRLT_JSON2XML_ARRAY);
    } else {
        if (json2xml->stack[i].type == XRLT_JSON2XML_ARRAY) {
            const xmlChar *name = yajlCallbackGetNodeName(json2xml);

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


static int
yajlCallbackArrayEnd(void *ctx)
{
    ASSERT_json2xml;

    xrltBool hasContent, ret;

    if (json2xml->stackPos >= 0) {
        hasContent = json2xml->stack[json2xml->stackPos].hasContent;
    }

    ret = xrltJSON2XMLStackPop(json2xml);

    if (ret) {
        if (!hasContent) {
            const xmlChar *name = yajlCallbackGetNodeName(json2xml);

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


static yajl_callbacks yajlCallbacks = {
    yajlCallbackNull,
    yajlCallbackBoolean,
    NULL,
    NULL,
    yajlCallbackNumber,
    yajlCallbackString,
    yajlCallbackMapStart,
    yajlCallbackMapKey,
    yajlCallbackMapEnd,
    yajlCallbackArrayStart,
    yajlCallbackArrayEnd
};


xrltJSON2XMLPtr
xrltJSON2XMLInit(xmlNodePtr insert)
{
    xrltJSON2XMLPtr   ret = NULL;
    yajl_handle       parser;

    if (insert == NULL) { goto error; }

    ret = (xrltJSON2XMLPtr)xmlMalloc(sizeof(xrltJSON2XML));
    if (ret == NULL) { goto error; }

    memset(ret, 0, sizeof(xrltJSON2XML));

    ret->insert = insert;
    ret->cur = insert;
    ret->ns = xmlSearchNsByHref(insert->doc, insert, XRLT_NS);
    ret->stackPos = -1;

    parser = yajl_alloc(&yajlCallbacks, NULL, (void *)ret);
    if (parser == NULL) { goto error; }

    yajl_config(parser, yajl_allow_comments, 1);

    ret->parser = (void *)parser;

    return ret;

  error:
    if (ret) { xmlFree(ret); }

    return NULL;
}


xrltBool
xrltJSON2XMLFree(xrltJSON2XMLPtr json2xml)
{
    xrltBool      ret = TRUE;
    yajl_handle   parser = NULL;

    if (json2xml == NULL) {
        ret = FALSE;
        goto error;
    }

    parser = (yajl_handle)json2xml->parser;

    if (parser == NULL) {
        ret = FALSE;
        goto error;
    }

    if (yajl_complete_parse(parser) != yajl_status_ok) {
        ret = FALSE;
        goto error;
    }

  error:
    if (parser) { yajl_free(parser); }

    if (json2xml) {
        int i;

        for (i = 0; i <= json2xml->stackPos; i++) {
            if (json2xml->stack[i].key) {
                xmlFree(json2xml->stack[i].key);
            }
        }
        xmlFree(json2xml);
    }

    return ret;
}


xrltBool
xrltJSON2XMLFeed(xrltJSON2XMLPtr json2xml, xmlChar *chunk, size_t l)
{
    if (json2xml == NULL) { goto error; }

    yajl_handle parser = (yajl_handle)json2xml->parser;

    if (parser == NULL) { goto error; }

    if (yajl_parse(parser, (const unsigned char *)chunk, l) != yajl_status_ok) {
        goto error;
    }

    return TRUE;

  error:
    return FALSE;
}

xmlChar *
xrltJSON2XMLGetError(xrltJSON2XMLPtr json2xml, xmlChar *chunk, size_t l)
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
