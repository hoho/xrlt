#include <xrltstruct.h>

#include "../test.h"


void test_xrltHeaderList(void)
{
    xrltHeaderList   h;
    xrltString       name1, name2, val1, val2, name, val;

    xrltHeaderListInit(&h);
    ASSERT_NULL(h.first);
    ASSERT_NULL(h.last);

    xrltStringInit(&name1, "name1");
    xrltStringInit(&name2, "name2");
    xrltStringInit(&val1, "value1");
    xrltStringInit(&val2, "value2");

    ASSERT_STR(name1, "name1");

    xrltHeaderListPush(&h, &name1, &val1);
    ASSERT_NOT_NULL(h.first);
    ASSERT_TRUE(h.first == h.last);
    ASSERT_STR(h.first->name, "name1");
    ASSERT_STR(h.first->value, "value1");

    xrltHeaderListPush(&h, &name2, &val2);
    ASSERT_TRUE(h.first != h.last);

    xrltHeaderListClear(&h);
    ASSERT_NULL(h.first);
    ASSERT_NULL(h.last);

    xrltHeaderListPush(&h, &name1, &val1);
    xrltHeaderListPush(&h, &name2, &val2);
    xrltHeaderListPush(&h, &name1, &val1);

    ASSERT_FALSE(xrltHeaderListPush(NULL, &name1, &val1));
    ASSERT_FALSE(xrltHeaderListPush(&h, NULL, &val1));
    ASSERT_FALSE(xrltHeaderListPush(&h, &name1, NULL));

    ASSERT_FALSE(xrltHeaderListShift(NULL, &name1, &val1));
    ASSERT_FALSE(xrltHeaderListShift(&h, NULL, &val1));
    ASSERT_FALSE(xrltHeaderListShift(&h, &name1, NULL));

    ASSERT_TRUE(xrltHeaderListShift(&h, &name, &val));
    ASSERT_STR(name, "name1");
    ASSERT_STR(val, "value1");
    xrltStringFree(&name);
    xrltStringFree(&val);

    ASSERT_TRUE(xrltHeaderListShift(&h, &name, &val));
    ASSERT_STR(name, "name2");
    ASSERT_STR(val, "value2");
    xrltStringFree(&name);
    xrltStringFree(&val);

    ASSERT_TRUE(xrltHeaderListShift(&h, &name, &val));
    ASSERT_STR(name, "name1");
    ASSERT_STR(val, "value1");
    xrltStringFree(&name);
    xrltStringFree(&val);

    ASSERT_FALSE(xrltHeaderListShift(&h, &name, &val));

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
