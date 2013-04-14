#include "../../transform.h"

#include "../test.h"
#include "xrltstruct.h"


void
printData(xrltContextPtr ctx)
{
    xrltString       s;
    xrltLogType      t;
    xrltString       url, q, b, n, v;
    xrltHeaderList   header;
    size_t           id;

    while (xrltLogListShift(&ctx->log, &t, &s)) {
        printf("log: %d %s\n", t, s.data);
        xrltStringClear(&s);
    }

    while (xrltChunkListShift(&ctx->chunk, &s)) {
        printf("chunk: %s\n", s.data);
        xrltStringClear(&s);
    }

    while (xrltSubrequestListShift(&ctx->sr, &id, &header, &url, &q, &b)) {
        printf("sr id: %d\n", (int)id);
        while (xrltHeaderListShift(&header, &n, &v)) {
            printf("sr header: %s: %s\n", n.data, v.data);
            xrltStringClear(&n);
            xrltStringClear(&v);
        }

        printf("sr url: %s\n", url.data);
        xrltStringClear(&url);

        printf("sr query: %s\n", q.data);
        xrltStringClear(&q);

        printf("sr body: %s\n", b.data);
        xrltStringClear(&b);
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

        int i;

        for (i = 0; i < 1; i++) {
        ctx = xrltContextCreate(sheet, NULL);
        ret = xrltTransform(ctx, NULL);
        printData(ctx);
        while (!((ret & XRLT_STATUS_DONE) | (ret & XRLT_STATUS_ERROR))) {
            ret = xrltTransform(ctx, NULL);
            printData(ctx);
        }

            xrltTransformValue  v;
            v.type = XRLT_PROCESS_SUBREQUEST_BODY;
            v.id = 2;
            v.data.len = 10;
            v.data.data = "abcde12345";
            ret = xrltTransform(ctx, &v);
        xrltContextFree(ctx);
        }


        xrltRequestsheetFree(sheet);
    }
    xrltCleanup();
    return 0;
}
