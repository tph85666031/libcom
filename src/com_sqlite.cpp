#include <thread>
#include <atomic>
#include "sqlite3.h"
#include "com_sqlite.h"
#include "com_base.h"
#include "com_file.h"
#include "com_log.h"

static std::atomic<int> sqlite_retry_count = {10};
static std::atomic<int> sqlite_retry_interval_ms = {100};

std::string com_sqlite_escape(const char* str)
{
    if(str == NULL)
    {
        return std::string();
    }
    std::string val = str;
    com_string_replace(val, "'", "''");
    return val;
}

std::string com_sqlite_escape(const std::string& str)
{
    std::string val = str;
    com_string_replace(val, "'", "''");
    return val;
}

void com_sqlite_set_retry_count(int count)
{
    sqlite_retry_count = count;
}

void com_sqlite_set_retry_interval(int interval_ms)
{
    sqlite_retry_interval_ms = interval_ms;
}

/********************************************************
    com_sqlite_run_sql
    说明：基于sqlite3的数据库语句运行函数
    入参：fd:slite3操作句柄，sql：待执行的sql语句

    返回值：错误码,参考COM_SQL_ERR_XXX
 ********************************************************/
int com_sqlite_run_sql(void* fd, const char* sql)
{
    if(fd == NULL || sql == NULL || sql[0] == '\0')
    {
        //LOG_W("arg incorrect");
        return -1;
    }
    int retry_count = sqlite_retry_count;
    int ret = 0;
    while(retry_count--)
    {
        ret = sqlite3_exec((sqlite3*)fd, sql, NULL, NULL, NULL);
        if(ret != SQLITE_BUSY)
        {
            break;
        }
        com_sleep_ms(sqlite_retry_interval_ms);
    }
    return (-1 * ret);
}

void* com_sqlite_open(const char* file, bool read_only)
{
    if(file == NULL || file[0] == '\0')
    {
        return NULL;
    }
    sqlite3* fd = NULL;
    int flags = SQLITE_OPEN_FULLMUTEX;
    if(read_only)
    {
        flags |= SQLITE_OPEN_READONLY;
    }
    else
    {
        flags |= SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
    }
    int ret = sqlite3_open_v2(file, &fd, flags, NULL);
    if(ret != SQLITE_OK)
    {
        LOG_E("failed:%s", sqlite3_errmsg(fd));
        com_sqlite_close(fd);
        return NULL;
    }
    sqlite3_busy_timeout(fd, 1000);
    return fd;
}

/********************************************************
    com_sqlite_close
    说明：关闭sqlite3的数据库句柄
    入参：fd:slite3操作句柄

    返回值：无
 ********************************************************/
void com_sqlite_close(void* fd)
{
    if(fd == NULL)
    {
        return;
    }
    int ret = sqlite3_close_v2((sqlite3*)fd);
    if(ret != SQLITE_OK)
    {
        LOG_E("failed, ret=%d, err=%s", ret, sqlite3_errmsg((sqlite3*)fd));
    }
}

const char* com_sqlite_file_name(void* fd)
{
    return sqlite3_db_filename((sqlite3*)fd, NULL);
}

/********************************************************
    com_sqlite_query_free
    说明：释放数据库查询结果集
    入参：由com_sqlite_query_F生成的查询结果指针

    返回值：无
 ********************************************************/
void com_sqlite_query_free(char** result)
{
    if(result)
    {
        sqlite3_free_table(result);
    }
}

/********************************************************
    com_sqlite_query_F
    说明：数据库查询并获取所有结果
    入参：fd:sqlite3句柄，sql：查询语句，row_count：存储查询结果的行数
         column_count：存储查询结果的列数

    返回值：查询结果集，格式为result[row_count][column_count]
 ********************************************************/
char** com_sqlite_query_F(void* fd, const char* sql, int* row_count, int* column_count)
{
    if(fd == NULL || sql == NULL || row_count == NULL || column_count == NULL)
    {
        return NULL;
    }
    *row_count = 0;
    *column_count = 0;
    char** result = NULL;
    int ret = sqlite3_get_table((sqlite3*)fd, sql, &result,
                                row_count, column_count, NULL);
    if(ret != SQLITE_OK)
    {
        com_sqlite_query_free(result);
        return NULL;
    }

    return result;
}

int com_sqlite_query_count(void* fd, const char* sql)
{
    if(fd == NULL || sql == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    int count = 0;
    int ret = sqlite3_exec((sqlite3*)fd, sql, [](void* arg, int argc, char** argv, char** col_name)->int
    {
        int* row_count = (int*)arg;
        *row_count = *row_count + 1;
        return 0;
    }, &count, NULL);
    if(ret != SQLITE_OK)
    {
        return (-1 * ret);
    }
    return count;
}

/********************************************************
    com_sqlite_insert
    说明：数据库插入操作
    入参：fd:sqlite3句柄，sql：查询语句

    返回值：成功插入的个数
 ********************************************************/
int com_sqlite_insert(void* fd, const char* sql)
{
    if(fd == NULL || sql == NULL)
    {
        return 0;
    }
    int ret = com_sqlite_run_sql(fd, sql);
    if(ret != 0)
    {
        return ret;
    }
    return sqlite3_changes((sqlite3*)fd);
}

/********************************************************
    com_sqlite_delete
    说明：数据库删除操作
    入参：fd:sqlite3句柄，sql：查询语句

    返回值：成功删除的个数
 ********************************************************/
int com_sqlite_delete(void* fd, const char* sql)
{
    if(fd == NULL || sql == NULL)
    {
        return 0;
    }
    int ret = com_sqlite_run_sql(fd, sql);
    if(ret != 0)
    {
        return ret;
    }
    return sqlite3_changes((sqlite3*)fd);
}

/********************************************************
    com_sqlite_update
    说明：数据库更新操作
    入参：fd:sqlite3句柄，sql：查询语句

    返回值：成功更新的个数
 ********************************************************/
int com_sqlite_update(void* fd, const char* sql)
{
    if(fd == NULL || sql == NULL)
    {
        return 0;
    }
    int ret = com_sqlite_run_sql(fd, sql);
    if(ret != 0)
    {
        return ret;
    }
    return sqlite3_changes((sqlite3*)fd);
}

/********************************************************
    com_sqlite_is_table_exist
    说明：查询数据表是否存在
    入参：fd:sqlite3句柄，table_name：表名

    返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_is_table_exist(void* fd, const char* table_name)
{
    if(fd == NULL || table_name == NULL)
    {
        return false;
    }
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT * FROM sqlite_master WHERE type='table' AND name='%s';",
             com_sqlite_escape(table_name).c_str());
    int count = com_sqlite_query_count(fd, sql);
    return count > 0  ? true : false;
}

/********************************************************
    com_sqlite_table_row_count
    说明：查询数据表行数
    入参：fd:sqlite3句柄，table_name：表名

    返回值：数据表行数
 ********************************************************/
int com_sqlite_table_row_count(void* fd, const char* table_name)
{
    if(fd == NULL || table_name == NULL)
    {
        return 0;
    }
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM \"%s\";", com_sqlite_escape(table_name).c_str());
    int count = 0;
    int ret = sqlite3_exec((sqlite3*)fd, sql, [](void* arg, int argc, char** argv, char** col_name)->int
    {
        int* row_count = (int*)arg;
        if(argc > 0 && argv[0] != NULL)
        {
            *row_count = strtol(argv[0], NULL, 10);
        }
        return 0;
    }, &count, NULL);
    if(ret != SQLITE_OK)
    {
        return (-1 * ret);
    }
    return count;
}

/********************************************************
    com_sqlite_table_clean
    说明：清空数据表并保留数据表
    入参：fd:sqlite3句柄，table_name：表名

    返回值：错误码,参考COM_SQL_ERR_XXX
 ********************************************************/
bool com_sqlite_table_clean(void* fd, const char* table_name)
{
    if(fd == NULL || table_name == NULL)
    {
        return false;
    }
    char sql[256];
    snprintf(sql, sizeof(sql),
             "DELETE FROM \"%s\";UPDATE sqlite_sequence SET seq = 0 WHERE name = '%s';",
             table_name, com_sqlite_escape(table_name).c_str());
    return (com_sqlite_run_sql(fd, sql) == 0);
}

/********************************************************
    com_sqlite_table_remove
    说明：删除数据库
    入参：fd:sqlite3句柄，table_name：表名

    返回值：错误码,参考COM_SQL_ERR_XXX
 ********************************************************/
bool com_sqlite_table_remove(void* fd, const char* table_name)
{
    if(fd == NULL || table_name == NULL)
    {
        return false;
    }
    if(com_sqlite_table_clean(fd, table_name) == false)
    {
        return false;
    }
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE \"%s\";", com_sqlite_escape(table_name).c_str());
    return (com_sqlite_run_sql(fd, sql) == 0);
}

/********************************************************
    com_sqlite_begin_transaction
    说明：开启事务
    入参：fd:sqlite3句柄

    返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_begin_transaction(void* fd)
{
    return (com_sqlite_run_sql(fd, "BEGIN TRANSACTION;") == 0);
}

/********************************************************
    com_sqlite_commit_transaction
    说明：提交事务
    入参：fd:sqlite3句柄

    返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_commit_transaction(void* fd)
{
    return (com_sqlite_run_sql(fd, "COMMIT TRANSACTION;") == 0);
}

/********************************************************
    com_sqlite_rollback_transaction
    说明：回滚事务
    入参：fd:sqlite3句柄

    返回值：布尔值，true代表成功
 ********************************************************/
bool com_sqlite_rollback_transaction(void* fd)
{
    return (com_sqlite_run_sql(fd, "ROLLBACK TRANSACTION;") == 0);
}

void* com_sqlite_prepare(void* fd, const char* sql)
{
    if(fd == NULL || sql == NULL)
    {
        return NULL;
    }
    sqlite3_stmt* stmt = NULL;
    int ret = sqlite3_prepare_v2((sqlite3*)fd, sql, -1, &stmt, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }
    return stmt;
}

int com_sqlite_bind(void* stmt, int pos, bool val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int((sqlite3_stmt*)stmt, pos, val ? 1 : 0);
}

int com_sqlite_bind(void* stmt, int pos, int8 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int((sqlite3_stmt*)stmt, pos, (int)val);
}

int com_sqlite_bind(void* stmt, int pos, uint8 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int((sqlite3_stmt*)stmt, pos, (int)val);
}

int com_sqlite_bind(void* stmt, int pos, int16 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int((sqlite3_stmt*)stmt, pos, (int)val);
}

int com_sqlite_bind(void* stmt, int pos, uint16 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int((sqlite3_stmt*)stmt, pos, (int)val);
}

int com_sqlite_bind(void* stmt, int pos, int32 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int((sqlite3_stmt*)stmt, pos, (int)val);
}

int com_sqlite_bind(void* stmt, int pos, uint32 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int((sqlite3_stmt*)stmt, pos, (int)val);
}

int com_sqlite_bind(void* stmt, int pos, int64 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int64((sqlite3_stmt*)stmt, pos, val);
}

int com_sqlite_bind(void* stmt, int pos, uint64 val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_int64((sqlite3_stmt*)stmt, pos, (int64)val);
}

int com_sqlite_bind(void* stmt, int pos, double val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_double((sqlite3_stmt*)stmt, pos, val);
}

int com_sqlite_bind(void* stmt, int pos, float val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_double((sqlite3_stmt*)stmt, pos, (double)val);
}

int com_sqlite_bind(void* stmt, int pos, const char* val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_text((sqlite3_stmt*)stmt, pos, val, -1, SQLITE_TRANSIENT);
}

int com_sqlite_bind(void* stmt, int pos, const std::string& val)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_text((sqlite3_stmt*)stmt, pos, val.c_str(), -1, SQLITE_TRANSIENT);
}

int com_sqlite_bind(void* stmt, int pos, const uint8* data, int data_size)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_blob((sqlite3_stmt*)stmt, pos, data, data_size, SQLITE_STATIC);
}

int com_sqlite_bind(void* stmt, int pos, ComBytes& bytes)
{
    if(stmt == NULL || pos < 0)
    {
        return -1;
    }
    return sqlite3_bind_blob((sqlite3_stmt*)stmt, pos, bytes.getData(), bytes.getDataSize(), SQLITE_STATIC);
}

int com_sqlite_step(void* stmt)
{
    if(stmt == NULL)
    {
        return -1;
    }
    int retry_count = sqlite_retry_count;
    int ret = 0;
    while(retry_count--)
    {
        ret = sqlite3_step((sqlite3_stmt*)stmt);
        if(ret != SQLITE_BUSY)
        {
            break;
        }
        com_sleep_ms(sqlite_retry_interval_ms);
    }
    return (-1 * ret);
}

int com_sqlite_reset(void* stmt)
{
    if(stmt == NULL)
    {
        return -1;
    }
    return sqlite3_reset((sqlite3_stmt*)stmt);
}

int com_sqlite_finalize(void* stmt)
{
    if(stmt == NULL)
    {
        return -1;
    }
    return sqlite3_finalize((sqlite3_stmt*)stmt);
}

std::string com_sqlite_column_string(void* stmt, int pos)
{
    const unsigned char* val = sqlite3_column_text((sqlite3_stmt*)stmt, pos);
    if(val == NULL)
    {
        return std::string();
    }
    return (char*)val;
}

bool com_sqlite_column_bool(void* stmt, int pos)
{
    return (sqlite3_column_int((sqlite3_stmt*)stmt, pos) == 1);
}

int8 com_sqlite_column_int8(void* stmt, int pos)
{
    return (int8)sqlite3_column_int((sqlite3_stmt*)stmt, pos);
}

uint8 com_sqlite_column_uint8(void* stmt, int pos)
{
    return (uint8)sqlite3_column_int((sqlite3_stmt*)stmt, pos);
}

int16 com_sqlite_column_int16(void* stmt, int pos)
{
    return (int16)sqlite3_column_int((sqlite3_stmt*)stmt, pos);
}

uint16 com_sqlite_column_uint16(void* stmt, int pos)
{
    return (uint16)sqlite3_column_int((sqlite3_stmt*)stmt, pos);
}

int32 com_sqlite_column_int32(void* stmt, int pos)
{
    return sqlite3_column_int((sqlite3_stmt*)stmt, pos);
}

uint32 com_sqlite_column_uint32(void* stmt, int pos)
{
    return (uint32)sqlite3_column_int((sqlite3_stmt*)stmt, pos);
}

int64 com_sqlite_column_int64(void* stmt, int pos)
{
    return sqlite3_column_int64((sqlite3_stmt*)stmt, pos);
}

uint64 com_sqlite_column_uint64(void* stmt, int pos)
{
    return (uint64)sqlite3_column_int64((sqlite3_stmt*)stmt, pos);
}

float com_sqlite_column_float(void* stmt, int pos)
{
    return (float)sqlite3_column_double((sqlite3_stmt*)stmt, pos);
}

double com_sqlite_column_double(void* stmt, int pos)
{
    return sqlite3_column_double((sqlite3_stmt*)stmt, pos);
}

ComBytes com_sqlite_column_bytes(void* stmt, int pos)
{
    int size = sqlite3_column_bytes((sqlite3_stmt*)stmt, pos);
    if(size <= 0)
    {
        return ComBytes();
    }
    const void* val = sqlite3_column_blob((sqlite3_stmt*)stmt, pos);
    if(val == NULL)
    {
        return ComBytes();
    }

    return ComBytes((const uint8*)val, size);
}

DBQuery::DBQuery(const void* sqlite_fd, const char* sql)
{
    if(sqlite_fd != NULL && sql != NULL)
    {
        sqlite3_exec((sqlite3*)sqlite_fd, sql, QueryCallback, this, NULL);
    }
}

DBQuery::~DBQuery()
{
}

int DBQuery::getCount()
{
    return (int)results.size();
}

std::string DBQuery::getItem(int row, const char* col_name, const char* default_val)
{
    if(row < 0 || row >= (int)results.size() || col_name == NULL)
    {
        return default_val == NULL ? "" : default_val;
    }

    std::map<std::string, std::string>& item = results[row];
    if(item.count(col_name) <= 0)
    {
        return default_val == NULL ? "" : default_val;
    }
    return item[col_name];
}

double DBQuery::getItemAsDouble(int row, const char* col_name, double default_val)
{
    if(row < 0 || row >= (int)results.size() || col_name == NULL)
    {
        return default_val;
    }

    std::map<std::string, std::string>& item = results[row];
    if(item.count(col_name) <= 0)
    {
        return default_val;
    }
    return strtold(item[col_name].c_str(), NULL);
}

int64 DBQuery::getItemAsNumber(int row, const char* col_name, int64 default_val)
{
    if(row < 0 || row >= (int)results.size() || col_name == NULL)
    {
        return default_val;
    }

    std::map<std::string, std::string>& item = results[row];
    if(item.count(col_name) <= 0)
    {
        return default_val;
    }
    return strtoll(item[col_name].c_str(), NULL, 10);
}

bool DBQuery::getItemAsBool(int row, const char* col_name, bool default_val)
{
    std::string val = getItem(row, col_name);
    if(val.empty())
    {
        return default_val;
    }
    com_string_to_lower(val);
    if(val == "true" || val == "yes")
    {
        return true;
    }
    else if(val == "false" || val == "no")
    {
        return false;
    }
    else
    {
        return (val[0] == 1);
    }
}

int DBQuery::QueryCallback(void* arg, int argc, char** argv, char** col_name)
{
    if(arg == NULL)
    {
        return 0;
    }
    DBQuery* ctx = (DBQuery*)arg;

    std::map<std::string, std::string> row;
    for(int i = 0; i < argc; i++)
    {
        if(col_name[i] == NULL || argv[i] == NULL)
        {
            continue;
        }
        row[col_name[i]] = argv[i];
    }
    ctx->results.push_back(row);
    return 0;
}

DBConnnection::DBConnnection(const char* file_db, bool read_only)
{
    fd = com_sqlite_open(file_db, read_only);
}

DBConnnection::DBConnnection(const std::string& file_db, bool read_only)
{
    fd = com_sqlite_open(file_db.c_str(), read_only);
}

DBConnnection::~DBConnnection()
{
    com_sqlite_close(fd);
    fd = NULL;
}

bool DBConnnection::isConnected()
{
    return fd != NULL;
}

void* DBConnnection::getHandle()
{
    return fd;
}

DBStatement::DBStatement()
{
}

DBStatement::~DBStatement()
{
}

