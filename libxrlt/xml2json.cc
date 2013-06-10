#include "xml2json.h"
#include <sstream>


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


xrltBool
xrltXML2JSONStringify(xmlNodePtr parent, xmlXPathObjectPtr val,
                      xmlBufferPtr *buf)
{
    // TODO: Check if response values are valid JSON values.
    //       Check buffer operations success.
    xrltXML2JSONData    *data = NULL;
    size_t               i;
    xrltBool             prev;

    if (buf == NULL) { return FALSE; }

    if (*buf == NULL) {
        *buf = xmlBufferCreate();
    }

    if (val != NULL) {
        switch (val->type) {
            case XPATH_STRING:
                xmlBufferAdd(*buf, (const xmlChar *)"\"", 1);
                xmlBufferAdd(*buf, val->stringval, xmlStrlen(val->stringval));
                xmlBufferAdd(*buf, (const xmlChar *)"\"", 1);
                return TRUE;

            case XPATH_NUMBER:
                {
                    std::ostringstream   fbuf;
                    std::string          tmp;
                    fbuf << val->floatval;
                    tmp = fbuf.str();
                    xmlBufferAdd(*buf, (xmlChar *)tmp.c_str(), tmp.length());
                }
                return TRUE;

            case XPATH_BOOLEAN:
                if (val->boolval) {
                    xmlBufferAdd(*buf, (const xmlChar *)"true", 4);
                } else {
                    xmlBufferAdd(*buf, (const xmlChar *)"false", 5);
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
            return FALSE;
        }

        if (val->nodesetval->nodeNr == 1 &&
            val->nodesetval->nodeTab[0]->type == XML_DOCUMENT_NODE)
        {
            data = new xrltXML2JSONData(val->nodesetval->nodeTab[0],
                                        NULL, NULL);
        } else {
            data = new xrltXML2JSONData(NULL, val, NULL);
        }
    } else {
        data = new xrltXML2JSONData(parent, NULL, NULL);
    }

    if (data->type == XRLT_JSON2XML_ARRAY) {
        xmlBufferAdd(*buf, (const xmlChar *)"[", 1);

        prev = FALSE;

        for (i = 0; i < data->nodes.size(); i++) {
            if (prev) {
                xmlBufferAdd(*buf, (const xmlChar *)", ", 2);
            }

            prev |= xrltXML2JSONStringify(data->nodes[i], NULL, buf);
        }

        xmlBufferAdd(*buf, (const xmlChar *)"]", 1);
    } else {
        if (data->nodes.size() == 0) {
            if (data->type == XRLT_JSON2XML_OBJECT) {
                xmlBufferAdd(*buf, (const xmlChar *)"{}", 2);
            } else if (data->type == XRLT_JSON2XML_STRING && parent != NULL &&
                       parent->content != NULL) {
                xmlBufferAdd(*buf, (const xmlChar *)"\"", 1);
                xmlBufferAdd(*buf, parent->content, xmlStrlen(parent->content));
                xmlBufferAdd(*buf, (const xmlChar *)"\"", 1);
            } else {
                xmlBufferAdd(*buf, (const xmlChar *)"null", 4);
            }
        } else if (data->nodes.size() == 1 && xmlNodeIsText(data->nodes[0])) {
            if (data->type == XRLT_JSON2XML_NUMBER) {
                xmlBufferAdd(*buf, parent->content, xmlStrlen(parent->content));
            } else if (data->type == XRLT_JSON2XML_BOOLEAN) {
                if (xmlStrEqual(data->nodes[0]->content,
                    (const xmlChar *)"true"))
                {
                    xmlBufferAdd(*buf, (const xmlChar *)"true", 4);
                } else {
                    xmlBufferAdd(*buf, (const xmlChar *)"false", 5);
                }
            } else {
                xmlBufferAdd(*buf, (const xmlChar *)"\"", 1);
                xmlBufferAdd(*buf, data->nodes[0]->content,
                             xmlStrlen(data->nodes[0]->content));
                xmlBufferAdd(*buf, (const xmlChar *)"\"", 1);
            }
        } else {
            std::multimap<std::string, xmlNodePtr>::iterator              iter;
            std::multimap<std::string, xmlNodePtr>::iterator              iter2;
            std::pair<std::multimap<std::string, xmlNodePtr>::iterator,
                      std::multimap<std::string, xmlNodePtr>::iterator>   range;
            xrltBool                                                      prev2;

            xmlBufferAdd(*buf, (const xmlChar *)"{", 1);

            prev = FALSE;

            for (iter = data->named.begin(); iter != data->named.end();
                 iter = data->named.upper_bound(iter->first))
            {
                range = data->named.equal_range(iter->first);

                if (prev) {
                    xmlBufferAdd(*buf, (const xmlChar *)", ", 2);
                }

                xmlBufferAdd(*buf, (const xmlChar *)"\"", 1);
                xmlBufferAdd(*buf, (xmlChar *)iter->first.c_str(),
                             iter->first.length());
                xmlBufferAdd(*buf, (const xmlChar *)"\": ", 3);

                if (std::distance(range.first, range.second) <= 1) {
                    prev |= xrltXML2JSONStringify(iter->second, NULL, buf);
                } else {
                    xmlBufferAdd(*buf, (const xmlChar *)"[", 1);

                    prev2 = FALSE;

                    for (iter2 = range.first; iter2 != range.second; iter2++) {
                        if (prev2) {
                            xmlBufferAdd(*buf, (const xmlChar *)", ", 2);
                        }

                        prev2 |=
                            xrltXML2JSONStringify(iter2->second, NULL, buf);
                    }

                    xmlBufferAdd(*buf, (const xmlChar *)"]", 1);

                    prev = TRUE;
                }

                i++;
            }

            xmlBufferAdd(*buf, (const xmlChar *)"}", 1);
        }
    }

    if (data != NULL) {
        delete data;
    }

    return TRUE;
}
