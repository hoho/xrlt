#ifndef __XRLT_XML2JSON_H__
#define __XRLT_XML2JSON_H__


#include <libxml/xpath.h>
#include <v8.h>


void
        xrltXML2JSONTemplateInit   (void);
void
        xrltXML2JSONTemplateFree   (void);
v8::Handle<v8::Value>
        xrltXML2JSONCreate         (xmlXPathObjectPtr value);


#endif /* __XRLT_XML2JSON_H__ */
