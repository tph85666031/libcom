#include "bcp.h"

static const char* mem_db_opt_type[] = {"remove all", "remove", "update", "add", "add all"};

static void mem_cb_test(std::string key, int flag)
{
    LOG_W("%s detected,flag=%s", key.c_str(), mem_db_opt_type[flag + 2]);
}

void bcp_mem_unit_test_suit(void** state)
{
    bcp_mem_add_notify("age", mem_cb_test);
    bcp_mem_add_notify("name", mem_cb_test);
    bcp_mem_add_notify("money", mem_cb_test);
    bcp_mem_set("name", "test");
    bcp_mem_set("name", "test");
    bcp_mem_remove("name");
    bcp_mem_set("name", "test");
    bcp_mem_set("age", 24);
    bcp_mem_set("money", 15792.25);
    bcp_mem_set("country", "china");
    bcp_mem_set("year", 2020);
    bcp_mem_set("month", 2);
    bcp_mem_set("day", 26);
    std::string name =  bcp_mem_get_string("name");
    ASSERT_STR_EQUAL("test", name.c_str());

    std::string json = bcp_mem_to_json();
    ASSERT_FALSE(json.empty());

    FILE* file = bcp_file_open("./1.json", "w+");
    bcp_file_write(file, json.data(), json.length());
    bcp_file_flush(file);
    bcp_file_close(file);

    ASSERT_TRUE(bcp_file_type("./1.json") == FILE_TYPE_FILE);
    bcp_mem_remove_all();

    ASSERT_FALSE(bcp_mem_is_key_exist("country"));
    bcp_mem_from_file("./1.json");
    ASSERT_STR_EQUAL("test", name.c_str());
    bcp_file_remove("./1.json");
}

