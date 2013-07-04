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


// XML2JS cache template, needs xrltXML2JSTemplateInit to initialize.
v8::Persistent<v8::ObjectTemplate> xrltXML2JSCacheTemplate;

// XML2JS object template, needs xrltXML2JSTemplateInit to initialize.
v8::Persistent<v8::ObjectTemplate> xrltXML2JSTemplate;


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
xrltXML2JSCacheWeakCallback(v8::Persistent<v8::Value> obj, void *payload)
{
    xrltXML2JSCache *data = static_cast<xrltXML2JSCache *>(payload);
    delete data;
    obj.ClearWeak();
    obj.Dispose();
}


// Deallocator for C++ data connected to XML2JS JavaScript object.
void
xrltXML2JSWeakCallback(v8::Persistent<v8::Value> obj, void *payload)
{
    xrltXML2JSONData *data = static_cast<xrltXML2JSONData *>(payload);
    delete data;
    obj.ClearWeak();
    obj.Dispose();
}


// XML2JS JavaScript object creator. Pretty much to optimize here.
v8::Handle<v8::Value>
xrltXML2JSCreateInternal(xmlNodePtr parent, xmlXPathObjectPtr val,
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
                     xrltXML2JSCreateInternal(data->nodes[i], NULL, cache));
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
            v8::Persistent<v8::Object> rddm;

            rddm = v8::Persistent<v8::Object>::New(
                xrltXML2JSTemplate->NewInstance()
            );

            rddm.MakeWeak(data, xrltXML2JSWeakCallback);

            rddm->SetAlignedPointerInInternalField(0, data);

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
xrltXML2JSCreate(xmlXPathObjectPtr value)
{
    xrltXML2JSCache *cache;
    v8::Persistent<v8::Object>   cacheobj;

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
    v8::Local<v8::Array>         data = v8::Array::New();

    cacheobj = v8::Persistent<v8::Object>::New(
        xrltXML2JSCacheTemplate->NewInstance()
    );

    // Make a reference to cached data.
    cacheobj->Set(0, data);

    cache = new xrltXML2JSCache(data);

    cacheobj.MakeWeak(cache, xrltXML2JSCacheWeakCallback);
    cacheobj->SetAlignedPointerInInternalField(0, cache);

    if (value->nodesetval->nodeNr == 1 &&
        value->nodesetval->nodeTab[0]->type == XML_DOCUMENT_NODE)
    {
        return xrltXML2JSCreateInternal(value->nodesetval->nodeTab[0], NULL,
                                        cache);
    } else {
        return xrltXML2JSCreateInternal(NULL, value, cache);
    }
}


// Named properties interceptor for XML2JS JavaScript object.
v8::Handle<v8::Value>
xrltXML2JSGetProperty(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
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
            return xrltXML2JSCreateInternal(
                values[0], NULL, static_cast<xrltXML2JSCache *>(data->cache)
            );
        } else if (values.size() > 1) {
            uint32_t               index = 0;
            v8::Local<v8::Array>   ret = v8::Array::New();

            for (index = 0; index < values.size(); index++) {
                ret->Set(
                    index, xrltXML2JSCreateInternal(
                        values[index], NULL,
                        static_cast<xrltXML2JSCache *>(data->cache)
                    )
                );
            }
            return ret;
        }
    }

    return v8::Undefined();
}


// Named properties enumerator for XML2JS JavaScript object.
v8::Handle<v8::Array>
xrltXML2JSEnumProperties(const v8::AccessorInfo& info)
{
    xrltXML2JSONData                                  *data;
    data = (xrltXML2JSONData *)info.Holder()->
                                         GetAlignedPointerFromInternalField(0);

    v8::HandleScope                                    scope;

    v8::Local<v8::Array>                               ret = v8::Array::New();
    uint32_t                                           index = 0;
    std::multimap<std::string, xmlNodePtr>::iterator   i;

    for (i = data->named.begin(); i != data->named.end();
         i = data->named.upper_bound(i->first))
    {
        ret->Set(index++, v8::String::New(i->first.c_str()));
    }

    return scope.Close(ret);
}


// This function must be called to initialize XML2JS object template.
void
xrltXML2JSTemplateInit(void)
{
    xrltXML2JSCacheTemplate = \
        v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    xrltXML2JSCacheTemplate->SetInternalFieldCount(1);

    xrltXML2JSTemplate = \
        v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    xrltXML2JSTemplate->SetInternalFieldCount(1);

    xrltXML2JSTemplate->SetNamedPropertyHandler(
        xrltXML2JSGetProperty, 0, 0, 0, xrltXML2JSEnumProperties
    );
}


void
xrltXML2JSTemplateFree(void)
{
    xrltXML2JSCacheTemplate.Dispose();
    xrltXML2JSTemplate.Dispose();
}
