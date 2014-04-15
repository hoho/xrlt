/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */

#include <vector>
#include <map>
#include <string>

#include <xrlt.h>
#include "json2xml.h"
#include "xml2js.h"
#include "xml2json.h"


using v8::Handle;
using v8::Array;
using v8::Value;
using v8::Isolate;
using v8::Persistent;
using v8::Object;
using v8::Local;
using v8::String;
using v8::Null;
using v8::True;
using v8::False;
using v8::Boolean;
using v8::Number;
using v8::Undefined;
using v8::PropertyCallbackInfo;
using v8::ObjectTemplate;
using v8::HandleScope;
using v8::WeakCallbackData;


// C++ structure to be associated with XML2JS cache.
struct xrltXML2JSCache {
    xrltXML2JSCache(Handle<Array> data) : index(0), data(data) {};
    bool get(void *key, Handle<Value> *value);
    void set(void *key, Handle<Value> value);

  private:
    uint32_t                     index;
    std::map<void *, uint32_t>   cache;
    Handle<Array>        data;
};


bool
xrltXML2JSCache::get(void *key, Handle<Value> *value)
{
    std::map<void *, uint32_t>::iterator i;

    i = cache.find(key);

    if (i != cache.end()) {
        *value = data->Get(i->second);
        return true;
    }

    return false;
}


void
xrltXML2JSCache::set(void *key, Handle<Value> value)
{
    cache.insert(std::pair<void *, uint32_t>(key, index));

    data->Set(index++, value);
}


// Deallocator for C++ data connected to XML2JS cache.
void
xrltXML2JSCacheWeakCallback(const WeakCallbackData<Object, xrltXML2JSCache>& data)
{
    xrltXML2JSCache *d = data.GetParameter();
    delete d;
    //obj->ClearWeak();
    //obj->Reset();
}


// Deallocator for C++ data connected to XML2JS JavaScript object.
void
xrltXML2JSWeakCallback(const WeakCallbackData<Object, xrltXML2JSONData>& data)
{
    xrltXML2JSONData *d = data.GetParameter();
    delete d;
    //obj->ClearWeak();
    //obj->Reset();
}


// XML2JS JavaScript object creator. Pretty much to optimize here.
Handle<Value>
xrltXML2JSCreateInternal(Isolate *isolate,
                         Persistent<ObjectTemplate> *xml2jsTemplate,
                         xmlNodePtr parent, xmlXPathObjectPtr val,
                         xrltXML2JSCache *cache)
{
    Handle<Value>   ret;

    if (cache != NULL) {
        if ((parent != NULL && cache->get(parent, &ret)) ||
            (val != NULL && cache->get(val, &ret)))
        {
            return ret;
        }
    }

    xrltXML2JSONData       *data = new xrltXML2JSONData(parent, val, cache);
    bool                    isXML2JS = false;

    if (data->type == XRLT_JSON2XML_ARRAY) {
        // If it's an array, return a JavaScript array of XML2JS objects.
        Local<Array>   arr = Array::New(isolate, data->nodes.size());
        uint32_t               i;

        for (i = 0; i < data->nodes.size(); i++) {
            arr->Set(i,
                     xrltXML2JSCreateInternal(isolate, xml2jsTemplate,
                                              data->nodes[i], NULL, cache));
        }
        ret = arr;
    } else {
        if (data->nodes.size() == 0) {
            // Empty XML node is null, unless xrl:type="object" attribute is
            // present.
            if (data->type == XRLT_JSON2XML_OBJECT) {
                ret = Object::New(isolate);
            } else if (data->type == XRLT_JSON2XML_STRING && parent != NULL &&
                       parent->content != NULL) {
                ret = String::NewFromUtf8(isolate, (const char *)parent->content);
            } else {
                ret = Null(isolate);
            }
        }
        else if (data->type == XRLT_JSON2XML_STRING && data->nodes.size() > 1)
        {
            std::string   str;
            uint32_t      i;

            for (i = 0; i < data->nodes.size(); i++) {
                str.append((char *)data->nodes[i]->content);
            }

            ret = String::NewFromUtf8(isolate, str.c_str());
        }
        else if (data->nodes.size() == 1 && xmlNodeIsText(data->nodes[0]))
        {
            // A single text node becomes a JavaScript string, unless parent's
            // xrl:type attribute says it should be a number or a boolean.
            if (data->type == XRLT_JSON2XML_NUMBER) {
                ret = String::NewFromUtf8(
                    isolate,
                    (const char *)data->nodes[0]->content
                )->ToNumber();
            } else if (data->type == XRLT_JSON2XML_BOOLEAN) {
                ret = xmlStrEqual(data->nodes[0]->content,
                                  (const xmlChar *)"true")
                    ?
                    True(isolate)
                    :
                    False(isolate);
            } else {
                ret = String::NewFromUtf8(isolate, (const char *)data->nodes[0]->content);
            }
        } else {
            // We have some more complex structure, create XML2JS object.
            Local<ObjectTemplate>   tpl = \
                Local<ObjectTemplate>::New(isolate, *xml2jsTemplate);
            Local<Object>           rddm(tpl->NewInstance());
            Persistent<Object>      wrapper(isolate, rddm);

            data->xml2jsTemplate = xml2jsTemplate;
            rddm->SetAlignedPointerInInternalField(0, data);

            wrapper.SetWeak(data, xrltXML2JSWeakCallback);
            wrapper.ClearAndLeak();

            ret = rddm;
            isXML2JS = true;
        }
    }

    // Free data if necessary.
    if (!isXML2JS) { delete data; }

    if (cache != NULL) {
        cache->set(parent, ret);
    }

    return ret;
}


// XML2JS JavaScript object creator. Pretty much to optimize here.
Handle<Value>
xrltXML2JSCreate(Isolate *isolate,
                 Persistent<ObjectTemplate> *xml2jsTemplate,
                 Persistent<ObjectTemplate> *xml2jsCacheTemplate,
                 xmlXPathObjectPtr value)
{
    xrltXML2JSCache *cache;

    switch (value->type) {
        case XPATH_STRING:
            return String::NewFromUtf8(isolate, (const char *)value->stringval);
        case XPATH_NUMBER:
            return Number::New(isolate, value->floatval);
        case XPATH_BOOLEAN:
            return Boolean::New(isolate, value->boolval ? true : false);
        case XPATH_NODESET:
            break;
        case XPATH_UNDEFINED:
        case XPATH_POINT:
        case XPATH_RANGE:
        case XPATH_LOCATIONSET:
        case XPATH_USERS:
        case XPATH_XSLT_TREE:
            return Undefined(isolate);
    }

    if (value->nodesetval == NULL || value->nodesetval->nodeTab == NULL ||
        value->nodesetval->nodeNr == 0)
    {
        return Undefined(isolate);
    }

    // We will store objects in JavaScript array to avoid freeing them by GC
    // before cache is freed.
    Local<Array>            data = Array::New(isolate);
    Local<ObjectTemplate>   tpl = \
        Local<ObjectTemplate>::New(isolate, *xml2jsCacheTemplate);
    Local<Object>           cacheobj(tpl->NewInstance());
    Persistent<Object>      wrapper(isolate, cacheobj);

    // Make a reference to cached data.
    cacheobj->Set(0, data);
    cache = new xrltXML2JSCache(data);
    cacheobj->SetAlignedPointerInInternalField(0, cache);

    wrapper.SetWeak(cache, xrltXML2JSCacheWeakCallback);
    wrapper.ClearAndLeak();

    if (value->nodesetval->nodeNr == 1 &&
        value->nodesetval->nodeTab[0]->type == XML_DOCUMENT_NODE)
    {
        return xrltXML2JSCreateInternal(isolate, xml2jsTemplate,
                                        value->nodesetval->nodeTab[0], NULL,
                                        cache);
    } else {
        return xrltXML2JSCreateInternal(isolate, xml2jsTemplate, NULL, value,
                                        cache);
    }
}


// Named properties interceptor for XML2JS JavaScript object.
void
xrltXML2JSGetProperty(Local<String> name,
                      const PropertyCallbackInfo<Value>& info)
{
    Isolate                                       *isolate;
    isolate = info.GetIsolate();

    xrltXML2JSONData                                  *data;
    data = (xrltXML2JSONData *)info.Holder()->
                                        GetAlignedPointerFromInternalField(0);

    std::string                                        key;
    String::Utf8Value                              keyarg(name);
    std::multimap<std::string, xmlNodePtr>::iterator   i;
    std::vector<xmlNodePtr>                            values;

    key.assign(*keyarg);

    i = data->named.find(key);

    if (i != data->named.end()) {
        for (; i != data->named.upper_bound(key); i++) {
            values.push_back(i->second);
        }

        if (values.size() == 1) {
            info.GetReturnValue().Set(xrltXML2JSCreateInternal(
                isolate,
                (Persistent<ObjectTemplate> *)data->xml2jsTemplate,
                values[0],
                NULL,
                static_cast<xrltXML2JSCache *>(data->cache)
            ));
            return;
        } else if (values.size() > 1) {
            uint32_t               index = 0;
            Local<Array>   ret = Array::New(isolate);

            for (index = 0; index < values.size(); index++) {
                ret->Set(
                    index, xrltXML2JSCreateInternal(
                        isolate,
                        (Persistent<ObjectTemplate> *)data->
                                xml2jsTemplate,
                        values[index],
                        NULL,
                        static_cast<xrltXML2JSCache *>(data->cache)
                    )
                );
            }
            info.GetReturnValue().Set(ret);
            return;
        }
    }

    info.GetReturnValue().Set(Undefined(isolate));
}


// Named properties enumerator for XML2JS JavaScript object.
void
xrltXML2JSEnumProperties(const PropertyCallbackInfo<Array>& info)
{
    xrltXML2JSONData                                  *data;
    data = (xrltXML2JSONData *)info.Holder()->
                                         GetAlignedPointerFromInternalField(0);

    HandleScope                                    scope(info.GetIsolate());

    Local<Array>                               ret = Array::New(info.GetIsolate());
    uint32_t                                           index = 0;
    std::multimap<std::string, xmlNodePtr>::iterator   i;

    for (i = data->named.begin(); i != data->named.end();
         i = data->named.upper_bound(i->first))
    {
        ret->Set(index++, String::NewFromUtf8(info.GetIsolate(), i->first.c_str()));
    }

    info.GetReturnValue().Set(ret);
}
