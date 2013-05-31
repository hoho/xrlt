#include <xrlt.h>
#include <xrlterror.h>

#include "../test.h"
#include "xrlt.h"

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
    xrltHeaderOutList        header;
    size_t                   id;
    xrltHeaderOutType        ht;
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

    while (xrltHeaderOutListShift(&ctx->header, &ht, &n, &v)) {
        switch (ht) {
            case XRLT_HEADER_OUT_COOKIE:
                sprintf(buf, "cookie: %s %s\n", n.data, v.data);
                break;
            case XRLT_HEADER_OUT_STATUS:
                sprintf(buf, "status: %s\n", v.data);
                break;
            case XRLT_HEADER_OUT_HEADER:
                sprintf(buf, "header: %s %s\n", n.data, v.data);
                break;
            case XRLT_HEADER_OUT_REDIRECT:
                if (n.data == NULL) {
                    sprintf(buf, "redirect: %s\n", v.data);
                } else {
                    sprintf(buf, "redirect: %s %s\n", n.data, v.data);
                }
                break;
        }

        xrltStringClear(&n);
        xrltStringClear(&v);

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

        while (xrltHeaderOutListShift(&header, &ht, &n, &v)) {
            switch (ht) {
                case XRLT_HEADER_OUT_COOKIE:
                    sprintf(buf, "sr cookie: %s: %s\n", n.data, v.data);
                    break;
                case XRLT_HEADER_OUT_STATUS:
                    sprintf(buf, "sr status: %s\n", v.data);
                    break;
                case XRLT_HEADER_OUT_HEADER:
                    sprintf(buf, "sr header: %s: %s\n", n.data, v.data);
                    break;
                case XRLT_HEADER_OUT_REDIRECT:
                    break;
            }
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

        memset(&val, 0, sizeof(val));

        i = strlen(data);

        if (j == XRLT_TRANSFORM_VALUE_HEADER) {
            val.type = XRLT_TRANSFORM_VALUE_HEADER;
        } else if (j == XRLT_TRANSFORM_VALUE_COOKIE) {
            val.type = XRLT_TRANSFORM_VALUE_COOKIE;
        } else if (j == XRLT_TRANSFORM_VALUE_STATUS) {
            val.type = XRLT_TRANSFORM_VALUE_STATUS;
        } else if (j == XRLT_TRANSFORM_VALUE_QUERYSTRING) {
            val.type = XRLT_TRANSFORM_VALUE_QUERYSTRING;
        } else if (j == XRLT_TRANSFORM_VALUE_BODY) {
            val.type = XRLT_TRANSFORM_VALUE_BODY;
        } else if (j == 100) {
            val.type = XRLT_TRANSFORM_VALUE_EMPTY;
        } else if (j == 0) {
            val.type = XRLT_TRANSFORM_VALUE_ERROR;
        } else {
            xrltTestFailurePush((char *)"Unexpected 'type' value");
            TEST_FAILED;
        }

        if (k == 0) {
            val.bodyval.last = FALSE;
        } else if (k == 1) {
            val.bodyval.last = TRUE;
        } else {
            xrltTestFailurePush((char *)"Unexpected 'last' value");
            TEST_FAILED;
        }

        if (i > 1) {
            data[i - 1] = '\0';

            switch (val.type) {
                case XRLT_TRANSFORM_VALUE_HEADER:
                case XRLT_TRANSFORM_VALUE_COOKIE:
                    val.headerval.name.len = (size_t)(i - 1) / 2;
                    val.headerval.name.data = data;
                    val.headerval.val.len = (size_t)(i - 1) - val.headerval.name.len;
                    val.headerval.val.data = data + val.headerval.name.len;
                    break;

                case XRLT_TRANSFORM_VALUE_STATUS:
                    val.statusval.status = (size_t)atoi(data);
                    break;

                case XRLT_TRANSFORM_VALUE_QUERYSTRING:
                    val.querystringval.val.len = (size_t)(i - 1);
                    val.querystringval.val.data = data;
                    break;

                case XRLT_TRANSFORM_VALUE_BODY:
                    val.bodyval.val.len = (size_t)(i - 1);
                    val.bodyval.val.data = data;
                    break;

                case XRLT_TRANSFORM_VALUE_ERROR:
                case XRLT_TRANSFORM_VALUE_EMPTY:
                    xrltTestFailurePush((char *)"Unexpected type");
                    TEST_FAILED;
            }
        }

        if (l) {
            val.type = XRLT_TRANSFORM_VALUE_ERROR;
        }

        i = xrltTransform(ctx, id, &val);

        pos = dumpResult(ctx, i, pos);
    }
    fclose(infile);

    //xmlDocFormatDump(stderr, ctx->responseDoc, 1);


    ASSERT_CSTR(indata, outdata);

    xrltContextFree(ctx);
    xrltRequestsheetFree(sheet);

    TEST_PASSED;
}


int main(int argc, char *argv[])
{
    xmlInitParser();
    xrltInit();

    if (argc % 3 != 1) {
        fprintf(stderr, "Incorrect number of arguments\n");
        return 1;
    }

    int i;
    for (i = 1; i < argc; i += 3) {
        test_xrltTransform(argv[i], argv[i + 1], argv[i + 2]);
    }

    xrltCleanup();
    xmlCleanupParser();

    xrltTestFailurePrint();

    return 0;
}
