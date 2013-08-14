/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_XML2JS_H__
#define __XRLT_XML2JS_H__

#define V8_USE_UNSAFE_HANDLES

#include <libxml/xpath.h>
#include <v8.h>


void
        xrltXML2JSTemplateInit   (void);
void
        xrltXML2JSTemplateFree   (void);
v8::Handle<v8::Value>
        xrltXML2JSCreate         (xmlXPathObjectPtr value);


#endif /* __XRLT_XML2JS_H__ */
