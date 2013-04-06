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
        xrltRequestsheetFree(sheet);
    }
    xrltCleanup();
    return 0;
}
