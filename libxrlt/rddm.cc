#include <libxml/tree.h>
#include <v8.h>
#include <vector>
#include <map>
#include <string>

#include "xrlt.h"
#include "rddm.h"


#define EXTRACT_RDDM_OBJECT_DATA                                              \
    v8::Handle<v8::External> field =                                          \
        v8::Handle<v8::External>::Cast(info.Holder()->GetInternalField(0));   \
    RDDMObjectData *data = static_cast<RDDMObjectData *>(field->Value());


// RDDM object template, needs InitRDDMObjectTemplate to initialize.
v8::Persistent<v8::ObjectTemplate> RDDMObjectTemplate;


// Map to find XML node content type by xrl:type attribute.
struct xrltRDDMTypeMap {
    xrltRDDMTypeMap() {
        types.insert(std::pair<std::string, xrltJSON2XMLType>((const char *)XRLT_JSON2XML_ATTR_TYPE_STRING, XRLT_JSON2XML_STRING));
        types.insert(std::pair<std::string, xrltJSON2XMLType>((const char *)XRLT_JSON2XML_ATTR_TYPE_NUMBER, XRLT_JSON2XML_NUMBER));
        types.insert(std::pair<std::string, xrltJSON2XMLType>((const char *)XRLT_JSON2XML_ATTR_TYPE_BOOLEAN, XRLT_JSON2XML_BOOLEAN));
        types.insert(std::pair<std::string, xrltJSON2XMLType>((const char *)XRLT_JSON2XML_ATTR_TYPE_OBJECT, XRLT_JSON2XML_OBJECT));
        types.insert(std::pair<std::string, xrltJSON2XMLType>((const char *)XRLT_JSON2XML_ATTR_TYPE_ARRAY, XRLT_JSON2XML_ARRAY));
        types.insert(std::pair<std::string, xrltJSON2XMLType>((const char *)XRLT_JSON2XML_ATTR_TYPE_NULL, XRLT_JSON2XML_NULL));
    }

    xrltJSON2XMLType getType(xmlChar * str) {
        std::map<std::string, xrltJSON2XMLType>::iterator i;
        i = types.find((const char *)str);
        return i != types.end() ? i->second : XRLT_JSON2XML_UNKNOWN;
    }
  private:
    std::map<std::string, xrltJSON2XMLType> types;
};
xrltRDDMTypeMap XRLT_TYPE_MAP;


// C++ structure to be associated with RDDM JavaScript object.
struct RDDMObjectData {
    RDDMObjectData(xmlNodePtr first);
    xrltJSON2XMLType type;
    std::vector<xmlNodePtr> nodes;
    std::multimap<std::string, xmlNodePtr> named;
};

RDDMObjectData::RDDMObjectData(xmlNodePtr parent) {
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
        (named.size() > 1 && nodes.size() == named.count(key))) {
        type = XRLT_JSON2XML_ARRAY;
    }
}


// Deallocator for C++ data connected to RDDM JavaScript object.
void RDDMObjectWeakCallback(v8::Persistent<v8::Value> obj, void *payload) {
    RDDMObjectData *data = static_cast<RDDMObjectData *>(payload);
    delete data;
    obj.ClearWeak();
    obj.Dispose();
}


// RDDM JavaScript object creator. Pretty much to optimize here.
v8::Handle<v8::Value> CreateRDDMObject(xmlNodePtr parent) {
    RDDMObjectData *data = new RDDMObjectData(parent);
    v8::Handle<v8::Value> ret;
    bool isRDDM = false;

    if (data->type == XRLT_JSON2XML_ARRAY) {
        // If it's an array, return a JavaScript array of RDDM objects.
        v8::Local<v8::Array> arr = v8::Array::New(data->nodes.size());
        uint32_t i;
        for (i = 0; i < data->nodes.size(); i++) {
            arr->Set(i, CreateRDDMObject(data->nodes[i]));
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
                ret = v8::String::New((const char *)data->nodes[0]->content)->ToNumber();
            } else if (data->type == XRLT_JSON2XML_BOOLEAN) {
                ret = xmlStrEqual(data->nodes[0]->content, (const xmlChar *)"true") ? v8::True() : v8::False();
            } else {
                ret = v8::String::New((const char *)data->nodes[0]->content);
            }
        } else {
            // We have some more complex structure, create RDDM object.
            v8::Persistent<v8::Object> rddm;
            rddm = v8::Persistent<v8::Object>::New(RDDMObjectTemplate->NewInstance());
            rddm.MakeWeak(data, RDDMObjectWeakCallback);
            rddm->SetInternalField(0, v8::External::New(data));
            ret = rddm;
            isRDDM = true;
        }
    }

    // Free data if necessary.
    if (!isRDDM) { delete data; }

    return ret;
}


// Named properties interceptor for RDDM JavaScript object.
v8::Handle<v8::Value> GetRDDMObjectProperty(v8::Local<v8::String> name,
                                            const v8::AccessorInfo& info) {
    EXTRACT_RDDM_OBJECT_DATA;

    std::string key;
    v8::String::Utf8Value keyarg(name);
    key.assign(*keyarg);

    std::multimap<std::string, xmlNodePtr>::iterator i;
    std::vector<xmlNodePtr> values;

    i = data->named.find(key);
    if (i != data->named.end()) {
        for (; i != data->named.upper_bound(key); i++) {
            values.push_back(i->second);
        }

        if (values.size() == 1) {
            return CreateRDDMObject(values[0]);
        } else if (values.size() > 1) {
            v8::Local<v8::Array> ret = v8::Array::New();
            uint32_t index = 0;
            for (index = 0; index < values.size(); index++) {
                ret->Set(index, CreateRDDMObject(values[index]));
            }
            return ret;
        }
    }

    return v8::Undefined();
}


// Named properties enumerator for RDDM JavaScript object.
v8::Handle<v8::Array> EnumRDDMObjectProperties(const v8::AccessorInfo& info) {
    EXTRACT_RDDM_OBJECT_DATA;

    v8::HandleScope scope;

    v8::Local<v8::Array> ret = v8::Array::New();
    uint32_t index = 0;

    std::multimap<std::string, xmlNodePtr>::iterator i;

    for (i = data->named.begin(); i != data->named.end(); i = data->named.upper_bound(i->first)) {
        ret->Set(index++, v8::String::New(i->first.c_str()));
    }

    return scope.Close(ret);
}


// This function must be called to initialize RDDM object template.
void InitRDDMObjectTemplate(void) {
    RDDMObjectTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    RDDMObjectTemplate->SetInternalFieldCount(1);
    RDDMObjectTemplate->SetNamedPropertyHandler(GetRDDMObjectProperty, 0, 0, 0, EnumRDDMObjectProperties);
}
