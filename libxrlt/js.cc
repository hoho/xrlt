/*
 * Copyright Marat Abdullin (https://github.com/hoho)
 */
#include <v8.h>
#include <string>

#include "xrlterror.h"
#include "js.h"
#include "xml2js.h"
#include "json2xml.h"
#include "transform.h"


typedef struct {
    v8::Persistent<v8::ObjectTemplate>     globalTemplate;
    v8::Persistent<v8::Object>             global;
    v8::Persistent<v8::Object>             functions;
    v8::Persistent<v8::Context>            context;
    v8::Persistent<v8::FunctionTemplate>   deferredTemplate;
    v8::Persistent<v8::ObjectTemplate>     xml2jsCacheTemplate;
    v8::Persistent<v8::ObjectTemplate>     xml2jsTemplate;
} xrltJSContextPrivate;


typedef struct {
    xmlNodePtr                   node;
    xmlNodePtr                   src;
    v8::Persistent<v8::Object>   deferred;
} xrltDeferredInsertTransformingData;


static xrltBool xrltDeferredInsert   (v8::Isolate *isolate,
                                      xrltContextPtr ctx, void *val,
                                      xmlNodePtr insert, xmlNodePtr src,
                                      xrltDeferredInsertTransformingData *data);

static xrltBool xrltJS2XML           (v8::Isolate *isolate, xrltContextPtr ctx,
                                      xrltJSON2XMLPtr js2xml,
                                      xmlNodePtr srcNode,
                                      v8::Local<v8::Value> val);


void ReportException(v8::Isolate *isolate, xrltContextPtr ctx,
                     xrltRequestsheetPtr sheet, xmlNodePtr node,
                     v8::TryCatch* trycatch)
{
    v8::HandleScope          scope(isolate);

    v8::Local<v8::Value>     exception(trycatch->Exception());
    v8::String::AsciiValue   exception_str(exception);
    v8::Local<v8::Message>   message = trycatch->Message();

    if (message.IsEmpty()) {
        xrltTransformError(ctx, sheet, node, "%s\n", *exception_str);
    } else {
        xrltTransformError(ctx, sheet, node, "Line %i: %s\n",
                           message->GetLineNumber() + node->line - 2,
                           *exception_str);
    }
}


void
xrltDeferredInit(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.This()->SetAlignedPointerInInternalField(0, NULL);

    args.This()->Set(0, v8::Array::New(0));

    args.GetReturnValue().Set(args.This());
}


void
xrltDeferredThen(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope        scope(args.GetIsolate());

    v8::Local<v8::Array>   cb = v8::Local<v8::Array>::Cast(args.This()->Get(0));

    if (args.Length() < 1) {
        args.GetReturnValue().Set(
            v8::ThrowException(v8::String::New("Too few arguments"))
        );
        return;
    }

    if (!args[0]->IsFunction()) {
        args.GetReturnValue().Set(v8::ThrowException(
            v8::Exception::TypeError(v8::String::New("Function is expected"))
        ));
        return;
    }

    cb->Set(cb->Length(), args[0]);

    args.GetReturnValue().Set(v8::Undefined());
}


void
xrltDeferredResolve(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate              *isolate = args.GetIsolate();
    v8::HandleScope           scope(isolate);

    v8::Local<v8::Array>      cb;
    v8::Local<v8::Function>   func;
    uint32_t                  i;
    v8::Local<v8::Value>      argv[1];


    if (args.Length() < 1) {
        args.GetReturnValue().Set(
            v8::ThrowException(v8::String::New("Too few arguments"))
        );
        return;
    }

    if (args.This()->Get(1)->BooleanValue()) {
        args.GetReturnValue().Set(v8::ThrowException(
            v8::String::New("Can't resolve internal Deferred")
        ));
        return;
    }

    argv[0] = args[0];

    cb = v8::Local<v8::Array>::Cast(args.This()->Get(0));

    v8::TryCatch   trycatch;

    for (i = 0; i < cb->Length(); i++) {
        func = v8::Local<v8::Function>::Cast(cb->Get(i));

        func->Call(args.This(), 1, argv);

        if (trycatch.HasCaught()) {
            void                         *internal;
            xrltDeferredTransformingPtr   deferredData;
            xrltContextPtr                ctx;

            internal = args.This()->GetAlignedPointerFromInternalField(0);

            if (internal == NULL) {
                args.GetReturnValue().Set(trycatch.ReThrow());
                return;
            }

            deferredData = (xrltDeferredTransformingPtr)args.This()->
                                        GetAlignedPointerFromInternalField(1);
            ctx = (xrltContextPtr)args.This()->
                                        GetAlignedPointerFromInternalField(2);

            if (deferredData != NULL && ctx != NULL) {
                ReportException(isolate, ctx, NULL, deferredData->codeNode,
                                &trycatch);

                ctx->cur |= XRLT_STATUS_ERROR;
            }

            break;
        }
    }

    args.GetReturnValue().Set(v8::Undefined());
}


void
xrltDeferredCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate                         *isolate = args.GetIsolate();
    v8::HandleScope                      scope(isolate);

    v8::Local<v8::Object>                proto;
    xrltContextPtr                       ctx;
    xrltDeferredInsertTransformingData  *tdata;
    v8::Local<v8::Value>                 val;

    proto = v8::Local<v8::Object>::Cast(args.Callee()->GetPrototype());

    ctx = (xrltContextPtr)proto->GetAlignedPointerFromInternalField(0);
    tdata = (xrltDeferredInsertTransformingData *)proto->
                                 GetAlignedPointerFromInternalField(1);

    if (ctx != NULL && tdata != NULL) {
        val = args[0];
        xrltDeferredInsert(isolate, ctx, &val, NULL, NULL, tdata);
    }

    args.GetReturnValue().Set(v8::Undefined());
}


void
xrltJSInit(void)
{
    v8::V8::InitializeICU();
}


void
xrltJSFree(void)
{
    v8::V8::Dispose();
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}


void
Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope   scope(args.GetIsolate());
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
    args.GetReturnValue().Set(v8::Undefined());
}


void
Apply(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate              *isolate = args.GetIsolate();
    v8::HandleScope           scope(isolate);

    v8::Local<v8::External>  _jsctx;
    xrltJSContextPtr          jsctx;
    xrltJSContextPrivate     *priv;

    _jsctx = v8::Local<v8::External>::Cast(args.Data());
    jsctx = static_cast<xrltJSContextPtr>(_jsctx->Value());
    priv = (xrltJSContextPrivate *)jsctx->_private;

    if (args.Length() > 0) {
        v8::Local<v8::Object>     functions = \
            v8::Local<v8::Object>::New(isolate, priv->functions);

        v8::Local<v8::Object>     funcwrap = \
            v8::Local<v8::Object>::Cast(functions->Get(args[0]));
        v8::Local<v8::Function>   func;
        v8::Local<v8::Number>     count;
        v8::Local<v8::Object>    _args;
        int                       i;

        _args = v8::Local<v8::Object>::Cast(args[1]);

        func = \
            v8::Local<v8::Function>::Cast(funcwrap->Get(v8::String::New("0")));
        count = \
            v8::Local<v8::Number>::Cast(funcwrap->Get(v8::String::New("1")));

        i = count->Int32Value();
        v8::Local<v8::Value>      argv[i > 0 ? i : 1];

        for (i = i - 1; i >= 0; i--) {
            argv[i] = v8::Local<v8::Value>::New(isolate, v8::Undefined());
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

        args.GetReturnValue().Set(func->Call(args.Holder(), i, argv));
        return;
    }

    args.GetReturnValue().Set(v8::Undefined());
}


xrltJSContextPtr
xrltJSContextCreate(void)
{
    xrltJSContextPtr          ret;
    xrltJSContextPrivate     *priv;
    v8::Isolate              *isolate = v8::Isolate::GetCurrent();
    v8::HandleScope           scope(isolate);

    v8::Local<v8::External>   data;

    ret = (xrltJSContextPtr)xmlMalloc(sizeof(xrltJSContext) +
                                      sizeof(xrltJSContextPrivate));

    if (ret == NULL) { return NULL; }

    memset(ret, 0, sizeof(xrltJSContext) + sizeof(xrltJSContextPrivate));

    priv = (xrltJSContextPrivate *)(ret + 1);
    ret->_private = priv;

    data = v8::External::New(ret);

    v8::Local<v8::ObjectTemplate>     globalTpl = v8::Local<v8::ObjectTemplate>::New(isolate, v8::ObjectTemplate::New());
    v8::Local<v8::FunctionTemplate>   deferredTpl(v8::FunctionTemplate::New());


    deferredTpl->SetClassName(v8::String::New("Deferred"));
    deferredTpl->InstanceTemplate()->SetInternalFieldCount(3);
    deferredTpl->SetCallHandler(xrltDeferredInit);

    deferredTpl->PrototypeTemplate()->Set(
        v8::String::New("then"),
        v8::FunctionTemplate::New(xrltDeferredThen)
    );

    deferredTpl->PrototypeTemplate()->Set(
        v8::String::New("resolve"),
        v8::FunctionTemplate::New(xrltDeferredResolve)
    );

    v8::Local<v8::ObjectTemplate>   xml2jsTpl(v8::ObjectTemplate::New());
    v8::Local<v8::ObjectTemplate>   xml2jsCacheTpl(v8::ObjectTemplate::New());

    xml2jsTpl->SetInternalFieldCount(1);
    xml2jsTpl->SetNamedPropertyHandler(
        xrltXML2JSGetProperty, 0, 0, 0, xrltXML2JSEnumProperties
    );

    xml2jsCacheTpl->SetInternalFieldCount(1);

    globalTpl->Set(v8::String::New("apply"),
                   v8::FunctionTemplate::New(Apply, data));
    globalTpl->Set(v8::String::New("Deferred"), deferredTpl);
    globalTpl->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));

    v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, globalTpl);

    if (context.IsEmpty()) {
        xmlFree(ret);
        return NULL;
    }

    v8::Context::Scope       context_scope(context);

    v8::Local<v8::Object>    global = context->Global();

    global->Set(v8::String::New("global"), global);

    priv->context.Reset(isolate, context);
    priv->globalTemplate.Reset(isolate, globalTpl);
    priv->global.Reset(isolate, global);
    priv->functions.Reset(isolate, v8::Object::New());
    priv->deferredTemplate.Reset(isolate, deferredTpl);
    priv->xml2jsTemplate.Reset(isolate, xml2jsTpl);
    priv->xml2jsCacheTemplate.Reset(isolate, xml2jsCacheTpl);

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
        priv->deferredTemplate.Dispose();
        priv->xml2jsCacheTemplate.Dispose();
        priv->xml2jsTemplate.Dispose();
    }

    xmlFree(jsctx);

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

    v8::Isolate              *isolate = v8::Isolate::GetCurrent();
    v8::HandleScope           scope(isolate);
    v8::Local<v8::Context>    context = \
        v8::Local<v8::Context>::New(isolate, priv->context);
    v8::Context::Scope        context_scope(context);

    v8::Local<v8::Function>   constr;
    v8::Local<v8::Value>      argv[2];
    int                       argc;
    size_t                    count;
    v8::Local<v8::Object>     funcwrap = v8::Object::New();
    v8::Local<v8::Object>     func;
    v8::Local<v8::Object>     global = \
        v8::Local<v8::Object>::New(isolate, priv->global);
    v8::Local<v8::Object>     functions = \
        v8::Local<v8::Object>::New(isolate, priv->functions);

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
        global->Get(v8::String::New("Function"))
    );

    v8::TryCatch   trycatch;

    func = constr->NewInstance(argc, argv);

    if (trycatch.HasCaught()) {
        ReportException(isolate, NULL, sheet, node, &trycatch);

        return FALSE;
    }

    funcwrap->Set(v8::String::New("0"), func);
    funcwrap->Set(v8::String::New("1"), v8::Number::New(count));

    functions->Set(v8::String::New((char *)name), funcwrap);

    return TRUE;
}


static void
xrltDeferredInsertTransformingFree(void *data)
{
    if (data != NULL) {
        xrltDeferredInsertTransformingData  *tdata = \
                                    (xrltDeferredInsertTransformingData *)data;

        if (!tdata->deferred.IsEmpty()) {
            tdata->deferred.Dispose();
        }

        delete tdata;
    }
}


static xrltBool
xrltDeferredInsert(v8::Isolate *isolate, xrltContextPtr ctx, void *val,
                   xmlNodePtr insert, xmlNodePtr src,
                   xrltDeferredInsertTransformingData *data)
{
    xmlNodePtr                           node;
    xrltNodeDataPtr                      n;

    if (data == NULL) {
        v8::Local<v8::Object>            *d = (v8::Local<v8::Object> *) val;
        v8::Local<v8::ObjectTemplate>     protoTempl;
        v8::Local<v8::Object>             proto;
        v8::Local<v8::FunctionTemplate>   funcTempl;
        v8::Local<v8::Object>             func;
        v8::Local<v8::Function>           then;
        v8::Local<v8::Value>              argv[1];

        NEW_CHILD(ctx, node, insert, "d");

        ASSERT_NODE_DATA(node, n);

        data = new xrltDeferredInsertTransformingData;

        n->data = data;
        n->free = xrltDeferredInsertTransformingFree;

        data->node = node;
        data->src = src;
        data->deferred.Reset(v8::Isolate::GetCurrent(), *d);

        protoTempl = v8::ObjectTemplate::New();
        protoTempl->SetInternalFieldCount(2);
        proto = protoTempl->NewInstance();
        proto->SetAlignedPointerInInternalField(0, ctx);
        proto->SetAlignedPointerInInternalField(1, data);

        funcTempl = v8::FunctionTemplate::New(xrltDeferredCallback);
        func = funcTempl->GetFunction();
        func->SetPrototype(proto);

        argv[0] = func;

        then = \
            v8::Local<v8::Function>::Cast((*d)->Get(v8::String::New("then")));

        then->Call(*d, 1, argv);

        COUNTER_INCREASE(ctx, node);
    } else {
        v8::Local<v8::Value>  *_val = (v8::Local<v8::Value> *)val;
        xrltJSON2XMLPtr        js2xml;
        xrltBool               r;

        node = data->node;

        js2xml = xrltJSON2XMLInit(node, TRUE);

        r = xrltJS2XML(isolate, ctx, js2xml, data->src, *_val);

        xrltJSON2XMLFree(js2xml);

        COUNTER_DECREASE(ctx, node);

        ASSERT_NODE_DATA(node, n);

        if (r) {
            REPLACE_RESPONSE_NODE(ctx, node, node->children, data->src);
        }

        return r;
    }

    return TRUE;
}


static xrltBool
xrltJS2XML(v8::Isolate *isolate, xrltContextPtr ctx, xrltJSON2XMLPtr js2xml,
           xmlNodePtr srcNode, v8::Local<v8::Value> val)
{
    if (val->IsString() || val->IsDate()) {
        v8::Local<v8::String>   _val = val->ToString();
        v8::String::Utf8Value   __val(_val);

        xrltJSON2XMLString(
            js2xml, (const unsigned char *)*__val, (size_t)_val->Utf8Length()
        );
    } else if (val->IsNumber()) {
        v8::Local<v8::String>   _val = val->ToString();
        v8::String::Utf8Value   __val(_val);

        xrltJSON2XMLNumber(
            js2xml, (const char *)*__val, (size_t)_val->Utf8Length()
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
            if (!xrltJS2XML(isolate, ctx, js2xml, srcNode, _val->Get(i))) {
                return FALSE;
            }
        }

        xrltJSON2XMLArrayEnd(js2xml);
    } else if (val->IsObject()) {
        xrltJSContextPtr              jsctx = (xrltJSContextPtr)ctx->sheet->js;
        xrltJSContextPrivate         *priv = \
                                       (xrltJSContextPrivate *)jsctx->_private;

        v8::Local<v8::Object>            _val = \
            v8::Local<v8::Object>::Cast(val);
        v8::Local<v8::FunctionTemplate>   deferredConst = \
            v8::Local<v8::FunctionTemplate>::New(isolate,
                                                 priv->deferredTemplate);

        if (deferredConst->HasInstance(_val)) {
            if (!xrltDeferredInsert(isolate, ctx, &_val, js2xml->cur, srcNode,
                                    NULL))
            {
                return FALSE;
            }
        } else {
            v8::Local<v8::Array>    keys = _val->GetPropertyNames();
            uint32_t                i;
            v8::Local<v8::Value>    key;
            v8::Local<v8::String>   _key;

            xrltJSON2XMLMapStart(js2xml);

            for (i = 0; i < keys->Length(); i++) {
                key = keys->Get(i);

                if (key->IsString()) {
                    _key = v8::Local<v8::String>::Cast(key);
                } else {
                    _key = key->ToString();
                }

                v8::String::Utf8Value   __key(_key);

                xrltJSON2XMLMapKey(js2xml, (const unsigned char *)*__key,
                                   (size_t)_key->Utf8Length());

                if (!xrltJS2XML(isolate, ctx, js2xml, srcNode,
                                _val->Get(key)))
                {
                    return FALSE;
                }
            }

            xrltJSON2XMLMapEnd(js2xml);
        }
    }

    return TRUE;
}


static xrltBool
xrltDeferredVariableResolve(xrltContextPtr ctx, void *comp, xmlNodePtr insert,
                            void *data)
{
    xrltDeferredTransformingPtr   dcomp = (xrltDeferredTransformingPtr)comp;
    xmlXPathObjectPtr             val;

    xrltJSContextPtr              jsctx = (xrltJSContextPtr)ctx->sheet->js;
    xrltJSContextPrivate         *priv = \
                                      (xrltJSContextPrivate *)jsctx->_private;

    v8::Isolate                  *isolate = v8::Isolate::GetCurrent();
    v8::HandleScope               scope(isolate);

    v8::Local<v8::Context>        context = \
        v8::Local<v8::Context>::New(isolate, priv->context);
    v8::Context::Scope            context_scope(context);

    v8::Persistent<v8::Object>   *obj;
    v8::Local<v8::Function>       resolve;
    v8::Local<v8::Value>          argv[1];

    obj = (v8::Persistent<v8::Object> *)dcomp->deferred;

    v8::Local<v8::Object>   tmptmp = v8::Local<v8::Object>::New(isolate, *obj);

    ctx->varContext = dcomp->varContext;

    val = xrltVariableLookupFunc(ctx, dcomp->name, NULL);

    if (val == NULL) {
        xrltTransformError(ctx, NULL, dcomp->node,
                           "Variable '%s' lookup failed\n", dcomp->name);
        return FALSE;
    }

    resolve = \
        v8::Local<v8::Function>::Cast(tmptmp->Get(v8::String::New("resolve")));

    argv[0] = v8::Local<v8::Value>::New(
        isolate,
        xrltXML2JSCreate(isolate,
                         &priv->xml2jsTemplate,
                         &priv->xml2jsCacheTemplate,
                         val)
    );

    resolve->Call(tmptmp, 1, argv);

    obj->Dispose();
    delete obj;

    xmlFree(comp);

    if (ctx->cur & XRLT_STATUS_ERROR) {
        return FALSE;
    }

    return TRUE;
}


xrltBool
xrltJSApply(xrltContextPtr ctx, xmlNodePtr node, xmlChar *name,
            xrltVariableDataPtr *param, size_t paramLen, xmlNodePtr insert)
{
    if (ctx == NULL || name == NULL || insert == NULL) { return FALSE; }

    xrltJSContextPtr                  jsctx = (xrltJSContextPtr)ctx->sheet->js;
    xrltJSContextPrivate             *priv = \
                                      (xrltJSContextPrivate *)jsctx->_private;

    v8::Isolate                      *isolate = v8::Isolate::GetCurrent();
    v8::HandleScope                   scope(isolate);
    v8::Local<v8::Context>            context = \
        v8::Local<v8::Context>::New(isolate, priv->context);
    v8::Context::Scope                context_scope(context);

    v8::Local<v8::Object>             funcwrap;
    v8::Local<v8::Function>           func;
    size_t                            argc;
    v8::Local<v8::Value>              ret;
    xmlXPathObjectPtr                 val;
    xrltDeferredTransformingPtr       deferredData;
    v8::Local<v8::Function>           deferredConstr;
    xrltNodeDataPtr                   n;

    v8::Local<v8::Object>             functions = \
        v8::Local<v8::Object>::New(isolate, priv->functions);
    v8::Local<v8::FunctionTemplate>   deferredTpl = \
        v8::Local<v8::FunctionTemplate>::New(isolate, priv->deferredTemplate);
    v8::Local<v8::Object>             global = \
        v8::Local<v8::Object>::New(isolate, priv->global);

    funcwrap = functions->Get(v8::String::New((char *)name))->ToObject();

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
                        deferredConstr = deferredTpl->GetFunction();
                    }

                    XRLT_MALLOC(ctx, NULL, node, deferredData,
                                xrltDeferredTransformingPtr,
                                sizeof(xrltDeferredTransforming), FALSE);

                    v8::Local<v8::Object> deferred = \
                        deferredConstr->NewInstance();

                    v8::Persistent<v8::Object>   *_val = \
                        new v8::Persistent<v8::Object>(isolate, deferred);


                    deferredData->node = param[i]->node;
                    deferredData->codeNode = node;
                    deferredData->name = param[i]->name;
                    deferredData->varContext = ctx->varContext;
                    deferredData->deferred = _val;

                    deferred->SetAlignedPointerInInternalField(0, (void *)0x2);
                    deferred->SetAlignedPointerInInternalField(1, deferredData);
                    deferred->SetAlignedPointerInInternalField(2, ctx);

                    ASSERT_NODE_DATA(ctx->xpathWait, n);

                    SCHEDULE_CALLBACK(ctx, &n->tcb,
                                      xrltDeferredVariableResolve,
                                      deferredData, insert, &_val);

                    // TODO: Create callbacks for cleanup in XRLT because
                    //       we're deleting *_val inside
                    //       xrltDeferredVariableResolve, so memory leak is
                    //       possible in case of error before.

                    argv[i] = deferred;
                } else {
                    argv[i] = v8::Local<v8::Value>::New(
                        isolate,
                        xrltXML2JSCreate(isolate,
                                         &priv->xml2jsTemplate,
                                         &priv->xml2jsCacheTemplate,
                                         val)
                    );
                }
            }

            ret = func->Call(global, argc, argv);
        } else {
            ret = func->Call(global, 0, NULL);
        }

        if (trycatch.HasCaught()) {
            ReportException(isolate, ctx, NULL, node, &trycatch);

            return FALSE;
        }

        if (!ret->IsUndefined()) {
            xrltJSON2XMLPtr   js2xml;
            xrltBool          r;

            js2xml = xrltJSON2XMLInit(insert, TRUE);

            r = xrltJS2XML(isolate, ctx, js2xml, node, ret);

            xrltJSON2XMLFree(js2xml);

            return r;
        } else {
            return TRUE;
        }
    } else {
        return FALSE;
    }
}
