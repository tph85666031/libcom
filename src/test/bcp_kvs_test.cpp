#include "bcp.h"

void kvs_test_thread(std::string file)
{
    for (int i = 0; i < 1000; i++)
    {
        bcp_kvs_set(file.c_str(), "key", i);
    }
}

void bcp_kvs_unit_test_suit(void** state)
{
    TIME_COST();
    bcp_file_remove("./kvs_test.db");
    bcp_kvs_set("./kvs_test.db", "string", "1234567");
    std::string val_str = bcp_kvs_get_string("./kvs_test.db", "string", NULL);
    ASSERT_STR_EQUAL(val_str.c_str(), "1234567");

    bcp_kvs_set("./kvs_test.db", "string", "142cger");
    val_str = bcp_kvs_get_string("./kvs_test.db", "string", NULL);
    ASSERT_STR_EQUAL(val_str.c_str(), "142cger");

    uint8 buf[7];
    for (int i = 0; i < sizeof(buf); i++)
    {
        buf[i] = i;
    }
    bcp_kvs_set("./kvs_test.db", "bytes", buf, sizeof(buf));
    ByteArray bytes = bcp_kvs_get_bytes("./kvs_test.db", "bytes");
    ASSERT_MEM_EQUAL(bytes.getData(), buf, sizeof(buf));

    bool val_bool = true;
    int8 val_int8 = -8;
    int16 val_int16 = -16;
    int32 val_int32 = -32;
    int64 val_int64 = -64;
    uint8 val_uint8 = 8;
    uint16 val_uint16 = 16;
    uint32 val_uint32 = 32;
    uint64 val_uint64 = 64;
    double val_double = 12345.4654;

    bcp_kvs_set("./kvs_test.db", "val_double", val_double);
    bcp_kvs_set("./kvs_test.db", "val_bool", val_bool);
    bcp_kvs_set("./kvs_test.db", "val_int8", val_int8);
    bcp_kvs_set("./kvs_test.db", "val_int16", val_int16);
    bcp_kvs_set("./kvs_test.db", "val_int32", val_int32);
    bcp_kvs_set("./kvs_test.db", "val_int64", val_int64);
    bcp_kvs_set("./kvs_test.db", "val_uint8", val_uint8);
    bcp_kvs_set("./kvs_test.db", "val_uint16", val_uint16);
    bcp_kvs_set("./kvs_test.db", "val_uint32", val_uint32);
    bcp_kvs_set("./kvs_test.db", "val_uint64", val_uint64);

    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_int8"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_bool"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_int16"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_int64"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_uint8"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_uint16"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_uint32"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_uint64"));
    ASSERT_TRUE(bcp_kvs_exist("./kvs_test.db", "val_double"));

    ASSERT_FLOAT_EQUAL(12345.4654, bcp_kvs_get_double("./kvs_test.db", "val_double"));
    ASSERT_TRUE(bcp_kvs_get_bool("./kvs_test.db", "val_bool"));
    ASSERT_INT_EQUAL(-8, bcp_kvs_get_int8("./kvs_test.db", "val_int8"));
    ASSERT_INT_EQUAL(-16, bcp_kvs_get_int8("./kvs_test.db", "val_int16"));
    ASSERT_INT_EQUAL(-32, bcp_kvs_get_int8("./kvs_test.db", "val_int32"));
    ASSERT_INT_EQUAL(-64, bcp_kvs_get_int8("./kvs_test.db", "val_int64"));
    ASSERT_INT_EQUAL(8, bcp_kvs_get_int8("./kvs_test.db", "val_uint8"));
    ASSERT_INT_EQUAL(16, bcp_kvs_get_int8("./kvs_test.db", "val_uint16"));
    ASSERT_INT_EQUAL(32, bcp_kvs_get_int8("./kvs_test.db", "val_uint32"));
    ASSERT_INT_EQUAL(64, bcp_kvs_get_int8("./kvs_test.db", "val_uint64"));

    ASSERT_INT_EQUAL(12, bcp_kvs_get_all_keys("./kvs_test.db").size());
    ASSERT_INT_EQUAL(12, bcp_kvs_count("./kvs_test.db"));

    std::vector<KVResult> vals = bcp_kvs_get_tail("./kvs_test.db", 1);
    ASSERT_INT_EQUAL(vals.size(), 1);
    ASSERT_STR_EQUAL(vals[0].key.c_str(), "val_uint64");
    ASSERT_INT_EQUAL(strtoull((char*)vals[0].data, NULL, 10), 64);

    vals = bcp_kvs_get_front("./kvs_test.db", 1);
    ASSERT_INT_EQUAL(vals.size(), 1);
    ASSERT_STR_EQUAL(vals[0].key.c_str(), "string");
    ASSERT_STR_EQUAL("142cger", (char*)vals[0].data);

    bcp_file_remove("./kvs_test.db");

    Serializer s;
    uint64 xx = 100;
    s.append(xx);

    Message msg;
    msg.set("bytes", s.toBytes());

    std::thread t1(kvs_test_thread, "./kvs.db");
    std::thread t2(kvs_test_thread, "./kvs.db");
    std::thread t3(kvs_test_thread, "./kvs.db");

    if (t1.joinable())
    {
        t1.join();
    }
    if (t2.joinable())
    {
        t2.join();
    }
    if (t3.joinable())
    {
        t3.join();
    }
    bcp_file_remove("./kvs.db");
}
