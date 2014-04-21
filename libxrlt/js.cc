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


using v8::Persistent; 
using v8::ObjectTemplate;
using v8::Object;
using v8::Context;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Handle;
using v8::HandleScope;
using v8::Value;
using v8::TryCatch;
using v8::String;
using v8::Message;
using v8::Array;
using v8::Exception;
using v8::Undefined;
using v8::Function;
using v8::V8;
using v8::External;
using v8::Number;


typedef struct {
    Persistent<ObjectTemplate>     globalTemplate;
    Persistent<Object>             global;
    Persistent<Object>             functions;
    Persistent<Context>            context;
    Persistent<FunctionTemplate>   deferredTemplate;
    Persistent<ObjectTemplate>     xml2jsCacheTemplate;
    Persistent<ObjectTemplate>     xml2jsTemplate;
} xrltJSContextPrivate;


typedef struct {
    xmlNodePtr           node;
    xmlNodePtr           src;
    Persistent<Object>   deferred;
} xrltDeferredInsertTransformingData;


static xrltBool xrltDeferredInsert   (Isolate *isolate,
                                      xrltContextPtr ctx, void *val,
                                      xmlNodePtr insert, xmlNodePtr src,
                                      xrltDeferredInsertTransformingData *data);

static xrltBool xrltJS2XML           (Isolate *isolate, xrltContextPtr ctx,
                                      xrltJSON2XMLPtr js2xml,
                                      xmlNodePtr srcNode,
                                      Local<Value> val);


void ReportException(Isolate *isolate, xrltContextPtr ctx,
                     xrltRequestsheetPtr sheet, xmlNodePtr node,
                     TryCatch* trycatch)
{
    HandleScope         scope(isolate);

    Local<Value>        exception(trycatch->Exception());
    String::Utf8Value   exception_str(exception);
    Local<Message>      message = trycatch->Message();

    if (message.IsEmpty()) {
        xrltTransformError(ctx, sheet, node, "%s\n", *exception_str);
    } else {
        xrltTransformError(ctx, sheet, node, "Line %i: %s\n",
                           message->GetLineNumber() + node->line - 2,
                           *exception_str);
    }
}


void
xrltDeferredInit(const v8::FunctionCallbackInfo<Value>& args) {
    args.This()->SetAlignedPointerInInternalField(0, NULL);

    args.This()->Set(0, Array::New(0));

    args.GetReturnValue().Set(args.This());
}


void
xrltDeferredThen(const v8::FunctionCallbackInfo<Value>& args) {
    Isolate      *isolate = args.GetIsolate();
    HandleScope   scope(isolate);

    Local<Array>  cb = Local<Array>::Cast(args.This()->Get(0));

    if (args.Length() < 1) {
        args.GetReturnValue().Set(isolate->ThrowException(
            String::NewFromUtf8(isolate, "Too few arguments")
        ));
        return;
    }

    if (!args[0]->IsFunction()) {
        args.GetReturnValue().Set(isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Function is expected")
        )));
        return;
    }

    cb->Set(cb->Length(), args[0]);

    args.GetReturnValue().Set(Undefined(isolate));
}


void
xrltDeferredResolve(const v8::FunctionCallbackInfo<Value>& args) {
    Isolate          *isolate = args.GetIsolate();
    HandleScope       scope(isolate);

    Local<Array>      cb;
    Local<Function>   func;
    uint32_t          i;
    Local<Value>      argv[1];


    if (args.Length() < 1) {
        args.GetReturnValue().Set(isolate->ThrowException(
            String::NewFromUtf8(isolate, "Too few arguments")
        ));
        return;
    }

    if (args.This()->Get(1)->BooleanValue()) {
        args.GetReturnValue().Set(isolate->ThrowException(
            String::NewFromUtf8(isolate, "Can't resolve internal Deferred")
        ));
        return;
    }

    argv[0] = args[0];

    cb = Local<Array>::Cast(args.This()->Get(0));

    TryCatch   trycatch;

    for (i = 0; i < cb->Length(); i++) {
        func = Local<Function>::Cast(cb->Get(i));

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

    args.GetReturnValue().Set(Undefined(isolate));
}


void
xrltDeferredCallback(const v8::FunctionCallbackInfo<Value>& args) {
    Isolate                             *isolate = args.GetIsolate();
    HandleScope                          scope(isolate);

    Local<Object>                        proto;
    xrltContextPtr                       ctx;
    xrltDeferredInsertTransformingData  *tdata;
    Local<Value>                         val;

    proto = Local<Object>::Cast(args.Callee()->GetPrototype());

    ctx = (xrltContextPtr)proto->GetAlignedPointerFromInternalField(0);
    tdata = (xrltDeferredInsertTransformingData *)proto->
                                 GetAlignedPointerFromInternalField(1);

    if (ctx != NULL && tdata != NULL) {
        val = args[0];
        xrltDeferredInsert(isolate, ctx, &val, NULL, NULL, tdata);
    }

    args.GetReturnValue().Set(Undefined(isolate));
}


void
xrltJSInit(void)
{
    V8::InitializeICU();
}


void
xrltJSFree(void)
{
    V8::Dispose();
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}


void
Print(const v8::FunctionCallbackInfo<Value>& args) {
    Isolate *isolate = args.GetIsolate();

    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        HandleScope   scope(isolate);
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        String::Utf8Value str(args[i]);
        const char* cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
    args.GetReturnValue().Set(Undefined(isolate));
}


void
Apply(const v8::FunctionCallbackInfo<Value>& args) {
    Isolate               *isolate = args.GetIsolate();
    HandleScope            scope(isolate);

    Local<External>       _jsctx;
    xrltJSContextPtr       jsctx;
    xrltJSContextPrivate  *priv;

    _jsctx = Local<External>::Cast(args.Data());
    jsctx = static_cast<xrltJSContextPtr>(_jsctx->Value());
    priv = (xrltJSContextPrivate *)jsctx->_private;

    if (args.Length() > 0) {
        Local<Object>     funcs = Local<Object>::New(isolate, priv->functions);
        Local<Object>     funcwrap = Local<Object>::Cast(funcs->Get(args[0]));
        Local<Function>   func;
        Local<Number>     count;
        Local<Object>    _args;
        int               i;

        _args = Local<Object>::Cast(args[1]);

        func = Local<Function>::Cast(funcwrap->Get(String::NewFromUtf8(isolate, "0")));
        count = Local<Number>::Cast(funcwrap->Get(String::NewFromUtf8(isolate, "1")));

        i = count->Int32Value();
        // FIXME: Possible memory leak.
        Local<Value>  *argv = new Local<Value>[i > 0 ? i : 1];

        for (i = i - 1; i >= 0; i--) {
            argv[i] = Local<Value>::New(isolate, Undefined(isolate));
        }

        if (_args->IsUndefined()) {
            i = 0;
        } else {
            Local<Array>    argnames = _args->GetOwnPropertyNames();
            Local<String>   name;
            Local<Number>   index;

            for (i = argnames->Length() - 1; i >= 0; i--) {
                name = Local<String>::Cast(argnames->Get(i));
                index = Local<Number>::Cast(funcwrap->Get(name));

                if (!index->IsUndefined()) {
                    argv[index->Int32Value()] = _args->Get(name);
                }
            }

            i = count->Int32Value();
        }

        args.GetReturnValue().Set(func->Call(args.Holder(), i, argv));

        delete argv;

        return;
    }

    args.GetReturnValue().Set(Undefined(isolate));
}


xrltJSContextPtr
xrltJSContextCreate(void)
{
    xrltJSContextPtr       ret;
    xrltJSContextPrivate  *priv;
    Isolate               *isolate = Isolate::GetCurrent();
    HandleScope            scope(isolate);

    Local<External>        data;

    ret = (xrltJSContextPtr)xmlMalloc(sizeof(xrltJSContext) +
                                      sizeof(xrltJSContextPrivate));

    if (ret == NULL) { return NULL; }

    memset(ret, 0, sizeof(xrltJSContext) + sizeof(xrltJSContextPrivate));

    priv = (xrltJSContextPrivate *)(ret + 1);
    ret->_private = priv;

    data = External::New(isolate, ret);

    Local<ObjectTemplate>     globalTpl = \
        Local<ObjectTemplate>::New(isolate, ObjectTemplate::New());

    Local<FunctionTemplate>   deferredTpl(FunctionTemplate::New(isolate));


    deferredTpl->SetClassName(String::NewFromUtf8(isolate, "Deferred"));
    deferredTpl->InstanceTemplate()->SetInternalFieldCount(3);
    deferredTpl->SetCallHandler(xrltDeferredInit);

    deferredTpl->PrototypeTemplate()->Set(
        String::NewFromUtf8(isolate, "then"),
        FunctionTemplate::New(isolate, xrltDeferredThen)
    );

    deferredTpl->PrototypeTemplate()->Set(
        String::NewFromUtf8(isolate, "resolve"),
        FunctionTemplate::New(isolate, xrltDeferredResolve)
    );

    Local<ObjectTemplate>   xml2jsTpl(ObjectTemplate::New());
    Local<ObjectTemplate>   xml2jsCacheTpl(ObjectTemplate::New());

    xml2jsTpl->SetInternalFieldCount(1);
    xml2jsTpl->SetNamedPropertyHandler(
        xrltXML2JSGetProperty, 0, 0, 0, xrltXML2JSEnumProperties
    );

    xml2jsCacheTpl->SetInternalFieldCount(1);

    globalTpl->Set(String::NewFromUtf8(isolate, "apply"),
                   FunctionTemplate::New(isolate, Apply, data));
    globalTpl->Set(String::NewFromUtf8(isolate, "Deferred"), deferredTpl);
    globalTpl->Set(String::NewFromUtf8(isolate, "print"),
                   FunctionTemplate::New(isolate, Print));

    Local<Context> context = Context::New(isolate, NULL, globalTpl);

    if (context.IsEmpty()) {
        xmlFree(ret);
        return NULL;
    }

    Context::Scope   context_scope(context);
    Local<Object>    global = context->Global();

    global->Set(String::NewFromUtf8(isolate, "global"), global);

    priv->context.Reset(isolate, context);
    priv->globalTemplate.Reset(isolate, globalTpl);
    priv->global.Reset(isolate, global);
    priv->functions.Reset(isolate, Object::New(isolate));
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
        priv->functions.Reset();
        priv->context.Reset();
        priv->global.Reset();
        priv->globalTemplate.Reset();
        priv->deferredTemplate.Reset();
        priv->xml2jsCacheTemplate.Reset();
        priv->xml2jsTemplate.Reset();
    }

    xmlFree(jsctx);

    while (!V8::IdleNotification());
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

    xrltJSContextPrivate  *priv = (xrltJSContextPrivate *)jsctx->_private;

    Isolate               *isolate = Isolate::GetCurrent();
    HandleScope            scope(isolate);
    Local<Context>         context = \
        Local<Context>::New(isolate, priv->context);
    Context::Scope         context_scope(context);

    Local<Function>        constr;
    Local<Value>           argv[2];
    int                    argc;
    size_t                 count;
    Local<Object>          funcwrap = Object::New(isolate);
    Local<Object>          func;
    Local<Object>          global = Local<Object>::New(isolate, priv->global);
    Local<Object>          funcs = Local<Object>::New(isolate, priv->functions);

    if (param != NULL && paramLen > 0) {
        std::string   _args;

        for (count = 0; count < paramLen; count++) {
            funcwrap->Set(String::NewFromUtf8(isolate,
                          (char *)param[count]->jsname),
                          Number::New(isolate, count));

            _args.append((char *)param[count]->jsname);

            if (count < paramLen - 1) { _args.append(", "); }
        }

        argv[0] = String::NewFromUtf8(isolate, _args.c_str());
        argc = 1;
    } else {
        argc = 0;
        count = 0;
    }

    argv[argc++] = String::NewFromUtf8(isolate, (char *)code);

    constr = Local<Function>::Cast(
        global->Get(String::NewFromUtf8(isolate, "Function"))
    );

    TryCatch   trycatch;

    func = constr->NewInstance(argc, argv);

    if (trycatch.HasCaught()) {
        ReportException(isolate, NULL, sheet, node, &trycatch);

        return FALSE;
    }

    funcwrap->Set(String::NewFromUtf8(isolate, "0"), func);
    funcwrap->Set(String::NewFromUtf8(isolate, "1"),
                  Number::New(isolate, count));

    funcs->Set(String::NewFromUtf8(isolate, (char *)name), funcwrap);

    return TRUE;
}


static void
xrltDeferredInsertTransformingFree(void *data)
{
    if (data != NULL) {
        xrltDeferredInsertTransformingData  *tdata = \
                                    (xrltDeferredInsertTransformingData *)data;

        if (!tdata->deferred.IsEmpty()) {
            tdata->deferred.Reset();
        }

        delete tdata;
    }
}


static xrltBool
xrltDeferredInsert(Isolate *isolate, xrltContextPtr ctx, void *val,
                   xmlNodePtr insert, xmlNodePtr src,
                   xrltDeferredInsertTransformingData *data)
{
    xmlNodePtr        node;
    xrltNodeDataPtr   n;

    if (data == NULL) {
        Local<Object>            *d = (Local<Object> *) val;
        Local<ObjectTemplate>     protoTempl;
        Local<Object>             proto;
        Local<FunctionTemplate>   funcTempl;
        Local<Object>             func;
        Local<Function>           then;
        Local<Value>              argv[1];

        NEW_CHILD(ctx, node, insert, "d");

        ASSERT_NODE_DATA(node, n);

        data = new xrltDeferredInsertTransformingData;

        n->data = data;
        n->free = xrltDeferredInsertTransformingFree;

        data->node = node;
        data->src = src;
        data->deferred.Reset(Isolate::GetCurrent(), *d);

        protoTempl = ObjectTemplate::New();
        protoTempl->SetInternalFieldCount(2);
        proto = protoTempl->NewInstance();
        proto->SetAlignedPointerInInternalField(0, ctx);
        proto->SetAlignedPointerInInternalField(1, data);

        funcTempl = FunctionTemplate::New(isolate, xrltDeferredCallback);
        func = funcTempl->GetFunction();
        func->SetPrototype(proto);

        argv[0] = func;

        then = Local<Function>::Cast((*d)->Get(
                                        String::NewFromUtf8(isolate, "then")));

        then->Call(*d, 1, argv);

        COUNTER_INCREASE(ctx, node);
    } else {
        Local<Value>     *_val = (Local<Value> *)val;
        xrltJSON2XMLPtr   js2xml;
        xrltBool          r;

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
xrltJS2XML(Isolate *isolate, xrltContextPtr ctx, xrltJSON2XMLPtr js2xml,
           xmlNodePtr srcNode, Local<Value> val)
{
    if (val->IsString() || val->IsDate()) {
        Local<String>       _val = val->ToString();
        String::Utf8Value   __val(_val);

        xrltJSON2XMLString(
            js2xml, (const unsigned char *)*__val, (size_t)_val->Utf8Length()
        );
    } else if (val->IsNumber()) {
        Local<String>       _val = val->ToString();
        String::Utf8Value   __val(_val);

        xrltJSON2XMLNumber(
            js2xml, (const char *)*__val, (size_t)_val->Utf8Length()
        );
    } else if (val->IsBoolean()) {
        xrltJSON2XMLBoolean(js2xml, val->ToBoolean()->Value() ? 1 : 0);
    } else if (val->IsNull()) {
        xrltJSON2XMLNull(js2xml);
    } else if (val->IsArray()) {
        Local<Array>   _val = Local<Array>::Cast(val);
        uint32_t       i;

        xrltJSON2XMLArrayStart(js2xml);

        for (i = 0; i < _val->Length(); i++) {
            if (!xrltJS2XML(isolate, ctx, js2xml, srcNode, _val->Get(i))) {
                return FALSE;
            }
        }

        xrltJSON2XMLArrayEnd(js2xml);
    } else if (val->IsObject()) {
        xrltJSContextPtr          jsctx = (xrltJSContextPtr)ctx->sheet->js;
        xrltJSContextPrivate     *priv = \
                                       (xrltJSContextPrivate *)jsctx->_private;

        Local<Object>            _val = Local<Object>::Cast(val);
        Local<FunctionTemplate>   deferredConst = \
            Local<FunctionTemplate>::New(isolate, priv->deferredTemplate);

        if (deferredConst->HasInstance(_val)) {
            if (!xrltDeferredInsert(isolate, ctx, &_val, js2xml->cur, srcNode,
                                    NULL))
            {
                return FALSE;
            }
        } else {
            Local<Array>    keys = _val->GetPropertyNames();
            uint32_t        i;
            Local<Value>    key;
            Local<String>   _key;

            xrltJSON2XMLMapStart(js2xml);

            for (i = 0; i < keys->Length(); i++) {
                key = keys->Get(i);

                if (key->IsString()) {
                    _key = Local<String>::Cast(key);
                } else {
                    _key = key->ToString();
                }

                String::Utf8Value   __key(_key);

                xrltJSON2XMLMapKey(js2xml, (const unsigned char *)*__key,
                                   (size_t)_key->Utf8Length());

                if (!xrltJS2XML(isolate, ctx, js2xml, srcNode, _val->Get(key)))
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

    Isolate                      *isolate = Isolate::GetCurrent();
    HandleScope                   scope(isolate);

    Local<Context>                context = \
                                    Local<Context>::New(isolate, priv->context);
    Context::Scope                context_scope(context);

    Persistent<Object>           *obj;
    Local<Function>               resolve;
    Local<Value>                  argv[1];

    obj = (Persistent<Object> *)dcomp->deferred;

    Local<Object>                 tmptmp = Local<Object>::New(isolate, *obj);

    ctx->varContext = dcomp->varContext;

    val = xrltVariableLookupFunc(ctx, dcomp->name, NULL);

    if (val == NULL) {
        xrltTransformError(ctx, NULL, dcomp->node,
                           "Variable '%s' lookup failed\n", dcomp->name);
        return FALSE;
    }

    resolve = Local<Function>::Cast(tmptmp->Get(
                                    String::NewFromUtf8(isolate, "resolve")));

    argv[0] = Local<Value>::New(
        isolate,
        xrltXML2JSCreate(isolate,
                         &priv->xml2jsTemplate,
                         &priv->xml2jsCacheTemplate,
                         val)
    );

    resolve->Call(tmptmp, 1, argv);

    obj->Reset();
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

    xrltJSContextPtr              jsctx = (xrltJSContextPtr)ctx->sheet->js;
    xrltJSContextPrivate         *priv = \
                                      (xrltJSContextPrivate *)jsctx->_private;

    Isolate                      *isolate = Isolate::GetCurrent();
    HandleScope                   scope(isolate);
    Local<Context>                context = \
                                    Local<Context>::New(isolate, priv->context);
    Context::Scope                context_scope(context);

    Local<Object>                 funcwrap;
    Local<Function>               func;
    size_t                        argc;
    Local<Value>                  ret;
    xmlXPathObjectPtr             val;
    xrltDeferredTransformingPtr   deferredData;
    Local<Function>               deferredConstr;
    xrltNodeDataPtr               n;

    Local<Object>                 funcs = \
                                   Local<Object>::New(isolate, priv->functions);
    Local<FunctionTemplate>       deferredTpl = \
                Local<FunctionTemplate>::New(isolate, priv->deferredTemplate);
    Local<Object>                 global = \
                                    Local<Object>::New(isolate, priv->global);

    funcwrap =
        funcs->Get(String::NewFromUtf8(isolate, (char *)name))->ToObject();

    if (!funcwrap.IsEmpty() && !funcwrap->IsUndefined()) {
        argc = paramLen;

        func = Local<Function>::Cast(
            funcwrap->Get(String::NewFromUtf8(isolate, "0"))
        );

        TryCatch   trycatch;

        if (argc > 0) {
            // XXX: Handle this to avoid memory leak.
            Handle<Value>  *argv = new Handle<Value>[argc];
            size_t          i;

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

                    Local<Object> deferred = deferredConstr->NewInstance();

                    Persistent<Object>  *_val = \
                        new Persistent<Object>(isolate, deferred);


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
                    argv[i] = Local<Value>::New(
                        isolate,
                        xrltXML2JSCreate(isolate,
                                         &priv->xml2jsTemplate,
                                         &priv->xml2jsCacheTemplate,
                                         val)
                    );
                }
            }

            ret = func->Call(global, argc, argv);

            delete argv;
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
