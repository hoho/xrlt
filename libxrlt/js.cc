#include <v8.h>
#include <string>

#include "js.h"
#include "xml2json.h"


typedef struct {
    v8::Persistent<v8::ObjectTemplate>   globalTemplate;
    v8::Persistent<v8::Object>           global;
    v8::Persistent<v8::Object>           functions;
    v8::Persistent<v8::Context>          context;
    v8::Persistent<v8::Object>           JSONobject;
    v8::Persistent<v8::Function>         stringify;
    v8::Persistent<v8::Function>         replacer;
} xrltJSContextPrivate;


v8::Persistent<v8::FunctionTemplate> xrltDeferredConstructor;
v8::Persistent<v8::FunctionTemplate> xrltStringifyReplacerTemplate;


v8::Handle<v8::Value>
xrltDeferredThen(const v8::Arguments& args) {
    printf("then\n");
    return v8::True();
}


v8::Handle<v8::Value>
xrltDeferredResolve(const v8::Arguments& args) {
    printf("resolve\n");
    return v8::True();
}


v8::Handle<v8::Value>
xrltStringifyReplacer(const v8::Arguments& args) {
    if (xrltDeferredConstructor->HasInstance(args[1])) {
        printf("fuckfuckufckfuckfuck\n");
        return v8::True();
    } else {
        return args[1];
    }
}


void
xrltJSInit(void)
{
    v8::HandleScope   scope;

    xrltXML2JSONTemplateInit();

    xrltDeferredConstructor = \
        v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
    xrltDeferredConstructor->SetClassName(v8::String::New("Deferred"));

    xrltDeferredConstructor->PrototypeTemplate()->Set(
        v8::String::New("then"),
        v8::FunctionTemplate::New(xrltDeferredThen)
    );

    xrltDeferredConstructor->PrototypeTemplate()->Set(
        v8::String::New("resolve"),
        v8::FunctionTemplate::New(xrltDeferredResolve)
    );

    xrltStringifyReplacerTemplate = v8::Persistent<v8::FunctionTemplate>::New(
         v8::FunctionTemplate::New(xrltStringifyReplacer)
    );
}


void
xrltJSFree(void)
{
    xrltXML2JSONTemplateFree();
    xrltDeferredConstructor.Dispose();
    xrltStringifyReplacerTemplate.Dispose();
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

    priv->JSONobject = v8::Persistent<v8::Object>::New(
        priv->global->Get(v8::String::New("JSON"))->ToObject()
    );
    stringify = v8::Local<v8::Function>::Cast(
        priv->JSONobject->Get(v8::String::New("stringify"))
    );
    priv->stringify = v8::Persistent<v8::Function>::New(stringify);

    priv->replacer = v8::Persistent<v8::Function>::New(
        xrltStringifyReplacerTemplate->GetFunction()
    );

    return ret;
}


void
xrltJSContextFree(xrltJSContextPtr jsctx)
{
    if (jsctx == NULL) { return; }

    xrltJSContextPrivate  *priv = (xrltJSContextPrivate *)jsctx->_private;

    if (priv != NULL) {
        priv->stringify.Dispose();
        priv->JSONobject.Dispose();
        priv->replacer.Dispose();
        priv->functions.Dispose();
        priv->context.Dispose();
        priv->global.Dispose();
        priv->globalTemplate.Dispose();
    }

    xrltFree(jsctx);

    while (!v8::V8::IdleNotification());
}


xrltBool
xrltJSFunction(xrltJSContextPtr jsctx, char *name,
               xrltJSArgumentListPtr args, char *code)
{
    if (jsctx == NULL || name == NULL || code == NULL) { return FALSE; }

    xrltJSContextPrivate  *priv = (xrltJSContextPrivate *)jsctx->_private;

    v8::HandleScope           scope;
    v8::Context::Scope        context_scope(priv->context);

    v8::Local<v8::Function>   constr;
    v8::Local<v8::Value>      argv[2];
    int                       argc;
    int                       count;
    v8::Local<v8::Object>     funcwrap = v8::Object::New();
    v8::Local<v8::Object>     func;

    if (args != NULL && args->len > 0) {
        xrltJSArgumentPtr   arg;
        std::string        _args;
        for (count = 0; count < args->len; count++) {
            arg = &args->arg[count];
            funcwrap->Set(v8::String::New(arg->name), v8::Number::New(count));
            _args.append(arg->name);
            if (count < args->len - 1) { _args.append(", "); }
        }
        argv[0] = v8::String::New(_args.c_str());
        argc = 1;
    } else {
        argc = 0;
        count = 0;
    }

    argv[argc++] = v8::String::New(code);

    constr = v8::Local<v8::Function>::Cast(
        priv->global->Get(v8::String::New("Function"))
    );
    func = constr->NewInstance(argc, argv);

    funcwrap->Set(v8::String::New("0"), func);
    funcwrap->Set(v8::String::New("1"), v8::Number::New(count));

    priv->functions->Set(v8::String::New(name), funcwrap);

    return TRUE;
}


xrltBool
xrltJSApply(xrltJSContextPtr jsctx, char *name, xrltJSArgumentListPtr args,
            char **ret)
{
    if (jsctx == NULL || name == NULL || ret == NULL) { return FALSE; }

    xrltJSContextPrivate     *priv = (xrltJSContextPrivate *)jsctx->_private;

    v8::HandleScope           scope;
    v8::Context::Scope        context_scope(priv->context);
    v8::Local<v8::Object>     funcwrap;
    v8::Local<v8::Function>   func;
    int                       argc;
    v8::Local<v8::Value>      _ret;

    funcwrap = priv->functions->Get(v8::String::New(name))->ToObject();
    if (!funcwrap->IsUndefined()) {
        argc = args != NULL && args->len > 0 ? args->len : 0;
        func = v8::Local<v8::Function>::Cast(
            funcwrap->Get(v8::String::New("0"))
        );

        if (argc > 0) {
            v8::Local<v8::Value>   argv[argc];
            int                    i;

            for (i = 0; i < argc; i++) {
                argv[i] = v8::Local<v8::Value>::New(
                    xrltXML2JSONCreate(args->arg[i].val)
                );
            }
            _ret = func->Call(priv->global, argc, argv);
        } else {
            _ret = func->Call(priv->global, 0, NULL);
        }

        if (_ret.IsEmpty() || _ret->IsUndefined()) {
            *ret = NULL;
        } else {
            v8::Local<v8::Value>   __ret;
            v8::Local<v8::Value>   argv[2] = {
                _ret,
                v8::Local<v8::Value>::New(priv->replacer)
            };

            __ret = priv->stringify->Call(priv->JSONobject, 2, argv);
            v8::String::Utf8Value   ___ret(__ret);
            *ret = strdup(*___ret);
        }
    } else {
        return FALSE;
    }

    return TRUE;
}
