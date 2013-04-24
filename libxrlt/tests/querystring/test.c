#include <xrlt.h>
#include "querystring.h"

#include "../test.h"

#define TEST_BUFFER_SIZE   2048

void test_xrltQueryStringParser(const char *in, const char *out)
{
    xmlDocPtr                  doc;
    xmlNodePtr                 node;
    xrltQueryStringParserPtr   parser;
    FILE                      *infile;
    FILE                      *outfile;
    char                       buf[TEST_BUFFER_SIZE];
    char                       buf2[TEST_BUFFER_SIZE];
    size_t                     i;
    xmlChar                   *ret;

    doc = xmlNewDoc(NULL);
    node = xmlNewDocNode(doc, NULL, (const xmlChar *)"root", NULL);
    xmlDocSetRootElement(doc, node);

    parser = xrltQueryStringParserInit(node);

    outfile = fopen(out, "r");

    if (outfile == NULL) {
        snprintf(buf, TEST_BUFFER_SIZE - 1, "Failed to open '%s' file", out);
        xrltTestFailurePush(buf);
        TEST_FAILED;
    }

    memset(buf, 0, TEST_BUFFER_SIZE);
    fread(buf, 1, TEST_BUFFER_SIZE - 1, outfile);
    fclose(outfile);

    infile = fopen(in, "r");

    if (infile == NULL) {
        memset(buf2, 0, TEST_BUFFER_SIZE);
        snprintf(buf2, TEST_BUFFER_SIZE - 1, "Failed to open '%s' file", in);
        xrltTestFailurePush(buf2);
        TEST_FAILED;
    }

    memset(buf2, 0, TEST_BUFFER_SIZE);

    while (!feof(infile)) {
        buf2[0] = '\0';
        fgets(buf2, TEST_BUFFER_SIZE, infile);
        i = strlen(buf2);
        if (i > 0) {
            i--;
        }
        xrltQueryStringParserFeed(parser, buf2, i, feof(infile) ? TRUE : FALSE);

    }

    fclose(infile);

    xrltQueryStringParserFree(parser);

    xmlDocDumpFormatMemory(doc, &ret, (int *)&i, 1);
    xmlFreeDoc(doc);

    ASSERT_CSTR((char *)ret, buf);

    xmlFree(ret);

    TEST_PASSED;
}


int main()
{
    test_xrltQueryStringParser("querystring/test1.in", "querystring/test1.out");
    test_xrltQueryStringParser("querystring/test2.in", "querystring/test2.out");
    test_xrltQueryStringParser("querystring/test3.in", "querystring/test3.out");

    xrltTestFailurePrint();

    xmlCleanupParser();

    return 0;
}
