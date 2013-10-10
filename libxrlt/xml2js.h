/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#ifndef __XRLT_XML2JS_H__
#define __XRLT_XML2JS_H__

#include <libxml/xpath.h>
#include <v8.h>


v8::Handle<v8::Value>
        xrltXML2JSCreate(
            v8::Isolate *isolate,
            v8::Persistent<v8::ObjectTemplate> *xml2jsTemplate,
            v8::Persistent<v8::ObjectTemplate> *xml2jsCacheTemplate,
            xmlXPathObjectPtr value
        );


void
        xrltXML2JSGetProperty(
            v8::Local<v8::String> name,
            const v8::PropertyCallbackInfo<v8::Value>& info
        );

void
        xrltXML2JSEnumProperties(
            const v8::PropertyCallbackInfo<v8::Array>& info
        );

#endif /* __XRLT_XML2JS_H__ */
