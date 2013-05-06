#include "transform.h"
#include "querystring.h"


xrltQueryStringParserPtr
xrltQueryStringParserInit(xmlNodePtr parent)
{
    xrltQueryStringParserPtr   ret;

    if (parent == NULL) { return NULL; }

    XRLT_MALLOC(NULL, NULL, NULL, ret, xrltQueryStringParserPtr,
                sizeof(xrltQueryStringParser), NULL);

    ret->parent = parent;

    return ret;
}


xrltBool
xrltQueryStringParserFeed(xrltQueryStringParserPtr parser,
                          char *data, size_t len, xrltBool last)
{
    if (parser == NULL) { return FALSE; }

    char        *cur, *tmp;
    size_t       i, pos, sz;
    xmlNodePtr   node = NULL;
    xmlChar     *n;

    if (parser->isVal) {
        cur = parser->val;
        pos = parser->valLen;
        sz = parser->valSize;
    } else {
        cur = parser->name;
        pos = parser->nameLen;
        sz = parser->nameSize;
    }

    for (i = 0; i <= len; i++) {
        if (i == len || data[i] == '&' || data[i] == ';') {
            if (i == len && !last) {
                // We're iterating to <= len to process the last parameter.
                break;
            }


            if (parser->isVal) {
                parser->valLen = pos;
            } else {
                parser->nameLen = pos;
            }

            if (parser->nameLen > 0 || parser->valLen > 0) {
                if (parser->nameLen == 0) {
                    n = xrltURLDecode(parser->val, parser->valLen);

                    node = xmlNewText(n);

                    xrltFree(n);
                } else {
                    n = xrltURLDecode(parser->name, parser->nameLen);

                    node = xmlNewNode(NULL, n);

                    xrltFree(n);

                    if (node != NULL && parser->valLen > 0) {
                        n = xrltURLDecode(parser->val, parser->valLen);

                        xmlNodeAddContent(node, n);

                        xrltFree(n);
                    }
                }

                if (node == NULL) {
                    return FALSE;
                }

                if (xmlAddChild(parser->parent, node) == NULL) {
                    xmlFreeNode(node);

                    return FALSE;
                }
            }

            parser->isVal = FALSE;

            parser->nameLen = 0;
            parser->valLen = 0;

            pos = 0;
            sz = parser->nameSize;
            cur = parser->name;
        } else if (data[i] == '=') {
            if (!parser->isVal) {
                parser->nameLen = pos;

                cur = parser->val;
                pos = parser->valLen;
                sz = parser->valSize;

                parser->isVal = TRUE;
            }
        } else {
            if (pos >= sz) {
                sz += 20;

                tmp = (char *)xrltRealloc(cur, sz);
                if (tmp == NULL) {
                    return FALSE;
                }

                cur = tmp;

                if (parser->isVal) {
                    parser->val = cur;
                    parser->valLen = pos;
                    parser->valSize = sz;
                } else {
                    parser->name = cur;
                    parser->nameLen = pos;
                    parser->nameSize = sz;
                }
            }

            cur[pos++] = data[i];
        }
    }

    if (parser->isVal) {
        parser->valLen = pos;
    } else {
        parser->nameLen = pos;
    }

    return TRUE;
}


void
xrltQueryStringParserFree(xrltQueryStringParserPtr parser)
{
    if (parser != NULL) {
        if (parser->name != NULL) { xrltFree(parser->name); }
        if (parser->val != NULL) { xrltFree(parser->val); }
        xrltFree(parser);
    }
}
