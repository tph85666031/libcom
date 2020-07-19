#include "bcp.h"

void bcp_kv_unit_test_suit(void** state)
{
    bcp_file_remove("./kv_test.db");
    bcp_kv_set("./kv_test.db", "string", "1234567");
    std::string val_str = bcp_kv_get_string("./kv_test.db", "string", NULL);
    ASSERT_STR_EQUAL(val_str.c_str(), "1234567");

    bcp_kv_set("./kv_test.db", "string", "142cger");
    val_str = bcp_kv_get_string("./kv_test.db", "string", NULL);
    ASSERT_STR_EQUAL(val_str.c_str(), "142cger");

    uint8 buf[7];
    for (int i = 0; i < sizeof(buf); i++)
    {
        buf[i] = i;
    }
    bcp_kv_set("./kv_test.db", "bytes", buf, sizeof(buf));
    ByteArray bytes = bcp_kv_get_bytes("./kv_test.db", "bytes");
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

    ASSERT_TRUE(bcp_kv_set("./kv_test.db", "val_bool", val_bool));
    bcp_kv_set("./kv_test.db", "val_int8", val_int8);
    bcp_kv_set("./kv_test.db", "val_int16", val_int16);
    bcp_kv_set("./kv_test.db", "val_int32", val_int32);
    bcp_kv_set("./kv_test.db", "val_int64", val_int64);
    bcp_kv_set("./kv_test.db", "val_uint8", val_uint8);
    bcp_kv_set("./kv_test.db", "val_uint16", val_uint16);
    bcp_kv_set("./kv_test.db", "val_uint32", val_uint32);
    bcp_kv_set("./kv_test.db", "val_uint64", val_uint64);

    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_bool"));
    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_int8"));
    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_int16"));
    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_int64"));
    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_uint8"));
    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_uint16"));
    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_uint32"));
    ASSERT_TRUE(bcp_kv_exist("./kv_test.db", "val_uint64"));

    ASSERT_TRUE(bcp_kv_get_bool("./kv_test.db", "val_bool"));
    ASSERT_INT_EQUAL(-8, bcp_kv_get_int8("./kv_test.db", "val_int8"));
    ASSERT_INT_EQUAL(-16, bcp_kv_get_int8("./kv_test.db", "val_int16"));
    ASSERT_INT_EQUAL(-32, bcp_kv_get_int8("./kv_test.db", "val_int32"));
    ASSERT_INT_EQUAL(-64, bcp_kv_get_int8("./kv_test.db", "val_int64"));
    ASSERT_INT_EQUAL(8, bcp_kv_get_int8("./kv_test.db", "val_uint8"));
    ASSERT_INT_EQUAL(16, bcp_kv_get_int8("./kv_test.db", "val_uint16"));
    ASSERT_INT_EQUAL(32, bcp_kv_get_int8("./kv_test.db", "val_uint32"));
    ASSERT_INT_EQUAL(64, bcp_kv_get_int8("./kv_test.db", "val_uint64"));

    ASSERT_INT_EQUAL(11, bcp_kv_get_all_keys("./kv_test.db").size());
    ASSERT_INT_EQUAL(11, bcp_kv_count("./kv_test.db"));

    std::vector<KVResult> vals = bcp_kv_get_tail("./kv_test.db", 1);
    ASSERT_INT_EQUAL(vals.size(), 1);
    ASSERT_STR_EQUAL(vals[0].key.c_str(), "val_uint64");
    ASSERT_INT_EQUAL(strtoull((char*)vals[0].data, NULL, 10), 64);

    vals = bcp_kv_get_front("./kv_test.db", 1);
    ASSERT_INT_EQUAL(vals.size(), 1);
    ASSERT_STR_EQUAL(vals[0].key.c_str(), "string");
    ASSERT_STR_EQUAL("142cger", (char*)vals[0].data);
    bcp_file_remove("./kv_test.db");
}
