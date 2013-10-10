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


// C++ structure to be associated with XML2JS cache.
struct xrltXML2JSCache {
    xrltXML2JSCache(v8::Handle<v8::Array> data) : index(0), data(data) {};
    bool get(void *key, v8::Handle<v8::Value> *value);
    void set(void *key, v8::Handle<v8::Value> value);

  private:
    uint32_t                     index;
    std::map<void *, uint32_t>   cache;
    v8::Handle<v8::Array>        data;
};


bool
xrltXML2JSCache::get(void *key, v8::Handle<v8::Value> *value)
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
xrltXML2JSCache::set(void *key, v8::Handle<v8::Value> value)
{
    cache.insert(std::pair<void *, uint32_t>(key, index));

    data->Set(index++, value);
}


// Deallocator for C++ data connected to XML2JS cache.
void
xrltXML2JSCacheWeakCallback(v8::Isolate *isolate,
                            v8::Persistent<v8::Object> *obj,
                            xrltXML2JSCache *data)
{
    delete data;
    obj->ClearWeak();
    obj->Dispose();
}


// Deallocator for C++ data connected to XML2JS JavaScript object.
void
xrltXML2JSWeakCallback(v8::Isolate *isolate,
                       v8::Persistent<v8::Object> *obj,
                       xrltXML2JSONData *data)
{
    delete data;
    obj->ClearWeak();
    obj->Dispose();
}


// XML2JS JavaScript object creator. Pretty much to optimize here.
v8::Handle<v8::Value>
xrltXML2JSCreateInternal(v8::Isolate *isolate,
                         v8::Persistent<v8::ObjectTemplate> *xml2jsTemplate,
                         xmlNodePtr parent, xmlXPathObjectPtr val,
                         xrltXML2JSCache *cache)
{
    v8::Handle<v8::Value>   ret;

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
        v8::Local<v8::Array>   arr = v8::Array::New(data->nodes.size());
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
                ret = v8::Object::New();
            } else if (data->type == XRLT_JSON2XML_STRING && parent != NULL &&
                       parent->content != NULL) {
                ret = v8::String::New((const char *)parent->content);
            } else {
                ret = v8::Null();
            }
        }
        else if (data->type == XRLT_JSON2XML_STRING && data->nodes.size() > 1)
        {
            std::string   str;
            uint32_t      i;

            for (i = 0; i < data->nodes.size(); i++) {
                str.append((char *)data->nodes[i]->content);
            }

            ret = v8::String::New(str.c_str());
        }
        else if (data->nodes.size() == 1 && xmlNodeIsText(data->nodes[0]))
        {
            // A single text node becomes a JavaScript string, unless parent's
            // xrl:type attribute says it should be a number or a boolean.
            if (data->type == XRLT_JSON2XML_NUMBER) {
                ret = v8::String::New(
                    (const char *)data->nodes[0]->content
                )->ToNumber();
            } else if (data->type == XRLT_JSON2XML_BOOLEAN) {
                ret = xmlStrEqual(data->nodes[0]->content,
                                  (const xmlChar *)"true")
                    ?
                    v8::True()
                    :
                    v8::False();
            } else {
                ret = v8::String::New((const char *)data->nodes[0]->content);
            }
        } else {
            // We have some more complex structure, create XML2JS object.
            v8::Local<v8::ObjectTemplate>   tpl = \
                v8::Local<v8::ObjectTemplate>::New(isolate, *xml2jsTemplate);
            v8::Local<v8::Object>           rddm(tpl->NewInstance());
            v8::Persistent<v8::Object>      wrapper(isolate, rddm);

            data->xml2jsTemplate = xml2jsTemplate;
            rddm->SetAlignedPointerInInternalField(0, data);

            wrapper.MakeWeak(data, xrltXML2JSWeakCallback);
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
v8::Handle<v8::Value>
xrltXML2JSCreate(v8::Isolate *isolate,
                 v8::Persistent<v8::ObjectTemplate> *xml2jsTemplate,
                 v8::Persistent<v8::ObjectTemplate> *xml2jsCacheTemplate,
                 xmlXPathObjectPtr value)
{
    xrltXML2JSCache *cache;

    switch (value->type) {
        case XPATH_STRING:
            return v8::String::New((const char *)value->stringval);
        case XPATH_NUMBER:
            return v8::Number::New(value->floatval);
        case XPATH_BOOLEAN:
            return v8::Boolean::New(value->boolval ? true : false);
        case XPATH_NODESET:
            break;
        case XPATH_UNDEFINED:
        case XPATH_POINT:
        case XPATH_RANGE:
        case XPATH_LOCATIONSET:
        case XPATH_USERS:
        case XPATH_XSLT_TREE:
            return v8::Undefined();
    }

    if (value->nodesetval == NULL || value->nodesetval->nodeTab == NULL ||
        value->nodesetval->nodeNr == 0)
    {
        return v8::Undefined();
    }

    // We will store objects in JavaScript array to avoid freeing them by GC
    // before cache is freed.
    v8::Local<v8::Array>            data = v8::Array::New();
    v8::Local<v8::ObjectTemplate>   tpl = \
        v8::Local<v8::ObjectTemplate>::New(isolate, *xml2jsCacheTemplate);
    v8::Local<v8::Object>           cacheobj(tpl->NewInstance());
    v8::Persistent<v8::Object>      wrapper(isolate, cacheobj);

    // Make a reference to cached data.
    cacheobj->Set(0, data);
    cache = new xrltXML2JSCache(data);
    cacheobj->SetAlignedPointerInInternalField(0, cache);

    wrapper.MakeWeak(cache, xrltXML2JSCacheWeakCallback);
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
xrltXML2JSGetProperty(v8::Local<v8::String> name,
                      const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Isolate                                       *isolate;
    isolate = info.GetIsolate();

    xrltXML2JSONData                                  *data;
    data = (xrltXML2JSONData *)info.Holder()->
                                        GetAlignedPointerFromInternalField(0);

    std::string                                        key;
    v8::String::Utf8Value                              keyarg(name);
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
                (v8::Persistent<v8::ObjectTemplate> *)data->xml2jsTemplate,
                values[0],
                NULL,
                static_cast<xrltXML2JSCache *>(data->cache)
            ));
            return;
        } else if (values.size() > 1) {
            uint32_t               index = 0;
            v8::Local<v8::Array>   ret = v8::Array::New();

            for (index = 0; index < values.size(); index++) {
                ret->Set(
                    index, xrltXML2JSCreateInternal(
                        isolate,
                        (v8::Persistent<v8::ObjectTemplate> *)data->
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

    info.GetReturnValue().Set(v8::Undefined());
}


// Named properties enumerator for XML2JS JavaScript object.
void
xrltXML2JSEnumProperties(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    xrltXML2JSONData                                  *data;
    data = (xrltXML2JSONData *)info.Holder()->
                                         GetAlignedPointerFromInternalField(0);

    v8::HandleScope                                    scope(info.GetIsolate());

    v8::Local<v8::Array>                               ret = v8::Array::New();
    uint32_t                                           index = 0;
    std::multimap<std::string, xmlNodePtr>::iterator   i;

    for (i = data->named.begin(); i != data->named.end();
         i = data->named.upper_bound(i->first))
    {
        ret->Set(index++, v8::String::New(i->first.c_str()));
    }

    info.GetReturnValue().Set(ret);
}
