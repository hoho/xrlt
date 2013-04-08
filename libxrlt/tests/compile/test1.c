#include "../../transform.h"

#include "../test.h"


void
printData(xrltContextPtr ctx)
{
    xrltString       s;
    xrltLogType      t;

    while (xrltLogListShift(&ctx->log, &t, &s)) {
        printf("log: %d %s\n", t, s.data);
        xrltStringClear(&s);
    }

    while (xrltChunkListShift(&ctx->chunk, &s)) {
        printf("chunk: %s\n", s.data);
        xrltStringClear(&s);
    }
}


int
main()
{
    xmlDocPtr             doc;
    xrltRequestsheetPtr   sheet;

    xrltInit();

    doc = xmlReadFile("compile/test1.1.xml", NULL, 0);
    if (doc == NULL) {
        printf("Failed to read tmp.xml\n");
    } else {
        printf("sdakjlfhlkj\n");
        sheet = xrltRequestsheetCreate(doc);
    }
    if (sheet == NULL) {
        xmlFreeDoc(doc);
    } else {
        int              ret;
        xrltContextPtr   ctx;

        ctx = xrltContextCreate(sheet, NULL);

        ret = xrltTransform(ctx, NULL);
        printData(ctx);

        while (!((ret & XRLT_STATUS_DONE) | (ret & XRLT_STATUS_ERROR))) {
            ret = xrltTransform(ctx, NULL);
            printData(ctx);
        }


        xrltContextFree(ctx);
        xrltRequestsheetFree(sheet);
    }
    xrltCleanup();
    return 0;
}
