#include <xrlt.h>
#include <xrlterror.h>

#include "../test.h"

// Assume this is enough buffer size. We won't check for overflows at the
// moment. Just increase the buffer size if it's not big enough to cover each
// test.
#define TEST_BUFFER_SIZE   8192

char    error_buf[TEST_BUFFER_SIZE];
int     error_buf_pos;


static void
xrltTestErrorFunc(void *ctx, const char *msg, ...)
{
    va_list   args;

    va_start(args, msg);
    error_buf_pos += vsnprintf(
        error_buf + error_buf_pos, TEST_BUFFER_SIZE - error_buf_pos, msg, args
    );
    va_end(args);
}


char *
dumpResult(xrltContextPtr ctx, int ret, char *out)
{
    xrltString               s;
    xrltLogType              t;
    xrltHTTPMethod           m;
    xrltSubrequestDataType   type;
    xrltString               url, q, b, n, v;
    xrltHeaderList           header;
    size_t                   id;
    char                     buf[TEST_BUFFER_SIZE];

    memset(buf, 0, TEST_BUFFER_SIZE);

    if (ret == XRLT_STATUS_UNKNOWN) {
        sprintf(buf, "XRLT_STATUS_UNKNOWN\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_ERROR) {
        sprintf(buf, "XRLT_STATUS_ERROR\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_WAITING) {
        sprintf(buf, "XRLT_STATUS_WAITING\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_DONE) {
        sprintf(buf, "XRLT_STATUS_DONE\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_NEED_HEADER) {
        sprintf(buf, "XRLT_STATUS_NEED_HEADER\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_HEADER) {
        sprintf(buf, "XRLT_STATUS_HEADER\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_SUBREQUEST) {
        sprintf(buf, "XRLT_STATUS_SUBREQUEST\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_CHUNK) {
        sprintf(buf, "XRLT_STATUS_CHUNK\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_LOG) {
        sprintf(buf, "XRLT_STATUS_LOG\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    if (ret & XRLT_STATUS_REFUSE_SUBREQUEST) {
        sprintf(buf, "XRLT_STATUS_REFUSE_SUBREQUEST\n");
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    while (xrltHeaderListShift(&ctx->header, &n, &v)) {
        sprintf(buf, "header: %s %s\n", n.data, v.data);

        xrltStringClear(&n);
        xrltStringClear(&v);

        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    while (xrltNeedHeaderListShift(&ctx->needHeader, &id, &n)) {
        sprintf(buf, "need header: %d %s\n", (int)id, n.data);
        xrltStringClear(&n);

        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    while (xrltLogListShift(&ctx->log, &t, &s)) {
        sprintf(buf, "log: %d %s\n", t, s.data);
        xrltStringClear(&s);

        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    while (xrltChunkListShift(&ctx->chunk, &s)) {
        sprintf(buf, "chunk: %s\n", s.data);
        xrltStringClear(&s);

        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    while (xrltSubrequestListShift(&ctx->sr, &id, &m, &type, &header, &url, &q,
                                   &b))
    {
        sprintf(buf, "sr id: %d\n", (int)id);
        sprintf(out, "%s", buf);
        out += strlen(buf);

        switch (m) {
            case XRLT_METHOD_GET:
                sprintf(buf, "sr method: GET\n");
                break;
            case XRLT_METHOD_HEAD:
                sprintf(buf, "sr method: HEAD\n");
                break;
            case XRLT_METHOD_POST:
                sprintf(buf, "sr method: POST\n");
                break;
            case XRLT_METHOD_PUT:
                sprintf(buf, "sr method: PUT\n");
                break;
            case XRLT_METHOD_DELETE:
                sprintf(buf, "sr method: DELETE\n");
                break;
            case XRLT_METHOD_TRACE:
                sprintf(buf, "sr method: TRACE\n");
                break;
            case XRLT_METHOD_OPTIONS:
                sprintf(buf, "sr method: OPTIONS\n");
                break;
        }
        sprintf(out, "%s", buf);
        out += strlen(buf);

        switch (type) {
            case XRLT_SUBREQUEST_DATA_XML:
                sprintf(buf, "sr type: XML\n");
                break;
            case XRLT_SUBREQUEST_DATA_JSON:
                sprintf(buf, "sr type: JSON\n");
                break;
            case XRLT_SUBREQUEST_DATA_TEXT:
                sprintf(buf, "sr type: TEXT\n");
                break;
            case XRLT_SUBREQUEST_DATA_QUERYSTRING:
                sprintf(buf, "sr type: QUERYSTRING\n");
                break;
        }
        sprintf(out, "%s", buf);
        out += strlen(buf);

        while (xrltHeaderListShift(&header, &n, &v)) {
            sprintf(buf, "sr header: %s: %s\n", n.data, v.data);
            xrltStringClear(&n);
            xrltStringClear(&v);

            sprintf(out, "%s", buf);
            out += strlen(buf);
        }

        sprintf(buf, "sr url: %s\n", url.data);
        xrltStringClear(&url);
        sprintf(out, "%s", buf);
        out += strlen(buf);

        sprintf(buf, "sr query: %s\n", q.data);
        xrltStringClear(&q);
        sprintf(out, "%s", buf);
        out += strlen(buf);

        sprintf(buf, "sr body: %s\n", b.data);
        xrltStringClear(&b);
        sprintf(out, "%s", buf);
        out += strlen(buf);
    }

    return out;
}


void
test_xrltTransform(const char *xrl, const char *in, const char *out)
{
    xmlDocPtr                doc;
    xrltRequestsheetPtr      sheet = NULL;
    FILE                    *infile;
    FILE                    *outfile;
    char                     indata[TEST_BUFFER_SIZE];
    char                     outdata[TEST_BUFFER_SIZE];
    char                    *pos;

    xrltContextPtr           ctx;
    char                     data[TEST_BUFFER_SIZE];
    size_t                   id;
    int                      i, j, k, l;
    xrltTransformValue       val;

    memset(indata, 0, TEST_BUFFER_SIZE);
    memset(outdata, 0, TEST_BUFFER_SIZE);
    memset(data, 0, TEST_BUFFER_SIZE);


    memset(error_buf, 0, TEST_BUFFER_SIZE);
    error_buf_pos = 0;
    xrltSetGenericErrorFunc(NULL, xrltTestErrorFunc);

    doc = xmlReadFile(xrl, NULL, 0);

    if (doc == NULL) {
        xrltTestFailurePush(outdata);
        TEST_FAILED;
    }

    sheet = xrltRequestsheetCreate(doc);

    if (sheet == NULL) {
        xmlFreeDoc(doc);
        xrltTestFailurePush(outdata);
        TEST_FAILED;
    }

    outfile = fopen(out, "r");
    if (outfile == NULL) {
        snprintf(
            outdata, TEST_BUFFER_SIZE - 1, "Failed to open '%s' file", out
        );
        xrltTestFailurePush(outdata);
        TEST_FAILED;
    }
    fread(outdata, 1, TEST_BUFFER_SIZE - 1, outfile);
    fclose(outfile);

    infile = fopen(in, "r");
    if (infile == NULL) {
        snprintf(
            indata, TEST_BUFFER_SIZE - 1, "Failed to open '%s' file", in
        );
        xrltTestFailurePush(indata);
        TEST_FAILED;
    }

    ctx = xrltContextCreate(sheet);
    ASSERT_NOT_NULL(ctx);

    pos = indata;

    id = 0;
    memset(&val, 0, sizeof(xrltTransformValue));

    while (fscanf(infile, "id:%d, type:%d, last:%d, error:%d, data:",
                  (int *)(&id), &j, &k, &l) == 4)
    {
        fgets(data, TEST_BUFFER_SIZE - 1, infile);

        if (j > 0) {
            i = strlen(data);
            if (j == XRLT_PROCESS_HEADER) {
                val.type = XRLT_PROCESS_HEADER;
            } else if (j == XRLT_PROCESS_BODY) {
                val.type = XRLT_PROCESS_BODY;
            } else {
                xrltTestFailurePush((char *)"Unexpected 'type' value");
                TEST_FAILED;
            }

            if (k == 0) {
                val.last = FALSE;
            } else if (k == 1) {
                val.last = TRUE;
            } else {
                xrltTestFailurePush((char *)"Unexpected 'last' value");
                TEST_FAILED;
            }

            if (i > 1) {
                data[i - 1] = '\0';
                val.data.len = (size_t)(i - 1);
                val.data.data = data;
            } else {
                val.data.len = 0;
                val.data.data = NULL;
            }

            val.error = l == 0 ? FALSE : TRUE;

            i = xrltTransform(ctx, id, &val);
        } else {
            i = xrltTransform(ctx, 0, NULL);
        }

        pos = dumpResult(ctx, i, pos);
    }
    fclose(infile);

    //xmlDocFormatDump(stderr, ctx->responseDoc, 1);


    ASSERT_CSTR(indata, outdata);

    xrltContextFree(ctx);
    xrltRequestsheetFree(sheet);

    TEST_PASSED;
}


int main()
{
    xmlInitParser();
    xrltInit();

    test_xrltTransform("transform/test1.xrl", "transform/test1.in", "transform/test1.out");
    test_xrltTransform("transform/test2.xrl", "transform/test2.in", "transform/test2.out");
    test_xrltTransform("transform/test3.xrl", "transform/test3.in", "transform/test3.out");
    test_xrltTransform("transform/test4.xrl", "transform/test4.in", "transform/test4.out");
    test_xrltTransform("transform/test5.xrl", "transform/test5.in", "transform/test5.out");
    test_xrltTransform("transform/test6.xrl", "transform/test6.in", "transform/test6.out");
    test_xrltTransform("transform/test7.xrl", "transform/test7.in", "transform/test7.out");
    test_xrltTransform("transform/test8.xrl", "transform/test8.in", "transform/test8.out");

    xrltCleanup();
    xmlCleanupParser();

    xrltTestFailurePrint();

    return 0;
}
