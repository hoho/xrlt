#include <v8.h>
#include <string>

#include "xrlterror.h"
#include "js.h"
#include "xml2json.h"
#include "json2xml.h"
#include "transform.h"


typedef struct {
    v8::Persistent<v8::ObjectTemplate>   globalTemplate;
    v8::Persistent<v8::Object>           global;
    v8::Persistent<v8::Object>           functions;
    v8::Persistent<v8::Context>          context;
} xrltJSContextPrivate;


v8::Persistent<v8::FunctionTemplate> xrltDeferredConstructor;


v8::Handle<v8::Value>
xrltDeferredThen(const v8::Arguments& args) {
    fprintf(stderr, "then\n");
    return v8::True();
}


v8::Handle<v8::Value>
xrltDeferredResolve(const v8::Arguments& args) {
    fprintf(stderr, "resolve\n");
    return v8::True();
}


void
xrltJSInit(void)
{
    v8::HandleScope   scope;

    xrltXML2JSONTemplateInit();

    xrltDeferredConstructor = \
        v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
    xrltDeferredConstructor->SetClassName(v8::String::New("Deferred"));

    xrltDeferredConstructor->InstanceTemplate()->SetInternalFieldCount(1);

    xrltDeferredConstructor->PrototypeTemplate()->Set(
        v8::String::New("then"),
        v8::FunctionTemplate::New(xrltDeferredThen)
    );

    xrltDeferredConstructor->PrototypeTemplate()->Set(
        v8::String::New("resolve"),
        v8::FunctionTemplate::New(xrltDeferredResolve)
    );
}


void
xrltJSFree(void)
{
    xrltXML2JSONTemplateFree();
    xrltDeferredConstructor.Dispose();
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}


v8::Handle<v8::Value>
Print(const v8::Arguments& args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope scope;
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        v8::String::Utf8Value str(args[i]);
        const char* cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
    return v8::Undefined();
}


v8::Handle<v8::Value>
Apply(const v8::Arguments& args) {
    v8::HandleScope           scope;

    v8::Local<v8::External>  _jsctx;
    xrltJSContextPtr          jsctx;
    xrltJSContextPrivate     *priv;

    _jsctx = v8::Local<v8::External>::Cast(args.Data());
    jsctx = static_cast<xrltJSContextPtr>(_jsctx->Value());
    priv = (xrltJSContextPrivate *)jsctx->_private;

    if (args.Length() > 0) {
        v8::Local<v8::Object>     funcwrap;
        v8::Local<v8::Function>   func;
        v8::Local<v8::Number>     count;
        v8::Local<v8::Object>    _args;
        int                       i;

        _args = v8::Local<v8::Object>::Cast(args[1]);

        funcwrap = v8::Local<v8::Object>::Cast(priv->functions->Get(args[0]));

        func = v8::Local<v8::Function>::Cast(funcwrap->Get(v8::String::New("0")));
        count = v8::Local<v8::Number>::Cast(funcwrap->Get(v8::String::New("1")));

        i = count->Int32Value();
        v8::Local<v8::Value>      argv[i > 0 ? i : 1];

        for (i = i - 1; i >= 0; i--) {
            argv[i] = v8::Local<v8::Value>::New(v8::Undefined());
        }

        if (_args->IsUndefined()) {
            i = 0;
        } else {
            v8::Local<v8::Array>    argnames = _args->GetOwnPropertyNames();
            v8::Local<v8::String>   name;
            v8::Local<v8::Number>   index;
            for (i = argnames->Length() - 1; i >= 0; i--) {
                name = v8::Local<v8::String>::Cast(argnames->Get(i));
                index = v8::Local<v8::Number>::Cast(funcwrap->Get(name));
                if (!index->IsUndefined()) {
                    argv[index->Int32Value()] = _args->Get(name);
                }
            }
            i = count->Int32Value();
        }

        return scope.Close(func->Call(args.Holder(), i, argv));
    }

    return v8::Undefined();
}


xrltJSContextPtr
xrltJSContextCreate(void)
{
    xrltJSContextPtr          ret;
    xrltJSContextPrivate     *priv;
    v8::HandleScope           scope;

    v8::Local<v8::External>   data;
    v8::Local<v8::Function>   stringify;

    ret = (xrltJSContextPtr)xrltMalloc(sizeof(xrltJSContext) +
                                       sizeof(xrltJSContextPrivate));

    if (ret == NULL) { return NULL; }

    memset(ret, 0, sizeof(xrltJSContext) + sizeof(xrltJSContextPrivate));

    priv = (xrltJSContextPrivate *)(ret + 1);
    ret->_private = priv;

    priv->globalTemplate = \
        v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    data = v8::External::New(ret);

    priv->globalTemplate->Set(v8::String::New("apply"),
                              v8::FunctionTemplate::New(Apply, data));

    priv->globalTemplate->Set(v8::String::New("Deferred"),
                              xrltDeferredConstructor);

    priv->globalTemplate->Set(v8::String::New("print"),
                              v8::FunctionTemplate::New(Print));

    priv->context = v8::Context::New(NULL, priv->globalTemplate);

    v8::Context::Scope   context_scope(priv->context);

    priv->global = v8::Persistent<v8::Object>::New(priv->context->Global());

    priv->functions = v8::Persistent<v8::Object>::New(v8::Object::New());

    priv->global->Set(v8::String::New("global"), priv->global);

    return ret;
}


void
xrltJSContextFree(xrltJSContextPtr jsctx)
{
    if (jsctx == NULL) { return; }

    xrltJSContextPrivate  *priv = (xrltJSContextPrivate *)jsctx->_private;

    if (priv != NULL) {
        priv->functions.Dispose();
        priv->context.Dispose();
        priv->global.Dispose();
        priv->globalTemplate.Dispose();
    }

    xrltFree(jsctx);

    while (!v8::V8::IdleNotification());
}


xrltBool
xrltJSFunction(xrltRequestsheetPtr sheet, xmlNodePtr node, xmlChar *name,
               xrltVariableDataPtr *param, size_t paramLen,
               const xmlChar *code)
{
    if (sheet == NULL || name == NULL || code == NULL) { return FALSE; }

    xrltJSContextPtr   jsctx = (xrltJSContextPtr)sheet->js;

    if (jsctx == NULL) {
        jsctx = xrltJSContextCreate();

        if (jsctx == NULL) {
            xrltTransformError(NULL, sheet, node,
                               "JavaScript context creation failed\n");
            return FALSE;
        }

        sheet->js = jsctx;
    }

    xrltJSContextPrivate     *priv = (xrltJSContextPrivate *)jsctx->_private;

    v8::HandleScope           scope;
    v8::Context::Scope        context_scope(priv->context);

    v8::Local<v8::Function>   constr;
    v8::Local<v8::Value>      argv[2];
    int                       argc;
    size_t                    count;
    v8::Local<v8::Object>     funcwrap = v8::Object::New();
    v8::Local<v8::Object>     func;

    if (param != NULL && paramLen > 0) {
        std::string   _args;

        for (count = 0; count < paramLen; count++) {
            funcwrap->Set(v8::String::New((char *)param[count]->jsname),
                          v8::Number::New(count));

            _args.append((char *)param[count]->jsname);

            if (count < paramLen - 1) { _args.append(", "); }
        }

        argv[0] = v8::String::New(_args.c_str());
        argc = 1;
    } else {
        argc = 0;
        count = 0;
    }

    argv[argc++] = v8::String::New((char *)code);

    constr = v8::Local<v8::Function>::Cast(
        priv->global->Get(v8::String::New("Function"))
    );

    v8::TryCatch   trycatch;

    func = constr->NewInstance(argc, argv);

    if (func.IsEmpty()) {
        v8::Handle<v8::Value>    exception = trycatch.Exception();
        v8::String::AsciiValue   exception_str(exception);

        xrltTransformError(NULL, sheet, node, "%s\n", *exception_str);

        return FALSE;
    }

    funcwrap->Set(v8::String::New("0"), func);
    funcwrap->Set(v8::String::New("1"), v8::Number::New(count));

    priv->functions->Set(v8::String::New((char *)name), funcwrap);

    return TRUE;
}


static void
xrltJS2XML(xrltJSON2XMLPtr js2xml, v8::Local<v8::Value> val)
{
    if (val->IsString() || val->IsDate()) {
        v8::Local<v8::String>   _val = val->ToString();
        v8::String::Utf8Value   __val(_val);

        xrltJSON2XMLString(
            js2xml, (const unsigned char *)*__val, (size_t)_val->Length()
        );
    } else if (val->IsNumber()) {
        v8::Local<v8::String>   _val = val->ToString();
        v8::String::Utf8Value   __val(_val);

        xrltJSON2XMLNumber(
            js2xml, (const char *)*__val, (size_t)_val->Length()
        );
    } else if (val->IsBoolean()) {
        xrltJSON2XMLBoolean(js2xml, val->ToBoolean()->Value() ? 1 : 0);
    } else if (val->IsNull()) {
        xrltJSON2XMLNull(js2xml);
    } else if (val->IsArray()) {
        v8::Local<v8::Array>   _val = v8::Local<v8::Array>::Cast(val);
        uint32_t               i;

        xrltJSON2XMLArrayStart(js2xml);

        for (i = 0; i < _val->Length(); i++) {
            xrltJS2XML(js2xml, _val->Get(i));
        }

        xrltJSON2XMLArrayEnd(js2xml);
    } else if (val->IsObject()) {
        v8::Local<v8::Object>   _val = val->ToObject();
        v8::Local<v8::Array>    keys = _val->GetPropertyNames();
        uint32_t                i;
        v8::Local<v8::Value>    key;
        v8::Local<v8::String>   _key;

        xrltJSON2XMLMapStart(js2xml);

        for (i = 0; i < keys->Length(); i++) {
            key = keys->Get(i);
            if (key->IsString()) {
                _key = key->ToString();
            } else {
                _key = v8::Local<v8::String>::Cast(key);
            }

            v8::String::Utf8Value   __key(_key);

            xrltJSON2XMLMapKey(
                js2xml, (const unsigned char *)*__key, (size_t)_key->Length()
            );

            xrltJS2XML(js2xml, _val->Get(key));
        }

        xrltJSON2XMLMapEnd(js2xml);
    }
}


static xrltBool
xrltDeferredVariableResolve(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                            void *data)
{
    xrltDeferredTransformingPtr   dcomp = (xrltDeferredTransformingPtr)comp;
    xmlXPathObjectPtr             val;
    v8::Persistent<v8::Object>   *obj;

    obj = (v8::Persistent<v8::Object> *)dcomp->deferred;

    ctx->varContext = dcomp->varContext;

    val = xrltVariableLookupFunc(ctx, dcomp->name, NULL);

    if (val == NULL) {
        xrltTransformError(ctx, NULL, dcomp->node,
                           "Variable '%s' lookup failed\n", dcomp->name);
        return FALSE;
    }

    // TODO: Call deferred callbacks.

    obj->Dispose();
    delete obj;

    xrltFree(comp);

    return TRUE;
}


xrltBool
xrltJSApply(xrltContextPtr ctx, xmlNodePtr node, xmlChar *name,
            xrltVariableDataPtr *param, size_t paramLen, xmlNodePtr insert)
{
    if (ctx == NULL || name == NULL || insert == NULL) { return FALSE; }

    xrltJSContextPtr              jsctx = (xrltJSContextPtr)ctx->sheet->js;
    xrltJSContextPrivate         *priv = \
                                      (xrltJSContextPrivate *)jsctx->_private;

    v8::HandleScope               scope;
    v8::Context::Scope            context_scope(priv->context);

    v8::Local<v8::Object>         funcwrap;
    v8::Local<v8::Function>       func;
    size_t                        argc;
    v8::Local<v8::Value>          _ret;
    xmlXPathObjectPtr             val;
    xrltDeferredTransformingPtr   deferredData;
    v8::Local<v8::Function>       deferredConstr;
    xrltNodeDataPtr               n;

    funcwrap = priv->functions->Get(v8::String::New((char *)name))->ToObject();

    if (!funcwrap.IsEmpty() && !funcwrap->IsUndefined()) {
        argc = paramLen;

        func = v8::Local<v8::Function>::Cast(
            funcwrap->Get(v8::String::New("0"))
        );

        v8::TryCatch   trycatch;

        if (argc > 0) {
            v8::Handle<v8::Value>   argv[argc];
            size_t                  i;

            for (i = 0; i < argc; i++) {
                ctx->xpathWait = NULL;

                val = xrltVariableLookupFunc(ctx, param[i]->name, NULL);

                if (val == NULL) {
                    xrltTransformError(ctx, NULL, param[i]->node,
                                       "Variable '%s' lookup failed\n",
                                       param[i]->name);
                    return FALSE;
                }

                if (ctx->xpathWait != NULL) {
                    if (deferredConstr.IsEmpty()) {
                        deferredConstr = xrltDeferredConstructor->GetFunction();
                    }

                    XRLT_MALLOC(ctx, NULL, node, deferredData,
                                xrltDeferredTransformingPtr,
                                sizeof(xrltDeferredTransforming), FALSE);

                    v8::Persistent<v8::Object>   *_val;

                    _val = new v8::Persistent<v8::Object>;

                    *_val = v8::Persistent<v8::Object>::New(
                        deferredConstr->NewInstance()
                    );

                    deferredData->node = param[i]->node;
                    deferredData->name = param[i]->name;
                    deferredData->varContext = ctx->varContext;
                    deferredData->deferred = _val;

                    (*_val)->SetInternalField(
                        0, v8::External::New(deferredData)
                    );

                    ASSERT_NODE_DATA(ctx->xpathWait, n);

                    SCHEDULE_CALLBACK(ctx, &n->tcb,
                                      xrltDeferredVariableResolve,
                                      deferredData, insert, &_val);

                    // TODO: Create callbacks for cleanup in XRLT because
                    //       we're deleting *_val inside
                    //       xrltDeferredVariableResolve, so memory leak is
                    //       possible in case of error before.

                    argv[i] = *_val;
                } else {
                    argv[i] = v8::Local<v8::Value>::New(
                        xrltXML2JSONCreate(val)
                    );
                }
            }

            _ret = func->Call(priv->global, argc, argv);
        } else {
            _ret = func->Call(priv->global, 0, NULL);
        }

        if (_ret.IsEmpty()) {
            v8::Handle<v8::Value>    exception = trycatch.Exception();
            v8::String::AsciiValue   exception_str(exception);

            xrltTransformError(ctx, NULL, node, "%s\n", *exception_str);

            return FALSE;
        }

        if (!_ret->IsUndefined()) {
            xrltJSON2XMLPtr   js2xml;

            js2xml = xrltJSON2XMLInit(insert, TRUE);

            xrltJS2XML(js2xml, _ret);

            xrltJSON2XMLFree(js2xml);
        }

        return TRUE;
    } else {
        return FALSE;
    }
}
