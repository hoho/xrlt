#include "../../js.h"

#include "../test.h"


int main()
{
    xrltJSContextPtr   jsctx, jsctx2;
    //int                i;

    printf("12341234134523456\n");

    xrltJSInit();

    xrltJSArgumentList args;

        jsctx = xrltJSContextCreate();
        jsctx2 = xrltJSContextCreate();


        xrltJSArgumentListInit(&args);
        xrltJSArgumentListPush(&args, "arg1");
        xrltJSArgumentListPush(&args, "arg2");
        xrltJSArgumentListPush(&args, "arg3");
        xrltJSSlice(jsctx, "testfunc2", &args, "if (global.aaa) {print(123)}; return 59595;");
        xrltJSSlice(jsctx, "testfunc", &args, "if (global.aaa) { global.aaa += 1} else {global.aaa = 1; }; print(apply('testfunc2', {aa:123, bb:234, arg2:456}) + JSON.stringify(arguments) + global + (this + 1), arg3, global.aaa); return 33;");
        xrltJSArgumentListClear(&args);

        xrltJSArgumentListInit(&args);
        xrltJSArgumentListPush(&args, "arg4");
        xrltJSArgumentListPush(&args, "arg5");
        xrltJSSlice(jsctx2, "testfunc", &args, "if (global.aaa) { global.aaa += 1} else {global.aaa = 1; }; print(JSON.stringify(arguments) + global + (this + 1), arg4, global.aaa); return 33;");
        xrltJSArgumentListClear(&args);

    //for (i = 0; i < 1000000; i++) {

        xrltJSApply(jsctx, "testfunc", NULL);
        xrltJSApply(jsctx, "testfunc", NULL);

        xrltJSApply(jsctx2, "testfunc", NULL);
        xrltJSApply(jsctx2, "testfunc", NULL);
        xrltJSApply(jsctx2, "testfunc", NULL);
        xrltJSApply(jsctx2, "testfunc", NULL);
        xrltJSApply(jsctx2, "testfunc", NULL);
        xrltJSApply(jsctx2, "testfunc", NULL);

        xrltJSApply(jsctx, "testfunc", NULL);

    //}

    xrltJSContextFree(jsctx);
        xrltJSContextFree(jsctx2);


    printf("sdjkfhaskdjfgaskldf\n");
    return 0;
}