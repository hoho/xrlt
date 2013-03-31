#include <v8.h>
#include <string>

#include "js.h"
#include "xml2json.h"


typedef struct {
    v8::Persistent<v8::ObjectTemplate>   global;
    v8::Persistent<v8::Object>           slice;
    v8::Persistent<v8::Context>          context;
} xrltJSContextPrivate;


void
xrltJSInit(void)
{
    xrltXML2JSONTemplateInit();
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}


v8::Handle<v8::Value> Print(const v8::Arguments& args) {
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


v8::Handle<v8::Value> Apply(const v8::Arguments& args) {
    v8::HandleScope           scope;

    v8::Local<v8::External>  _jsctx;
    xrltJSContextPtr          jsctx;
    xrltJSContextPrivate     *priv;

    _jsctx = v8::Local<v8::External>::Cast(args.Data());
    jsctx = static_cast<xrltJSContextPtr>(_jsctx->Value());
    priv = (xrltJSContextPrivate *)jsctx->_private;

    if (args.Length() > 0) {
        v8::Local<v8::Object>     slice;
        v8::Local<v8::Function>   func;
        v8::Local<v8::Number>     count;
        v8::Local<v8::Object>    _args;
        int                       i;

        _args = v8::Local<v8::Object>::Cast(args[1]);

        slice = v8::Local<v8::Object>::Cast(priv->slice->Get(args[0]));

        func = v8::Local<v8::Function>::Cast(slice->Get(v8::String::New("0")));
        count = v8::Local<v8::Number>::Cast(slice->Get(v8::String::New("1")));

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
                index = v8::Local<v8::Number>::Cast(slice->Get(name));
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

    ret = (xrltJSContextPtr)xrltMalloc(sizeof(xrltJSContext) +
                                       sizeof(xrltJSContextPrivate));

    if (ret == NULL) { return NULL; }

    memset(ret, 0, sizeof(xrltJSContext) + sizeof(xrltJSContextPrivate));

    printf("affff: %p\n", ret);


    priv = (xrltJSContextPrivate *)(ret + 1);
    ret->_private = priv;

    priv->global = \
        v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    data = v8::External::New(ret);

    priv->global->Set(v8::String::New("apply"),
                      v8::FunctionTemplate::New(Apply, data));
    priv->global->Set(v8::String::New("print"),
                      v8::FunctionTemplate::New(Print));
    priv->context = v8::Context::New(NULL, priv->global);

    v8::Context::Scope context_scope(priv->context);

    priv->slice = v8::Persistent<v8::Object>::New(v8::Object::New());

    priv->context->Global()->Set(v8::String::New("global"),
                                 priv->context->Global());
    return ret;
}


void
xrltJSContextFree(xrltJSContextPtr jsctx)
{
    if (jsctx == NULL) { return; }

    xrltJSContextPrivate  *priv = (xrltJSContextPrivate *)jsctx->_private;

    if (priv != NULL) {
        priv->slice.Dispose();
        priv->context.Dispose();
        priv->global.Dispose();
    }

    xrltFree(jsctx);

    while (!v8::V8::IdleNotification());
}


xrltBool
xrltJSSlice(xrltJSContextPtr jsctx, char *name,
            xrltJSArgumentList *args, char *code)
{
    if (jsctx == NULL || name == NULL || code == NULL) { return FALSE; }

    xrltJSContextPrivate  *priv = (xrltJSContextPrivate *)jsctx->_private;

    v8::HandleScope           scope;
    v8::Context::Scope        context_scope(priv->context);

    v8::Local<v8::Object>     global = priv->context->Global();
    v8::Local<v8::Function>   constr;
    v8::Local<v8::Value>      argv[2];
    int                       argc;
    int                       count;
    v8::Local<v8::Object>     slice = v8::Object::New();
    v8::Local<v8::Object>     func;

    if (args != NULL && args->first != NULL) {
        xrltJSArgumentPtr   arg = args->first;
        std::string        _args;
        count = 0;
        while (arg != NULL) {
            slice->Set(v8::String::New(arg->name), v8::Number::New(count++));
            _args.append(arg->name);
            arg = arg->next;
            if (arg != NULL) { _args.append(", "); }
        }
        argv[0] = v8::String::New(_args.c_str());
        argc = 1;
    } else {
        argc = 0;
        count = 0;
    }

    argv[argc++] = v8::String::New(code);

    constr = \
        v8::Local<v8::Function>::Cast(global->Get(v8::String::New("Function")));
    func = constr->NewInstance(argc, argv);

    slice->Set(v8::String::New("0"), func);
    slice->Set(v8::String::New("1"), v8::Number::New(count));

    priv->slice->Set(v8::String::New(name), slice);

    return TRUE;
}


xrltBool
xrltJSApply(xrltJSContextPtr jsctx, char *name, xrltJSArgumentList *args)
{
    if (jsctx == NULL || name == NULL) { return FALSE; }

    xrltJSContextPrivate  *priv = (xrltJSContextPrivate *)jsctx->_private;

    v8::HandleScope           scope;
    v8::Context::Scope        context_scope(priv->context);
    v8::Local<v8::Object>     slice;
    v8::Local<v8::Function>   func;

    slice = v8::Local<v8::Object>::Cast(priv->slice->Get(v8::String::New(name)));
    if (!slice->IsUndefined()) {
        func = v8::Local<v8::Function>::Cast(slice->Get(v8::String::New("0")));
        func->Call(priv->context->Global(), 0, NULL);
    } else {
        return FALSE;
    }

    return TRUE;
}
