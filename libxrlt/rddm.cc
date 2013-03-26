#include <libxml/tree.h>
#include <v8.h>
#include <vector>
#include <map>
#include <string>

#include <xrlt.h>
#include <rddm.h>


#define XRLT_EXTRACT_RDDM_OBJECT_DATA                                         \
    v8::Handle<v8::External> field =                                          \
        v8::Handle<v8::External>::Cast(info.Holder()->GetInternalField(0));   \
    xrltRDDMObjectData *data =                                                \
        static_cast<xrltRDDMObjectData *>(field->Value());


// RDDM cache template, needs xrltInitRDDMObjectTemplate to initialize.
v8::Persistent<v8::ObjectTemplate> xrltRDDMObjectCacheTemplate;
// RDDM object template, needs xrltInitRDDMObjectTemplate to initialize.
v8::Persistent<v8::ObjectTemplate> xrltRDDMObjectTemplate;


// C++ structure to be associated with RDDM cache.
struct xrltRDDMObjectCache {
    xrltRDDMObjectCache(v8::Handle<v8::Array> data) : index(0), data(data) {};
    bool get(xmlNodePtr node, v8::Handle<v8::Value> *value);
    void set(xmlNodePtr node, v8::Handle<v8::Value> value);

  private:
    uint32_t                         index;
    std::map<xmlNodePtr, uint32_t>   cache;
    v8::Handle<v8::Array>            data;
};


bool
xrltRDDMObjectCache::get(xmlNodePtr node, v8::Handle<v8::Value> *value)
{
    std::map<xmlNodePtr, uint32_t>::iterator i;

    i = cache.find(node);
    if (i != cache.end()) {
        *value = data->Get(i->second);
        return true;
    }

    return false;
}


void
xrltRDDMObjectCache::set(xmlNodePtr node, v8::Handle<v8::Value> value)
{
    cache.insert(std::pair<xmlNodePtr, uint32_t>(node, index));
    data->Set(index++, value);
}


// Deallocator for C++ data connected to RDDM cache.
void
xrltRDDMObjectCacheWeakCallback(v8::Persistent<v8::Value> obj, void *payload)
{
    xrltRDDMObjectCache *data = static_cast<xrltRDDMObjectCache *>(payload);
    delete data;
    obj.ClearWeak();
    obj.Dispose();
}


// Map to find XML node content type by xrl:type attribute.
struct xrltRDDMTypeMap {
    xrltRDDMTypeMap()
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

    xrltJSON2XMLType getType(xmlChar *str)
    {
        std::map<std::string, xrltJSON2XMLType>::iterator i;
        i = types.find((const char *)str);
        return i != types.end() ? i->second : XRLT_JSON2XML_UNKNOWN;
    }
  private:
    std::map<std::string, xrltJSON2XMLType> types;
};
xrltRDDMTypeMap XRLT_TYPE_MAP;


// C++ structure to be associated with RDDM JavaScript object.
struct xrltRDDMObjectData {
    xrltRDDMObjectData(xmlNodePtr parent, xrltRDDMObjectCache *cache);

    xrltJSON2XMLType                         type;
    std::vector<xmlNodePtr>                  nodes;
    std::multimap<std::string, xmlNodePtr>   named;
    xrltRDDMObjectCache                     *cache;
};


xrltRDDMObjectData::xrltRDDMObjectData(xmlNodePtr parent,
                                       xrltRDDMObjectCache *cache) : cache(cache)
{
    xmlNodePtr n = parent != NULL ? parent->children : NULL;

    // Create a list of nodes for RDDM object.
    while (n != NULL) {
        if (!xmlIsBlankNode(n)) {
            nodes.push_back(n);
        }
        n = n->next;
    }

    std::vector<xmlNodePtr>::iterator i;
    std::string key;

    // Create a multimap to find nodes by names.
    for(i = nodes.begin(); i != nodes.end(); i++) {
        n = *i;
        if (n != NULL) {
            key.assign((const char *)n->name);
            named.insert(std::pair<std::string, xmlNodePtr>(key, n));
        }
    }

    // Determine RDDM object type.
    xmlChar * typestr = NULL;

    if (parent != NULL) {
        typestr = xmlGetNsProp(parent, XRLT_JSON2XML_ATTR_TYPE, XRLT_NS);
        if (typestr != NULL) {
            type = XRLT_TYPE_MAP.getType(typestr);
            xmlFree(typestr);
        }
    }

    if ((typestr == NULL || type == XRLT_JSON2XML_UNKNOWN) &&
        (named.size() > 1 && nodes.size() == named.count(key)))
    {
        type = XRLT_JSON2XML_ARRAY;
    }
}


// Deallocator for C++ data connected to RDDM JavaScript object.
void
xrltRDDMObjectWeakCallback(v8::Persistent<v8::Value> obj, void *payload)
{
    xrltRDDMObjectData *data = static_cast<xrltRDDMObjectData *>(payload);
    delete data;
    obj.ClearWeak();
    obj.Dispose();
}


// RDDM JavaScript object creator. Pretty much to optimize here.
v8::Handle<v8::Value>
xrltCreateRDDMObject(xmlNodePtr parent, xrltRDDMObjectCache *cache)
{
    v8::Handle<v8::Value>   ret;

    if (cache != NULL && cache->get(parent, &ret)) {
        return ret;
    }

    xrltRDDMObjectData     *data = new xrltRDDMObjectData(parent, cache);
    bool                    isRDDM = false;

    if (data->type == XRLT_JSON2XML_ARRAY) {
        // If it's an array, return a JavaScript array of RDDM objects.
        v8::Local<v8::Array>   arr = v8::Array::New(data->nodes.size());
        uint32_t               i;

        for (i = 0; i < data->nodes.size(); i++) {
            arr->Set(i, xrltCreateRDDMObject(data->nodes[i], cache));
        }
        ret = arr;
    } else {
        if (data->nodes.size() == 0) {
            // Empty XML node is null, unless xrl:type="object" attribute is
            // present.
            if (data->type == XRLT_JSON2XML_OBJECT) {
                ret = v8::Object::New();
            } else {
                ret = v8::Null();
            }
        } else if (data->nodes.size() == 1 && xmlNodeIsText(data->nodes[0])) {
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
            // We have some more complex structure, create RDDM object.
            v8::Persistent<v8::Object> rddm;

            rddm = v8::Persistent<v8::Object>::New(
                xrltRDDMObjectTemplate->NewInstance()
            );

            rddm.MakeWeak(data, xrltRDDMObjectWeakCallback);

            rddm->SetInternalField(0, v8::External::New(data));

            ret = rddm;
            isRDDM = true;
        }
    }

    // Free data if necessary.
    if (!isRDDM) { delete data; }

    if (cache != NULL) {
        cache->set(parent, ret);
    }

    return ret;
}


// RDDM JavaScript object creator. Pretty much to optimize here.
v8::Handle<v8::Value>
xrltCreateRDDMObjectWithCache(xmlNodePtr parent)
{
    xrltRDDMObjectCache         *cache;
    v8::Persistent<v8::Object>   cacheobj;

    // We will store objects in JavaScript array avoid freeing them by GC
    // before cache is freed.
    v8::Local<v8::Array>         data = v8::Array::New();

    cacheobj = v8::Persistent<v8::Object>::New(
        xrltRDDMObjectCacheTemplate->NewInstance()
    );

    // Make a reference to cached data.
    cacheobj->Set(0, data);

    cache = new xrltRDDMObjectCache(data);

    cacheobj.MakeWeak(cache, xrltRDDMObjectCacheWeakCallback);
    cacheobj->SetInternalField(0, v8::External::New(cache));

    return xrltCreateRDDMObject(parent, cache);
}


// Named properties interceptor for RDDM JavaScript object.
v8::Handle<v8::Value>
xrltGetRDDMObjectProperty(v8::Local<v8::String> name,
                          const v8::AccessorInfo& info)
{
    XRLT_EXTRACT_RDDM_OBJECT_DATA;

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
            return xrltCreateRDDMObject(values[0], data->cache);
        } else if (values.size() > 1) {
            uint32_t               index = 0;
            v8::Local<v8::Array>   ret = v8::Array::New();

            for (index = 0; index < values.size(); index++) {
                ret->Set(index,
                         xrltCreateRDDMObject(values[index], data->cache));
            }
            return ret;
        }
    }

    return v8::Undefined();
}


// Named properties enumerator for RDDM JavaScript object.
v8::Handle<v8::Array>
xrltEnumRDDMObjectProperties(const v8::AccessorInfo& info)
{
    XRLT_EXTRACT_RDDM_OBJECT_DATA;

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


// This function must be called to initialize RDDM object template.
void
xrltInitRDDMObjectTemplate(void)
{
    xrltRDDMObjectCacheTemplate = \
        v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    xrltRDDMObjectCacheTemplate->SetInternalFieldCount(1);

    xrltRDDMObjectTemplate = \
        v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    xrltRDDMObjectTemplate->SetInternalFieldCount(1);

    xrltRDDMObjectTemplate->SetNamedPropertyHandler(
        xrltGetRDDMObjectProperty, 0, 0, 0, xrltEnumRDDMObjectProperties
    );
}
