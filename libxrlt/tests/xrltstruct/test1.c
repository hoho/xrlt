#include <xrltstruct.h>

#include "../test.h"


void test_xrltHeaderList(void)
{
    xrltHeaderList   h;
    xrltString       name1, name2, val1, val2, name, val;

    xrltHeaderListInit(&h);
    ASSERT(h.first == NULL)
    ASSERT(h.last == NULL)

    xrltStringInit(&name1, "name1");
    xrltStringInit(&name2, "name2");
    xrltStringInit(&val1, "value1");
    xrltStringInit(&val2, "value2");

    ASSERT(!strcmp(name1.data, "name1"))
    ASSERT(name1.len == 5)

    xrltHeaderListPush(&h, &name1, &val1);
    ASSERT(h.first != NULL)
    ASSERT(h.first == h.last)
    ASSERT(!strcmp(h.first->name.data, "name1"))
    ASSERT(h.first->name.len == 5)
    ASSERT(!strcmp(h.first->value.data, "value1"))
    ASSERT(h.first->value.len == 6)

    xrltHeaderListPush(&h, &name2, &val2);
    ASSERT(h.first != h.last)

    xrltHeaderListClear(&h);
    ASSERT(h.first == NULL)
    ASSERT(h.last == NULL)

    xrltHeaderListPush(&h, &name1, &val1);
    xrltHeaderListPush(&h, &name2, &val2);
    xrltHeaderListPush(&h, &name1, &val1);

    ASSERT(xrltHeaderListShift(&h, &name, &val) == TRUE)
    ASSERT(!strcmp(name.data, "name1"))
    ASSERT(!strcmp(val.data, "value1"))
    xrltStringFree(&name);
    xrltStringFree(&val);

    ASSERT(xrltHeaderListShift(&h, &name, &val) == TRUE)
    ASSERT(!strcmp(name.data, "name2"))
    ASSERT(!strcmp(val.data, "value2"))
    xrltStringFree(&name);
    xrltStringFree(&val);

    ASSERT(xrltHeaderListShift(&h, &name, &val) == TRUE)
    ASSERT(!strcmp(name.data, "name1"))
    ASSERT(!strcmp(val.data, "value1"))
    xrltStringFree(&name);
    xrltStringFree(&val);

    ASSERT(xrltHeaderListShift(&h, &name, &val) == FALSE)

    xrltStringFree(&name1);
    xrltStringFree(&name2);
    xrltStringFree(&val1);
    xrltStringFree(&val2);

    TEST_PASSED;
}


int main()
{
    test_xrltHeaderList();

    xrltTestFailurePrint();
    return 0;
}
