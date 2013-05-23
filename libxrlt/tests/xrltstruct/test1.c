#include <xrltstruct.h>

#include "../test.h"


void test_xrltHeaderList(void)
{
    xrltHeaderList   h;
    xrltString       name1, name2, val1, val2, name, val;
    xrltHeaderType   ht;

    xrltHeaderListInit(&h);
    ASSERT_NULL(h.first);
    ASSERT_NULL(h.last);

    xrltStringInit(&name1, "name1");
    xrltStringInit(&name2, "name2");
    xrltStringInit(&val1, "value1");
    xrltStringInit(&val2, "value2");

    ASSERT_STR(name1, "name1");

    xrltHeaderListPush(&h, XRLT_HEADER_TYPE_HEADER, &name1, &val1);
    ASSERT_NOT_NULL(h.first);
    ASSERT_TRUE(h.first == h.last);
    ASSERT_STR(h.first->name, "name1");
    ASSERT_STR(h.first->val, "value1");

    xrltHeaderListPush(&h, XRLT_HEADER_TYPE_COOKIE, &name2, &val2);
    ASSERT_TRUE(h.first != h.last);

    xrltHeaderListClear(&h);
    ASSERT_NULL(h.first);
    ASSERT_NULL(h.last);

    xrltHeaderListPush(&h, XRLT_HEADER_TYPE_HEADER, &name1, &val1);
    xrltHeaderListPush(&h, XRLT_HEADER_TYPE_COOKIE, &name2, &val2);
    xrltHeaderListPush(&h, XRLT_HEADER_TYPE_HEADER, &name1, &val1);

    ASSERT_FALSE(xrltHeaderListPush(NULL, XRLT_HEADER_TYPE_HEADER, &name1,
                                    &val1));
    ASSERT_FALSE(xrltHeaderListPush(&h, XRLT_HEADER_TYPE_HEADER, NULL, &val1));
    ASSERT_FALSE(xrltHeaderListPush(&h, XRLT_HEADER_TYPE_HEADER, &name1, NULL));

    ASSERT_FALSE(xrltHeaderListShift(NULL, &ht, &name1, &val1));
    ASSERT_FALSE(xrltHeaderListShift(&h, &ht, NULL, &val1));
    ASSERT_FALSE(xrltHeaderListShift(&h, &ht, &name1, NULL));
    ASSERT_FALSE(xrltHeaderListShift(&h, NULL, &name1, &val1));

    ASSERT_TRUE(xrltHeaderListShift(&h, &ht, &name, &val));
    ASSERT_INT(ht, XRLT_HEADER_TYPE_HEADER);
    ASSERT_STR(name, "name1");
    ASSERT_STR(val, "value1");
    xrltStringClear(&name);
    xrltStringClear(&val);

    ASSERT_TRUE(xrltHeaderListShift(&h, &ht, &name, &val));
    ASSERT_INT(ht, XRLT_HEADER_TYPE_COOKIE);
    ASSERT_STR(name, "name2");
    ASSERT_STR(val, "value2");
    xrltStringClear(&name);
    xrltStringClear(&val);

    ASSERT_TRUE(xrltHeaderListShift(&h, &ht, &name, &val));
    ASSERT_INT(ht, XRLT_HEADER_TYPE_HEADER);
    ASSERT_STR(name, "name1");
    ASSERT_STR(val, "value1");
    xrltStringClear(&name);
    xrltStringClear(&val);

    ASSERT_FALSE(xrltHeaderListShift(&h, &ht, &name, &val));

    xrltStringClear(&name1);
    xrltStringClear(&name2);
    xrltStringClear(&val1);
    xrltStringClear(&val2);

    TEST_PASSED;
}


void test_xrltSubrequestList(void)
{
    xrltSubrequestList       sr;
    xrltHeaderList           h;
    xrltString               n, v;
    size_t                   id;
    xrltSubrequestDataType   type;
    xrltHTTPMethod           m;
    xrltString               url1, url2, url3, url;
    xrltString               q1, q2, q;
    xrltString               b1, b2, b;
    xrltHeaderType           ht;

    xrltSubrequestListInit(&sr);

    xrltHeaderListInit(&h);
    xrltStringInit(&n, "hname");
    xrltStringInit(&v, "hvalue");
    xrltHeaderListPush(&h, FALSE, &n, &v);
    xrltHeaderListPush(&h, TRUE, &n, &v);
    xrltStringClear(&n);
    xrltStringClear(&v);

    xrltStringInit(&url1, "url1");
    xrltStringInit(&url2, "url2");
    xrltStringInit(&url3, "url3");

    xrltStringInit(&q1, "querystring1");
    xrltStringInit(&q2, "querystring2");

    xrltStringInit(&b1, "body1");
    xrltStringInit(&b2, "body2");

    ASSERT_TRUE(xrltSubrequestListPush(&sr, 123, XRLT_METHOD_POST,
                                       XRLT_SUBREQUEST_DATA_JSON, &h, &url1,
                                       &q1, &b1));
    ASSERT_TRUE(xrltSubrequestListPush(&sr, 234, XRLT_METHOD_GET,
                                       XRLT_SUBREQUEST_DATA_XML, NULL, &url2,
                                       &q2, &b2));
    ASSERT_TRUE(xrltSubrequestListPush(&sr, 345, XRLT_METHOD_HEAD,
                                       XRLT_SUBREQUEST_DATA_TEXT, NULL, &url3,
                                       NULL, NULL));

    ASSERT_NULL(h.first);
    ASSERT_NULL(h.last);

    ASSERT_TRUE(xrltSubrequestListShift(&sr, &id, &m, &type, &h, &url, &q, &b));
    ASSERT_INT(id, 123);
    ASSERT_INT(m, XRLT_METHOD_POST);
    ASSERT_INT(type, XRLT_SUBREQUEST_DATA_JSON);
    ASSERT_TRUE(xrltHeaderListShift(&h, &ht, &n, &v));
    ASSERT_INT(ht, XRLT_HEADER_TYPE_HEADER);
    ASSERT_STR(n, "hname");
    ASSERT_STR(v, "hvalue");
    xrltStringClear(&n);
    xrltStringClear(&v);

    ASSERT_TRUE(xrltHeaderListShift(&h, &ht, &n, &v));
    ASSERT_INT(ht, XRLT_HEADER_TYPE_COOKIE);
    ASSERT_STR(n, "hname");
    ASSERT_STR(v, "hvalue");
    xrltStringClear(&n);
    xrltStringClear(&v);

    ASSERT_FALSE(xrltHeaderListShift(&h, &ht, &n, &v));

    ASSERT_STR(url, "url1");
    ASSERT_STR(q, "querystring1");
    ASSERT_STR(b, "body1");
    xrltStringClear(&url);
    xrltStringClear(&q);
    xrltStringClear(&b);

    ASSERT_TRUE(xrltSubrequestListShift(&sr, &id, &m, &type, &h, &url, &q, &b));
    ASSERT_INT(id, 234);
    ASSERT_INT(m, XRLT_METHOD_GET);
    ASSERT_INT(type, XRLT_SUBREQUEST_DATA_XML);
    ASSERT_NULL(h.first);
    ASSERT_NULL(h.last);
    ASSERT_STR(url, "url2");
    ASSERT_STR(q, "querystring2");
    ASSERT_STR(b, "body2");
    xrltStringClear(&url);
    xrltStringClear(&q);
    xrltStringClear(&b);

    ASSERT_TRUE(xrltSubrequestListShift(&sr, &id, &m, &type, &h, &url, &q, &b));
    ASSERT_INT(id, 345);
    ASSERT_INT(m, XRLT_METHOD_HEAD);
    ASSERT_INT(type, XRLT_SUBREQUEST_DATA_TEXT);
    ASSERT_NULL(h.first);
    ASSERT_NULL(h.last);
    ASSERT_STR(url, "url3");
    ASSERT_NULL(q.data);
    ASSERT_NULL(b.data);
    xrltStringClear(&url);

    ASSERT_FALSE(xrltSubrequestListShift(&sr, &id, &m, &type, &h, &url, &q, &b));

    xrltStringInit(&n, "hname");
    xrltStringInit(&v, "hvalue");
    xrltHeaderListPush(&h, XRLT_HEADER_TYPE_HEADER, &n, &v);
    xrltHeaderListPush(&h, XRLT_HEADER_TYPE_HEADER, &n, &v);
    xrltStringClear(&n);
    xrltStringClear(&v);
    ASSERT_TRUE(xrltSubrequestListPush(&sr, 123, XRLT_METHOD_HEAD,
                                       XRLT_SUBREQUEST_DATA_TEXT, &h, &url1,
                                       &q1, &b1));
    ASSERT_TRUE(xrltSubrequestListPush(&sr, 234, XRLT_METHOD_HEAD,
                                       XRLT_SUBREQUEST_DATA_TEXT, NULL, &url2,
                                       &q2, &b2));
    ASSERT_TRUE(xrltSubrequestListPush(&sr, 345, XRLT_METHOD_HEAD,
                                       XRLT_SUBREQUEST_DATA_TEXT, NULL, &url3,
                                       NULL, NULL));

    ASSERT_FALSE(xrltSubrequestListPush(NULL, 123, XRLT_METHOD_HEAD,
                                        XRLT_SUBREQUEST_DATA_TEXT, NULL, &url1,
                                        NULL, NULL));
    ASSERT_FALSE(xrltSubrequestListPush(&sr, 0, XRLT_METHOD_HEAD,
                                        XRLT_SUBREQUEST_DATA_TEXT, NULL, &url1,
                                        NULL, NULL));
    ASSERT_FALSE(xrltSubrequestListPush(&sr, 123, XRLT_METHOD_HEAD,
                                        XRLT_SUBREQUEST_DATA_TEXT, NULL, NULL,
                                        NULL, NULL));

    ASSERT_FALSE(xrltSubrequestListShift(NULL, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL));
    ASSERT_FALSE(xrltSubrequestListShift(&sr, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL));
    ASSERT_FALSE(xrltSubrequestListShift(&sr, &id, &m, NULL, NULL, NULL, NULL,
                                         NULL));
    ASSERT_FALSE(xrltSubrequestListShift(&sr, &id, &m, &type, &h, NULL, NULL,
                                         NULL));
    ASSERT_FALSE(xrltSubrequestListShift(&sr, &id, &m, &type, &h, &url, NULL,
                                         NULL));
    ASSERT_FALSE(xrltSubrequestListShift(&sr, &id, &m, &type, &h, &url, &q,
                                         NULL));

    xrltSubrequestListClear(&sr);
    ASSERT_NULL(sr.first);
    ASSERT_NULL(sr.last);

    xrltStringClear(&url1);
    xrltStringClear(&url2);
    xrltStringClear(&url3);
    xrltStringClear(&q1);
    xrltStringClear(&q2);
    xrltStringClear(&b1);
    xrltStringClear(&b2);

    TEST_PASSED;
}


void test_xrltChunkList(void)
{
    xrltChunkList   cl;
    xrltString      c1, c2, c3, c;

    xrltChunkListInit(&cl);
    ASSERT_NULL(cl.first);
    ASSERT_NULL(cl.last);

    xrltStringInit(&c1, "chunk1");
    xrltStringInit(&c2, "chunk2");
    xrltStringInit(&c3, "chunk3");

    ASSERT_TRUE(xrltChunkListPush(&cl, &c1));
    ASSERT_TRUE(xrltChunkListPush(&cl, &c2));
    ASSERT_TRUE(xrltChunkListPush(&cl, &c3));

    ASSERT_TRUE(xrltChunkListShift(&cl, &c));
    ASSERT_STR(c, "chunk1");
    xrltStringClear(&c);

    ASSERT_TRUE(xrltChunkListShift(&cl, &c));
    ASSERT_STR(c, "chunk2");
    xrltStringClear(&c);

    ASSERT_TRUE(xrltChunkListShift(&cl, &c));
    ASSERT_STR(c, "chunk3");
    xrltStringClear(&c);

    ASSERT_FALSE(xrltChunkListShift(&cl, &c));

    ASSERT_NULL(cl.first);
    ASSERT_NULL(cl.last);

    ASSERT_TRUE(xrltChunkListPush(&cl, &c1));
    ASSERT_TRUE(xrltChunkListPush(&cl, &c2));
    ASSERT_TRUE(xrltChunkListPush(&cl, &c3));

    ASSERT_FALSE(xrltChunkListPush(NULL, NULL));
    ASSERT_FALSE(xrltChunkListPush(&cl, NULL));
    ASSERT_FALSE(xrltChunkListPush(NULL, &c));
    ASSERT_FALSE(xrltChunkListShift(NULL, NULL));
    ASSERT_FALSE(xrltChunkListShift(&cl, NULL));
    ASSERT_FALSE(xrltChunkListShift(NULL, &c));

    xrltChunkListClear(&cl);
    ASSERT_NULL(cl.first);
    ASSERT_NULL(cl.last);

    xrltStringClear(&c1);
    xrltStringClear(&c2);
    xrltStringClear(&c3);

    TEST_PASSED;
}


void test_xrltLogList(void)
{
    xrltLogList   ll;
    xrltString    msg1, msg2, msg3, msg;
    xrltLogType   type;

    xrltLogListInit(&ll);
    ASSERT_NULL(ll.first);
    ASSERT_NULL(ll.last);

    xrltStringInit(&msg1, "log1");
    xrltStringInit(&msg2, "log2");
    xrltStringInit(&msg3, "log3");

    ASSERT_TRUE(xrltLogListPush(&ll, XRLT_ERROR, &msg1));
    ASSERT_TRUE(xrltLogListPush(&ll, XRLT_WARNING, &msg2));
    ASSERT_TRUE(xrltLogListPush(&ll, XRLT_INFO, &msg3));

    ASSERT_TRUE(xrltLogListShift(&ll, &type, &msg));
    ASSERT_STR(msg, "log1");
    ASSERT_INT(type, XRLT_ERROR);
    xrltStringClear(&msg);

    ASSERT_TRUE(xrltLogListShift(&ll, &type, &msg));
    ASSERT_STR(msg, "log2");
    ASSERT_INT(type, XRLT_WARNING);
    xrltStringClear(&msg);

    ASSERT_TRUE(xrltLogListShift(&ll, &type, &msg));
    ASSERT_STR(msg, "log3");
    ASSERT_INT(type, XRLT_INFO);
    xrltStringClear(&msg);

    ASSERT_FALSE(xrltLogListShift(&ll, &type, &msg));

    ASSERT_NULL(ll.first);
    ASSERT_NULL(ll.last);

    ASSERT_TRUE(xrltLogListPush(&ll, XRLT_ERROR, &msg1));
    ASSERT_TRUE(xrltLogListPush(&ll, XRLT_WARNING, &msg2));
    ASSERT_TRUE(xrltLogListPush(&ll, XRLT_INFO, &msg3));

    ASSERT_FALSE(xrltLogListPush(NULL, XRLT_DEBUG, NULL));
    ASSERT_FALSE(xrltLogListPush(&ll, XRLT_DEBUG, NULL));
    ASSERT_FALSE(xrltLogListPush(NULL, XRLT_DEBUG, &msg1));
    ASSERT_FALSE(xrltLogListShift(NULL, NULL, NULL));
    ASSERT_FALSE(xrltLogListShift(&ll, NULL, NULL));
    ASSERT_FALSE(xrltLogListShift(&ll, &type, NULL));
    ASSERT_FALSE(xrltLogListShift(&ll, NULL, &msg));

    xrltLogListClear(&ll);
    ASSERT_NULL(ll.first);
    ASSERT_NULL(ll.last);

    xrltStringClear(&msg1);
    xrltStringClear(&msg2);
    xrltStringClear(&msg3);

    TEST_PASSED;
}


int main()
{
    test_xrltHeaderList();
    test_xrltSubrequestList();
    test_xrltChunkList();
    test_xrltLogList();

    xrltTestFailurePrint();
    return 0;
}
