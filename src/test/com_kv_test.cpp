#include "com.h"

void com_kv_unit_test_suit(void** state)
{
    com_file_remove("./kv_test.db");
    com_kv_set("./kv_test.db", "string", "1234567");
    std::string val_str = com_kv_get_string("./kv_test.db", "string", NULL);
    ASSERT_STR_EQUAL(val_str.c_str(), "1234567");

    com_kv_set("./kv_test.db", "string", "142cger");
    val_str = com_kv_get_string("./kv_test.db", "string", NULL);
    ASSERT_STR_EQUAL(val_str.c_str(), "142cger");

    uint8_t buf[7];
    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = i;
    }
    com_kv_set("./kv_test.db", "bytes", buf, sizeof(buf));
    CPPBytes bytes = com_kv_get_bytes("./kv_test.db", "bytes");
    ASSERT_MEM_EQUAL(bytes.getData(), buf, sizeof(buf));

    bool val_bool = true;
    int8_t val_int8 = -8;
    int16_t val_int16 = -16;
    int32_t val_int32 = -32;
    int64_t val_int64 = -64;
    uint8_t val_uint8 = 8;
    uint16_t val_uint16 = 16;
    uint32_t val_uint32 = 32;
    uint64_t val_uint64 = 64;

    ASSERT_TRUE(com_kv_set("./kv_test.db", "val_bool", val_bool));
    com_kv_set("./kv_test.db", "val_int8", val_int8);
    com_kv_set("./kv_test.db", "val_int16", val_int16);
    com_kv_set("./kv_test.db", "val_int32", val_int32);
    com_kv_set("./kv_test.db", "val_int64", val_int64);
    com_kv_set("./kv_test.db", "val_uint8", val_uint8);
    com_kv_set("./kv_test.db", "val_uint16", val_uint16);
    com_kv_set("./kv_test.db", "val_uint32", val_uint32);
    com_kv_set("./kv_test.db", "val_uint64", val_uint64);

    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_bool"));
    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_int8"));
    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_int16"));
    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_int64"));
    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_uint8"));
    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_uint16"));
    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_uint32"));
    ASSERT_TRUE(com_kv_exist("./kv_test.db", "val_uint64"));

    ASSERT_TRUE(com_kv_get_bool("./kv_test.db", "val_bool"));
    ASSERT_INT_EQUAL(-8, com_kv_get_int8("./kv_test.db", "val_int8"));
    ASSERT_INT_EQUAL(-16, com_kv_get_int8("./kv_test.db", "val_int16"));
    ASSERT_INT_EQUAL(-32, com_kv_get_int8("./kv_test.db", "val_int32"));
    ASSERT_INT_EQUAL(-64, com_kv_get_int8("./kv_test.db", "val_int64"));
    ASSERT_INT_EQUAL(8, com_kv_get_int8("./kv_test.db", "val_uint8"));
    ASSERT_INT_EQUAL(16, com_kv_get_int8("./kv_test.db", "val_uint16"));
    ASSERT_INT_EQUAL(32, com_kv_get_int8("./kv_test.db", "val_uint32"));
    ASSERT_INT_EQUAL(64, com_kv_get_int8("./kv_test.db", "val_uint64"));

    ASSERT_INT_EQUAL(11, com_kv_get_all_keys("./kv_test.db").size());
    ASSERT_INT_EQUAL(11, com_kv_count("./kv_test.db"));

    std::vector<KVResult> vals = com_kv_get_tail("./kv_test.db", 1);
    ASSERT_INT_EQUAL(vals.size(), 1);
    ASSERT_STR_EQUAL(vals[0].key.c_str(), "val_uint64");
    ASSERT_INT_EQUAL(strtoull((char*)vals[0].data, NULL, 10), 64);

    vals = com_kv_get_front("./kv_test.db", 1);
    ASSERT_INT_EQUAL(vals.size(), 1);
    ASSERT_STR_EQUAL(vals[0].key.c_str(), "string");
    ASSERT_STR_EQUAL("142cger", (char*)vals[0].data);
    com_file_remove("./kv_test.db");
}
