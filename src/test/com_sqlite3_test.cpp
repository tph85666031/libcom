#include "com.h"

std::atomic<uint32> progress_cur;
std::atomic<uint32> progress_all;
void sqlite3_test_thread_progress()
{
    while (progress_cur < progress_all)
    {
        LOG_D("sqlite3 test progress: %.2f%%", progress_cur * 100.0f / progress_all);
        com_sleep_ms(100);
    }
    LOG_D("sqlite3 test progress: 100%%");
}

void sqlite3_test_thread(void* fd, std::string table_name, int count, int start)
{
    uint32 ID = start;
    uint32 PARAM = start;
    while (count-- > 0)
    {
        ID++;
        PARAM++;
        progress_cur++;
        std::string sql = com_string_format("INSERT INTO %s (ID,PARAM,TYPE,VALUE) VALUES(%u,%u,%u,'%s')",
                                            table_name.c_str(), ID, PARAM, 3, "test");
        com_sqlite_insert(fd, sql.c_str());
    }
}

void com_sqlite3_unit_test_suit(void** state)
{
    TIME_COST();
    void* fd = com_sqlite_open("./settings.db");
    ASSERT_TRUE(com_string_end_with(com_sqlite_file_name(fd), "settings.db"));

    ASSERT_INT_EQUAL(com_file_get_create_time(NULL), 0);
    ASSERT_INT_EQUAL(com_file_get_create_time("./not_exist_file"), 0);
    ASSERT_TRUE(com_time_to_string(com_file_get_create_time("./settings.db"), "%Y-%m-%d %H:%M:%S %z").length() > 0);
    ASSERT_TRUE(com_time_to_string(com_file_get_modify_time("./settings.db"), "%Y-%m-%d %H:%M:%S %z").length() > 0);
    ASSERT_TRUE(com_time_to_string(com_file_get_access_time("./settings.db"), "%Y-%m-%d %H:%M:%S %z").length() > 0);
    ASSERT_NOT_NULL(fd);

    static char sql_create_table[1024];
    snprintf(sql_create_table, sizeof(sql_create_table),
             "CREATE TABLE \"%s\" ( \
             \"%s\"  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
             \"%s\"  INTEGER UNIQUE NOT NULL, \
             \"%s\"  INTEGER NOT NULL, \
             \"%s\"  TEXT NOT NULL);",
             "setting",
             "ID",
             "PARAM",
             "TYPE",
             "VALUE");

    ASSERT_INT_EQUAL(com_sqlite_run_sql(fd, sql_create_table),COM_SQL_ERR_OK);

    snprintf(sql_create_table, sizeof(sql_create_table),
             "CREATE TABLE \"%s\" ( \
             \"%s\"  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
             \"%s\"  INTEGER UNIQUE NOT NULL, \
             \"%s\"  INTEGER NOT NULL, \
             \"%s\"  TEXT NOT NULL);",
             "conf",
             "ID",
             "PARAM",
             "TYPE",
             "VALUE");

    ASSERT_INT_EQUAL(com_sqlite_run_sql(fd, sql_create_table),COM_SQL_ERR_OK);
    const char* sql = "select * from setting";
    int row_count;
    int column_count;
    char** result = com_sqlite_query_F(fd, sql, &row_count, &column_count);
    //ASSERT_NOT_NULL(result);
    //ASSERT_INT_EQUAL(column_count, 4);
    com_sqlite_query_free(result);
    bool ret = com_sqlite_is_table_exist(fd, "xxx");
    ASSERT_FALSE(ret);

    ret = com_sqlite_is_table_exist(fd, "setting");
    //ASSERT_TRUE(ret);

    std::vector<std::thread*> threads;
    int item_count = 50;
    int thread_count = 5;
    progress_cur = 0;
    progress_all = item_count * thread_count * 2;
    std::thread t_progress = std::thread(sqlite3_test_thread_progress);
    LOG_D("start create %d threads for setting table, each will insert %d items", thread_count, item_count);
    for (int i = 0; i < thread_count; i++)
    {
        std::thread* t = new std::thread(sqlite3_test_thread, fd, "setting", item_count, i * item_count);
        threads.push_back(t);
    }
    LOG_D("start create %d threads for conf table, each will insert %d items", thread_count, item_count);
    for (int i = 0; i < thread_count; i++)
    {
        std::thread* t = new std::thread(sqlite3_test_thread, fd, "conf", item_count, i * item_count);
        threads.push_back(t);
    }

    for (size_t i = 0; i < threads.size(); i++)
    {
        if (threads[i]->joinable())
        {
            threads[i]->join();
        }
        delete threads[i];
    }

    if (t_progress.joinable())
    {
        t_progress.join();
    }
    com_sqlite_close(fd);
    com_file_remove("./settings.db");
}

