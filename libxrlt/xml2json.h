#ifndef __XRLT_XML2JSON_H__
#define __XRLT_XML2JSON_H__


#include <libxml/tree.h>
#include <v8.h>


void
        xrltXML2JSONTemplateInit   (void);
v8::Handle<v8::Value>
        xrltXML2JSONCreate         (xmlNodePtr parent);


#endif /* __XRLT_XML2JSON_H__ */
