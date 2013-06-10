#include "xml2json.h"
#include "ccan_json.h"

xrltXML2JSONTypeMap::xrltXML2JSONTypeMap()
{
    types.insert(
        std::pair<std::string, xrltJSON2XMLType>(
            (const char *)XRLT_JSON2XML_ATTR_TYPE_STRING,
            XRLT_JSON2XML_STRING
        )
    );
    types.insert(
        std::pair<std::string, xrltJSON2XMLType>(
            (const char *)XRLT_JSON2XML_ATTR_TYPE_NUMBER,
            XRLT_JSON2XML_NUMBER
        )
    );
    types.insert(
        std::pair<std::string, xrltJSON2XMLType>(
            (const char *)XRLT_JSON2XML_ATTR_TYPE_BOOLEAN,
            XRLT_JSON2XML_BOOLEAN
        )
    );
    types.insert(
        std::pair<std::string, xrltJSON2XMLType>(
            (const char *)XRLT_JSON2XML_ATTR_TYPE_OBJECT,
            XRLT_JSON2XML_OBJECT
        )
    );
    types.insert(
        std::pair<std::string, xrltJSON2XMLType>(
            (const char *)XRLT_JSON2XML_ATTR_TYPE_ARRAY,
            XRLT_JSON2XML_ARRAY
        )
    );
    types.insert(
        std::pair<std::string, xrltJSON2XMLType>(
            (const char *)XRLT_JSON2XML_ATTR_TYPE_NULL,
            XRLT_JSON2XML_NULL
        )
    );
}
xrltXML2JSONTypeMap XRLT_TYPE_MAP;


xrltJSON2XMLType xrltXML2JSONTypeMap::getType(xmlChar *str)
{
    std::map<std::string, xrltJSON2XMLType>::iterator i;

    i = types.find((const char *)str);

    return i != types.end() ? i->second : XRLT_JSON2XML_UNKNOWN;
}


xrltXML2JSONData::xrltXML2JSONData(xmlNodePtr parent, xmlXPathObjectPtr val,
                                   void *cache) : cache(cache)
{
    xmlNodePtr   n;

    if (parent != NULL) {
        n = parent->children;

        // Create a list of nodes for XML2JSON object.
        while (n != NULL) {
            if (!xmlIsBlankNode(n)) {
                nodes.push_back(n);
            }
            n = n->next;
        }
    } else if (val != NULL && val->nodesetval != NULL &&
               val->nodesetval->nodeTab != NULL)
    {
        int   i;

        for (i = 0; i < val->nodesetval->nodeNr; i++) {
            nodes.push_back(val->nodesetval->nodeTab[i]);
        }
    }

    std::vector<xmlNodePtr>::iterator   i;
    std::string                         key;

    // Create a multimap to find nodes by names.
    for(i = nodes.begin(); i != nodes.end(); i++) {
        n = *i;
        if (n != NULL) {
            key.assign((const char *)n->name);
            named.insert(std::pair<std::string, xmlNodePtr>(key, n));
        }
    }

    // Determine XML2JSON object type.
    xmlChar  *typestr = NULL;

    if (parent != NULL) {
        typestr = xmlGetNsProp(parent, XRLT_JSON2XML_ATTR_TYPE, XRLT_NS);

        if (typestr != NULL) {
            type = XRLT_TYPE_MAP.getType(typestr);
            xmlFree(typestr);
        }
    }

    if (typestr == NULL || type == XRLT_JSON2XML_UNKNOWN) {
        if (named.size() > 1 && nodes.size() == named.count(key)) {
            type = XRLT_JSON2XML_ARRAY;
        }

        if (named.size() == 0 && parent != NULL &&
            (parent->type == XML_TEXT_NODE ||
             parent->type == XML_CDATA_SECTION_NODE))
        {
            type = XRLT_JSON2XML_STRING;
        }
    }
}


#define RAISE_OUT_OF_MEMORY_IF_TRUE(cond)                                     \
    if (cond) {                                                               \
        ERROR_OUT_OF_MEMORY(ctx, NULL, NULL);                                 \
        return FALSE;                                                         \
    }

#define ENCODE_ADD_STRING(str)                                                \
    tmp = json_encode_string((const char *)str);                              \
    RAISE_OUT_OF_MEMORY_IF_TRUE(tmp == NULL);                                 \
    if (xmlBufferAdd(*buf, (const xmlChar *)tmp, -1) != 0) {                  \
        free(tmp);                                                            \
        ERROR_OUT_OF_MEMORY(ctx, NULL, NULL);                                 \
        return FALSE;                                                         \
    } else {                                                                  \
        free(tmp);                                                            \
    }

#define ADD_CONST(chars)                                                      \
    RAISE_OUT_OF_MEMORY_IF_TRUE(                                              \
        xmlBufferAdd(*buf, (const xmlChar *)chars, sizeof(chars) - 1) != 0    \
    );

xrltBool
xrltXML2JSONStringify(xrltContextPtr ctx, xmlNodePtr parent,
                      xmlXPathObjectPtr val, xmlBufferPtr *buf)
{
    xrltXML2JSONData    *data = NULL;
    size_t               i;
    xrltBool             prev;
    JsonNode            *jsnode;
    char                *tmp;

    if (buf == NULL) {
        return FALSE;
    }

    if (*buf == NULL) {
        *buf = xmlBufferCreate();

        RAISE_OUT_OF_MEMORY_IF_TRUE(*buf == NULL);
    }

    if (val != NULL) {
        switch (val->type) {
            case XPATH_STRING:
                ENCODE_ADD_STRING(val->stringval);

                return TRUE;

            case XPATH_NUMBER:
                jsnode = json_mknumber(val->floatval);

                RAISE_OUT_OF_MEMORY_IF_TRUE(jsnode == NULL);

                tmp = json_encode(jsnode);

                free(jsnode);

                RAISE_OUT_OF_MEMORY_IF_TRUE(tmp == NULL);

                if (xmlBufferAdd(*buf, (xmlChar *)tmp, -1) != 0) {
                    free(tmp);

                    ERROR_OUT_OF_MEMORY(ctx, NULL, NULL);

                    return FALSE;
                }

                free(tmp);

                return TRUE;

            case XPATH_BOOLEAN:
                if (val->boolval) {
                    ADD_CONST("true");
                } else {
                    ADD_CONST("false");
                }

                return TRUE;

            case XPATH_NODESET:
                break;

            case XPATH_UNDEFINED:
            case XPATH_POINT:
            case XPATH_RANGE:
            case XPATH_LOCATIONSET:
            case XPATH_USERS:
            case XPATH_XSLT_TREE:
                return FALSE;
        }

        if (val->nodesetval == NULL || val->nodesetval->nodeTab == NULL ||
            val->nodesetval->nodeNr == 0)
        {
            xrltTransformError(ctx, NULL, NULL,
                               "Can't JSON-stringify empty nodeset\n");
            return FALSE;
        }

        if (val->nodesetval->nodeNr == 1 &&
            val->nodesetval->nodeTab[0]->type == XML_DOCUMENT_NODE)
        {
            data = new xrltXML2JSONData(val->nodesetval->nodeTab[0],
                                        NULL, NULL);
            RAISE_OUT_OF_MEMORY_IF_TRUE(data == NULL);
        } else {
            data = new xrltXML2JSONData(NULL, val, NULL);

            RAISE_OUT_OF_MEMORY_IF_TRUE(data == NULL);
        }
    } else {
        data = new xrltXML2JSONData(parent, NULL, NULL);

        RAISE_OUT_OF_MEMORY_IF_TRUE(data == NULL);
    }

    if (data->type == XRLT_JSON2XML_ARRAY) {
        ADD_CONST("[");

        prev = FALSE;

        for (i = 0; i < data->nodes.size(); i++) {
            if (prev) {
                ADD_CONST(", ");
            }

            if (!xrltXML2JSONStringify(ctx, data->nodes[i], NULL, buf)) {
                return FALSE;
            }

            prev = TRUE;
        }

        ADD_CONST("]");
    } else {
        if (data->nodes.size() == 0) {
            if (data->type == XRLT_JSON2XML_OBJECT) {
                ADD_CONST("{}");
            } else if (data->type == XRLT_JSON2XML_STRING && parent != NULL &&
                       parent->content != NULL) {
                ENCODE_ADD_STRING(parent->content);
            } else {
                ADD_CONST("null");
            }
        } else if (data->nodes.size() == 1 && xmlNodeIsText(data->nodes[0])) {
            if (data->type == XRLT_JSON2XML_NUMBER) {
                jsnode = json_decode((const char *)data->nodes[0]->content);

                RAISE_OUT_OF_MEMORY_IF_TRUE(jsnode == NULL);

                if (jsnode->tag != JSON_NUMBER) {
                    free(jsnode);

                    xrltTransformError(ctx, NULL, NULL,
                                       "Invalid JSON number\n");

                    return FALSE;
                }

                tmp = json_encode(jsnode);

                free(jsnode);

                RAISE_OUT_OF_MEMORY_IF_TRUE(tmp == NULL);

                if (xmlBufferAdd(*buf, (xmlChar *)tmp, -1) != 0) {
                    free(tmp);

                    ERROR_OUT_OF_MEMORY(ctx, NULL, NULL);

                    return FALSE;
                }

                free(tmp);
            } else if (data->type == XRLT_JSON2XML_BOOLEAN) {
                if (xmlStrEqual(data->nodes[0]->content,
                    (const xmlChar *)"true"))
                {
                    ADD_CONST("true");
                } else {
                    ADD_CONST("false");
                }
            } else {
                ENCODE_ADD_STRING(data->nodes[0]->content);
            }
        } else {
            std::multimap<std::string, xmlNodePtr>::iterator              iter;
            std::multimap<std::string, xmlNodePtr>::iterator              iter2;
            std::pair<std::multimap<std::string, xmlNodePtr>::iterator,
                      std::multimap<std::string, xmlNodePtr>::iterator>   range;
            xrltBool                                                      prev2;

            ADD_CONST("{");

            prev = FALSE;

            for (iter = data->named.begin(); iter != data->named.end();
                 iter = data->named.upper_bound(iter->first))
            {
                range = data->named.equal_range(iter->first);

                if (prev) {
                    ADD_CONST(", ");
                }

                ENCODE_ADD_STRING(iter->first.c_str());

                ADD_CONST(": ");

                if (std::distance(range.first, range.second) <= 1) {
                    if (!xrltXML2JSONStringify(ctx, iter->second, NULL, buf)) {
                        return FALSE;
                    }

                    prev = TRUE;
                } else {
                    ADD_CONST("[");

                    prev2 = FALSE;

                    for (iter2 = range.first; iter2 != range.second; iter2++) {
                        if (prev2) {
                            ADD_CONST(", ");
                        }

                        if (!xrltXML2JSONStringify(ctx, iter2->second,
                                                   NULL, buf))
                        {
                            return FALSE;
                        }

                        prev2 = TRUE;
                    }

                    ADD_CONST("]");

                    prev = TRUE;
                }
            }

            ADD_CONST("}");
        }
    }

    if (data != NULL) {
        delete data;
    }

    return TRUE;
}
