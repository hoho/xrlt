#include "../../transform.h"

#include "../test.h"
#include "xrltstruct.h"
#include "xrlt.h"


void
printData(xrltContextPtr ctx)
{
    xrltString               s;
    xrltLogType              t;
    xrltString               url, q, b, n, v;
    xrltSubrequestDataType   type;
    xrltHTTPMethod           m;
    xrltHeaderList           header;
    size_t                   id;

    while (xrltLogListShift(&ctx->log, &t, &s)) {
        printf("log: %d %s\n", t, s.data);
        xrltStringClear(&s);
    }

    while (xrltChunkListShift(&ctx->chunk, &s)) {
        printf("chunk: %s\n", s.data);
        xrltStringClear(&s);
    }

    while (xrltSubrequestListShift(&ctx->sr, &id, &m, &type, &header,
                                   &url, &q, &b))
    {
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
        ctx = xrltContextCreate(sheet);
        ret = xrltTransform(ctx, 0, NULL);
        printData(ctx);
        while (!((ret & XRLT_STATUS_DONE) | (ret & XRLT_STATUS_ERROR))) {
            ret = xrltTransform(ctx, 0, NULL);
            printData(ctx);
            printf("333\n");
            if (ret == XRLT_STATUS_WAITING) {
                break;
            }
        }
            printf("3555\n");

            xrltTransformValue  v;
            v.type = XRLT_PROCESS_BODY;
            v.data.len = 10;
            v.last = TRUE;
            v.data.data = "abcde12345";
            ret = xrltTransform(ctx, 2, &v);

            v.data.data = "09876poiuy";
            ret = xrltTransform(ctx, 1, &v);

        xrltContextFree(ctx);
        }


        xrltRequestsheetFree(sheet);
    }
    xrltCleanup();
    return 0;
}
