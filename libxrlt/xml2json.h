/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_XML2JSON_H__
#define __XRLT_XML2JSON_H__


#include <vector>
#include <map>
#include <string>
#include <libxml/xpath.h>

#include "xrlt.h"
#include "transform.h"
#include "json2xml.h"


// Map to find XML node content type by xrl:type attribute.
struct xrltXML2JSONTypeMap {
    xrltXML2JSONTypeMap();
    xrltJSON2XMLType getType(xmlChar *str);

  private:
    std::map<std::string, xrltJSON2XMLType> types;
};


// C++ structure to be associated with XML2JSON JavaScript object.
// This one is also used in json-stringify transformations.
struct xrltXML2JSONData {
    xrltXML2JSONData(xmlNodePtr parent, xmlXPathObjectPtr val,
                     void *payload);

    xrltJSON2XMLType                         type;
    std::vector<xmlNodePtr>                  nodes;
    std::multimap<std::string, xmlNodePtr>   named;

    void                                    *cache; // For creating JavaScript
                                                    // objects.
    void                                    *xml2jsTemplate;

};


xrltBool
        xrltXML2JSONStringify   (xrltContextPtr ctx, xmlNodePtr parent,
                                 xmlXPathObjectPtr val, xmlBufferPtr *buf);


#endif /* __XRLT_XML2JSON_H__ */
