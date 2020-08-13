#include <thread>
#include <atomic>
#include "sqlite3.h"
#include "com_sqlite.h"
#include "com_com.h"
#include "com_file.h"
#include "com_log.h"

/********************************************************
 * com_sqlite_run_sql
 * 说明：基于sqlite3的数据库语句运行函数
 * 入参：fd:slite3操作句柄，sql：待执行的sql语句
 *
 * 返回值：错误码,参考BCP_SQL_ERR_XXX
 ********************************************************/
int com_sqlite_run_sql(void* fd, const char* sql)
{
    if (fd == NULL || sql == NULL)
    {
        LOG_W("arg incorrect");
        return BCP_SQL_ERR_FAILED;
    }
    int retry_count = 0;
    while (retry_count++ < 5)
    {
        char* err_msg = NULL;
        int ret = sqlite3_exec((sqlite3*)fd, sql, NULL, NULL, &err_msg);
        if (ret == SQLITE_BUSY)
        {
            if (err_msg != NULL)
            {
                sqlite3_free(err_msg);
            }
            LOG_W("db %s is busy, retry %d", sqlite3_db_filename((sqlite3*)fd, NULL), retry_count);
            com_sleep_ms(retry_count * 100);
            continue;
        }
        else if (ret == SQLITE_OK)
        {
            if (err_msg != NULL)
            {
                sqlite3_free(err_msg);
            }
            return BCP_SQL_ERR_OK;
        }
        else
        {
            LOG_E("failed, ret=%d, err=%s, sql=%s, file=%s, file_exist=%s",
                  ret, err_msg, sql,
                  sqlite3_db_filename((sqlite3*)fd, NULL),
                  com_file_type(sqlite3_db_filename((sqlite3*)fd, NULL)) == FILE_TYPE_FILE ? "true" : "false");
            if (err_msg != NULL)
            {
                sqlite3_free(err_msg);
            }
            if (ret == SQLITE_READONLY || ret == SQLITE_IOERR)
            {
                LOG_E("file may not exist");
            }

            return BCP_SQL_ERR_FAILED;
        }
    }
    return BCP_SQL_ERR_BUSY;
}

void* com_sqlite_open(const char* file)
{
    if (file == NULL || strlen(file) == 0)
    {
        return NULL;
    }
    sqlite3* fd = NULL;
    if (com_file_type(file) == FILE_TYPE_NOT_EXIST)
    {
        LOG_I("db file not exist, system will auto create this missing db file: %s", file);
    }
    int ret = sqlite3_open_v2(file, &fd,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                              NULL);
    if (ret != SQLITE_OK)
    {
        LOG_E("failed:%s", sqlite3_errmsg(fd));
        com_sqlite_close(fd);
        return NULL;
    }
    sqlite3_busy_timeout(fd, 1000);
    return fd;
}

/********************************************************
 * com_sqlite_close
 * 说明：关闭sqlite3的数据库句柄
 * 入参：fd:slite3操作句柄
 *
 * 返回值：无
 ********************************************************/
void com_sqlite_close(void* fd)
{
    if (fd)
    {
        int ret = sqlite3_close_v2((sqlite3*)fd);
        if (ret != SQLITE_OK)
        {
            LOG_E("failed, ret=%d, err=%s", ret, sqlite3_errmsg((sqlite3*)fd));
        }
    }
}

const char* com_sqlite_file_name(void* fd)
{
    return sqlite3_db_filename((sqlite3*)fd, NULL);
}

/********************************************************
 * com_sqlite_query_free
 * 说明：释放数据库查询结果集
 * 入参：由com_sqlite_query_F生成的查询结果指针
 *
 * 返回值：无
 ********************************************************/
void com_sqlite_query_free(char** result)
{
    if (result)
    {
        sqlite3_free_table(result);
    }
}

/********************************************************
 * com_sqlite_query_F
 * 说明：数据库查询并获取所有结果
 * 入参：fd:sqlite3句柄，sql：查询语句，row_count：存储查询结果的行数
 *       column_count：存储查询结果的列数
 *
 * 返回值：查询结果集，格式为result[row_count][column_count]
 ********************************************************/
char** com_sqlite_query_F(void* fd, const char* sql,
                          int* row_count, int* column_count)
{
    if (fd == NULL || sql == NULL || row_count == NULL || column_count == NULL)
    {
        LOG_E("failed, arg incorrect");
        return NULL;
    }
    *row_count = 0;
    *column_count = 0;
    char** result = NULL;
    char* err_msg = NULL;
    int ret = sqlite3_get_table((sqlite3*)fd, sql, &result,
                                row_count, column_count,
                                &err_msg);
    if (ret != SQLITE_OK)
    {
        LOG_E("failed,sql=[%s],err=%s", sql, err_msg);
        sqlite3_free(err_msg);
        com_sqlite_query_free(result);
        return NULL;
    }

    return result;
}

/********************************************************
 * com_sqlite_query_count
 * 说明：数据库查询并获取查询匹配的数据个数
 * 入参：fd:sqlite3句柄，sql：查询语句
 *
 * 返回值：查询匹配的数据个数
 ********************************************************/
int com_sqlite_query_count(void* fd, const char* sql)
{
    if (fd == NULL || sql == NULL)
    {
        return 0;
    }
    char** result = NULL;
    char* err_msg = NULL;
    int row_count = 0;
    int column_count = 0;
    int ret = sqlite3_get_table((sqlite3*)fd, sql, &result,
                                &row_count, &column_count,
                                &err_msg);
    com_sqlite_query_free(result);
    if (ret != SQLITE_OK)
    {
        LOG_E("failed, ret=%d, err=%s", ret, err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return row_count;
}

/********************************************************
 * com_sqlite_insert
 * 说明：数据库插入操作
 * 入参：fd:sqlite3句柄，sql：查询语句
 *
 * 返回值：成功插入的个数
 ********************************************************/
int com_sqlite_insert(void* fd, const char* sql)
{
    if (fd == NULL || sql == NULL)
    {
        return 0;
    }
    int ret = com_sqlite_run_sql(fd, sql);
    if (ret != BCP_SQL_ERR_OK)
    {
        return ret;
    }
    ret = sqlite3_changes((sqlite3*)fd);
    if (ret <= 0)
    {
        ret = 0;
    }
    return ret;
}

/********************************************************
 * com_sqlite_delete
 * 说明：数据库删除操作
 * 入参：fd:sqlite3句柄，sql：查询语句
 *
 * 返回值：成功删除的个数
 ********************************************************/
int com_sqlite_delete(void* fd, const char* sql)
{
    if (fd == NULL || sql == NULL)
    {
        return 0;
    }
    int ret = com_sqlite_run_sql(fd, sql);
    if (ret != BCP_SQL_ERR_OK)
    {
        return ret;
    }
    ret = sqlite3_changes((sqlite3*)fd);
    if (ret <= 0)
    {
        ret = 0;
    }
    return ret;
}

/********************************************************
 * com_sqlite_update
 * 说明：数据库更新操作
 * 入参：fd:sqlite3句柄，sql：查询语句
 *
 * 返回值：成功更新的个数
 ********************************************************/
int com_sqlite_update(void* fd, const char* sql)
{
    if (fd == NULL || sql == NULL)
    {
        return 0;
    }
    int ret = com_sqlite_run_sql(fd, sql);
    if (ret != BCP_SQL_ERR_OK)
    {
        return ret;
    }
    ret = sqlite3_changes((sqlite3*)fd);
    if (ret <= 0)
    {
        ret = 0;
    }
    return ret;
}

/********************************************************
 * com_sqlite_is_table_exist
 * 说明：查询数据表是否存在
 * 入参：fd:sqlite3句柄，table_name：表名
 *
 * 返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_is_table_exist(void* fd, const char* table_name)
{
    if (fd == NULL || table_name == NULL)
    {
        return false;
    }
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT * FROM sqlite_master WHERE type='table' AND name='%s';",
             table_name);
    int count = com_sqlite_query_count(fd, sql);
    return count > 0  ? true : false;
}

/********************************************************
 * com_sqlite_table_row_count
 * 说明：查询数据表行数
 * 入参：fd:sqlite3句柄，table_name：表名
 *
 * 返回值：数据表行数
 ********************************************************/
int com_sqlite_table_row_count(void* fd, const char* table_name)
{
    if (fd == NULL || table_name == NULL)
    {
        return 0;
    }
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM \"%s\";", table_name);
    int row_count;
    int col_count;
    char** result = com_sqlite_query_F(fd, sql, &row_count, &col_count);
    if (result == NULL)
    {
        LOG_E("failed");
        return 0;
    }
    int count = strtoul(result[1 * col_count], NULL, 10);

    com_sqlite_query_free(result);
    return count;
}

/********************************************************
 * com_sqlite_table_clean
 * 说明：清空数据表并保留数据表
 * 入参：fd:sqlite3句柄，table_name：表名
 *
 * 返回值：错误码,参考BCP_SQL_ERR_XXX
 ********************************************************/
bool com_sqlite_table_clean(void* fd, const char* table_name)
{
    if (fd == NULL || table_name == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    char sql[256];
    snprintf(sql, sizeof(sql),
             "DELETE FROM \"%s\";UPDATE sqlite_sequence SET seq = 0 WHERE name = '%s';",
             table_name, table_name);
    return (com_sqlite_run_sql(fd, sql) == BCP_SQL_ERR_OK);
}

/********************************************************
 * com_sqlite_table_remove
 * 说明：删除数据库
 * 入参：fd:sqlite3句柄，table_name：表名
 *
 * 返回值：错误码,参考BCP_SQL_ERR_XXX
 ********************************************************/
bool com_sqlite_table_remove(void* fd, const char* table_name)
{
    if (fd == NULL || table_name == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    if (com_sqlite_table_clean(fd, table_name) == false)
    {
        LOG_E("failed");
        return false;
    }
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE \"%s\";", table_name);
    return (com_sqlite_run_sql(fd, sql) == BCP_SQL_ERR_OK);
}

/********************************************************
 * com_sqlite_begin_transaction
 * 说明：开启事务
 * 入参：fd:sqlite3句柄
 *
 * 返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_begin_transaction(void* fd)
{
    return (com_sqlite_run_sql(fd, "BEGIN TRANSACTION;") == BCP_SQL_ERR_OK);
}

/********************************************************
 * com_sqlite_commit_transaction
 * 说明：提交事务
 * 入参：fd:sqlite3句柄
 *
 * 返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_commit_transaction(void* fd)
{
    return (com_sqlite_run_sql(fd, "COMMIT TRANSACTION;") == BCP_SQL_ERR_OK);
}

/********************************************************
 * com_sqlite_rollback_transaction
 * 说明：回滚事务
 * 入参：fd:sqlite3句柄
 *
 * 返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_rollback_transaction(void* fd)
{
    return (com_sqlite_run_sql(fd, "ROLLBACK TRANSACTION;") == BCP_SQL_ERR_OK);
}

DBQuery::DBQuery(const void* sqlite_fd, const char* sql)
{
    if (sqlite_fd != NULL && sql != NULL)
    {
        int row_count = 0;
        int column_count = 0;
        char** data = com_sqlite_query_F((void*)sqlite_fd, sql, &row_count, &column_count);
        if (data == NULL)
        {
            row_count = 0;
            column_count = 0;
            return;
        }
        row_count++;//包括列头
        list = std::vector<std::vector<std::string>>(row_count, std::vector<std::string>(column_count));
        for (int i = 0; i < row_count; i++)
        {
            for (int j = 0; j < column_count; j++)
            {
                if (data[i * column_count + j] != NULL)
                {
                    list[i][j] = data[i * column_count + j];
                }
            }
        }
        com_sqlite_query_free(data);
    }
}

DBQuery::~DBQuery()
{
}

int DBQuery::getRowCount()
{
    return list.size() - 1;//排除列头
}

int DBQuery::getColumnCount()
{
    if (list.empty())
    {
        return 0;
    }
    return list[0].size();
}

const char* DBQuery::getItem(int row, int column)
{
    if (row < 0 || column < 0 || list.empty())
    {
        return NULL;
    }

    row++;//排除列头
    if (row >= this->list.size() || column >= this->list[0].size())
    {
        return NULL;
    }

    return list[row][column].c_str();//排除列头
}

