#include "../../transform.h"

#include "../test.h"


int main()
{
    xmlDocPtr             doc;
    xrltRequestsheetPtr   sheet;
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
        int ret;
        xrltContextPtr   ctx;

        ctx = xrltContextCreate(sheet, NULL);

        ret = xrltTransform(ctx, NULL);
        //while (!(ret & XRLT_STATUS_DONE) | (ret & XRLT_STATUS_ERROR)) {
        //    ret = xrltTransform(ctx, NULL);
        //}

        xrltContextFree(ctx);
        xrltRequestsheetFree(sheet);
    }
    xrltCleanup();
    return 0;
}
