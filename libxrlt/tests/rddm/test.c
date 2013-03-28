#include <xrlt.h>
#include <rddm.h>

#include "../test.h"

#define TEST_BUFFER_SIZE   2048

void test_xrltRDDM(char *in, char *out)
{
    xmlDocPtr         doc;
    xmlNodePtr        node;
    xrltJSON2XMLPtr   json2xml;
    FILE             *infile;
    FILE             *outfile;
    char              buf[TEST_BUFFER_SIZE];
    char              chunk[32];
    size_t            i;
    xrltBool          ok;
    xmlChar          *ret;

    doc = xmlNewDoc(NULL);
    node = xmlNewDocNode(doc, NULL, (const xmlChar *)"root", NULL);

    xmlNewNs(node, XRLT_NS, (const xmlChar *)"xrl");

    xmlDocSetRootElement(doc, node);

    json2xml = xrltJSON2XMLInit(node);

    memset(buf, 0, TEST_BUFFER_SIZE);

    outfile = fopen(out, "rb");

    if (outfile == NULL) {
        snprintf(buf, TEST_BUFFER_SIZE - 1, "Failed to open '%s' file", out);
        xrltTestFailurePush(buf);
        TEST_FAILED;
    }

    fread(buf, 1, TEST_BUFFER_SIZE - 1, outfile);
    fclose(outfile);

    infile = fopen(in, "rb");

    if (infile == NULL) {
        memset(buf, 0, TEST_BUFFER_SIZE);
        snprintf(buf, TEST_BUFFER_SIZE - 1, "Failed to open '%s' file", in);
        xrltTestFailurePush(buf);
        TEST_FAILED;
    }

    memset(chunk, 0, 32);
    i = fread(chunk, 1, 31, infile);
    ok = i > 0 ? TRUE : FALSE;

    while (i > 0) {
        if (!xrltJSON2XMLFeed(json2xml, (xmlChar *)chunk, strlen(chunk))) {
            xmlChar  *err;

            ok = FALSE;
            err = xrltJSON2XMLGetError(json2xml,
                                       (xmlChar *)chunk, strlen(chunk));
            memset(buf, 0, TEST_BUFFER_SIZE);
            snprintf(buf, TEST_BUFFER_SIZE - 1,
                     "Error: %s\n", (const char *)err);
            xmlFree(err);
            xrltTestFailurePush(buf);
            break;
        }

        memset(chunk, 0, 32);
        i = fread(chunk, 1, 31, infile);
    }

    fclose(infile);

    if (!ok) {
        TEST_FAILED;
    }

    xrltJSON2XMLFree(json2xml);

    xmlDocDumpFormatMemory(doc, &ret, (int *)&i, 1);
    xmlFreeDoc(doc);

    ASSERT_CSTR((char *)ret, buf);

    xmlFree(ret);

    TEST_PASSED;
}


int main()
{
    test_xrltRDDM("rddm/test1.in.json", "rddm/test1.out.xml");
    test_xrltRDDM("rddm/test2.in.json", "rddm/test2.out.xml");
    test_xrltRDDM("rddm/test3.in.json", "rddm/test3.out.xml");
    test_xrltRDDM("rddm/test4.in.json", "rddm/test4.out.xml");
    test_xrltRDDM("rddm/test5.in.json", "rddm/test5.out.xml");
    test_xrltRDDM("rddm/test6.in.json", "rddm/test6.out.xml");
    test_xrltRDDM("rddm/test7.in.json", "rddm/test7.out.xml");
    test_xrltRDDM("rddm/test8.in.json", "rddm/test8.out.xml");
    test_xrltRDDM("rddm/test9.in.json", "rddm/test9.out.xml");

    xrltTestFailurePrint();
    return 0;
}
