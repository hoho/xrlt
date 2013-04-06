#include "../../js.h"

#include "../test.h"


int main()
{
    xrltJSContextPtr   jsctx, jsctx2;
    char              *ret;
    //int                i;

    printf("12341234134523456\n");

    xrltJSInit();

    xrltJSArgumentListPtr args;

        jsctx = xrltJSContextCreate();
        jsctx2 = xrltJSContextCreate();


        args = xrltJSArgumentListCreate(3);
        xrltJSArgumentListPush(args, "arg1", NULL);
        xrltJSArgumentListPush(args, "arg2", NULL);
        xrltJSArgumentListPush(args, "arg3", NULL);
        xrltJSSlice(jsctx, "testfunc", args, "print(11); return {fuck: apply('testfunc2'), heck: apply('testfunc3')};");
        xrltJSSlice(jsctx, "testfunc2", args, "if (global.aaa) {print(123 + '' + new Deferred())}; return 59595;");
        xrltJSSlice(jsctx, "testfunc3", args, "return new Deferred()");
        xrltJSArgumentListFree(args);

        args = xrltJSArgumentListCreate(2);
        xrltJSArgumentListPush(args, "arg4", NULL);
        xrltJSArgumentListPush(args, "arg5", NULL);
        xrltJSSlice(jsctx2, "testfunc", args, "if (global.aaa) { global.aaa += 1} else {global.aaa = 1; }; print(JSON.stringify(arguments) + global + (this + 1), arg4, global.aaa); return 33;");
        xrltJSArgumentListFree(args);

    //for (i = 0; i < 1000000; i++) {


        args = xrltJSArgumentListCreate(1);
        xmlXPathObject rddm;
        memset(&rddm, 0, sizeof(xmlXPathObject));
        xmlDocPtr doc;
        doc = xmlReadFile("json2xml/test1.out.xml", NULL, 0);
        if (doc == NULL) {
            printf("Failed to read tmp.xml\n");
        } else {
            rddm.type = XPATH_USERS;
            rddm.user = xmlDocGetRootElement(doc);
            //rddm.type = XPATH_NUMBER;
            //rddm.floatval = 9876;
            xrltJSArgumentListPush(args, "arg4", &rddm);
        }
        xrltJSApply(jsctx, "testfunc", args, &ret);
        printf(";;;[;[;; %s\n", ret);
        xrltJSArgumentListFree(args);
        xmlFreeDoc(doc);

        xrltJSApply(jsctx, "testfunc", NULL, &ret);

        xrltJSApply(jsctx2, "testfunc", NULL, &ret);
        xrltJSApply(jsctx2, "testfunc", NULL, &ret);
        xrltJSApply(jsctx2, "testfunc", NULL, &ret);
        xrltJSApply(jsctx2, "testfunc", NULL, &ret);
        xrltJSApply(jsctx2, "testfunc", NULL, &ret);
        xrltJSApply(jsctx2, "testfunc", NULL, &ret);

        xrltJSApply(jsctx, "testfunc", NULL, &ret);

    //}

    xrltJSContextFree(jsctx);
    xrltJSContextFree(jsctx2);

    xrltJSFree();

    printf("sdjkfhaskdjfgaskldf\n");
    return 0;
}
