#include "bcp.h"

static const char* mem_db_opt_type[] = {"remove all", "remove", "update", "add", "add all"};

static void mem_cb_test(std::string key, int flag)
{
    LOG_W("%s detected,flag=%s", key.c_str(), mem_db_opt_type[flag + 2]);
}

void com_mem_unit_test_suit(void** state)
{
    com_mem_add_notify("age", mem_cb_test);
    com_mem_add_notify("name", mem_cb_test);
    com_mem_add_notify("money", mem_cb_test);
    com_mem_set("name", "test");
    com_mem_set("name", "test");
    com_mem_remove("name");
    com_mem_set("name", "test");
    com_mem_set("age", 24);
    com_mem_set("money", 15792.25);
    com_mem_set("country", "china");
    com_mem_set("year", 2020);
    com_mem_set("month", 2);
    com_mem_set("day", 26);
    std::string name =  com_mem_get_string("name");
    ASSERT_STR_EQUAL("test", name.c_str());

    std::string json = com_mem_to_json();
    ASSERT_FALSE(json.empty());

    FILE* file = com_file_open("./1.json", "w+");
    com_file_write(file, json.data(), json.length());
    com_file_flush(file);
    com_file_close(file);

    ASSERT_TRUE(com_file_type("./1.json") == FILE_TYPE_FILE);
    com_mem_remove_all();

    ASSERT_FALSE(com_mem_is_key_exist("country"));
    com_mem_from_file("./1.json");
    ASSERT_STR_EQUAL("test", name.c_str());
    com_file_remove("./1.json");
}

