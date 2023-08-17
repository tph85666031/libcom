#ifndef __COM_SQLITE_H__
#define __COM_SQLITE_H__

#include "com_base.h"

class COM_EXPORT DBConnnection
{
public:
    DBConnnection(const char* file_db, bool read_only = false);
    DBConnnection(const std::string& file_db, bool read_only = false);
    virtual ~DBConnnection();

    bool isConnected();
    void* getHandle();
private:
    void* fd;
};

class COM_EXPORT DBQuery
{
public:
    DBQuery(const void* sqlite_fd, const char* sql);
    virtual ~DBQuery();
    int getCount();
    std::string getItem(int row, const char* col_name, const char* default_val = NULL);
    double getItemAsDouble(int row, const char* col_name, double default_val = 0);
    int64 getItemAsNumber(int row, const char* col_name, int64 default_val = 0);
    bool getItemAsBool(int row, const char* col_name, bool default_val = false);
private:
    static int QueryCallback(void* arg, int argc, char** argv, char** col_name);
private:
    std::vector<std::map<std::string, std::string>> results;
};

class COM_EXPORT DBStatement
{
public:
    DBStatement();
    virtual ~DBStatement();

private:
    void* stmt;
};

COM_EXPORT std::string com_sqlite_escape(const char* str);
COM_EXPORT std::string com_sqlite_escape(const std::string& str);
COM_EXPORT void com_sqlite_set_retry_count(int count);
COM_EXPORT void com_sqlite_set_retry_interval(int interval_ms);
COM_EXPORT void* com_sqlite_open(const char* file, bool read_only = false);
COM_EXPORT void com_sqlite_close(void* fd);

COM_EXPORT const char* com_sqlite_file_name(void* fd);

COM_EXPORT int com_sqlite_run_sql(void* fd, const char* sql);

COM_EXPORT int com_sqlite_insert(void* fd, const char* sql);
COM_EXPORT int com_sqlite_delete(void* fd, const char* sql);
COM_EXPORT int com_sqlite_update(void* fd, const char* sql);

//推荐使用DBQueryResult来获取查询结果
COM_EXPORT char** com_sqlite_query_F(void* fd, const char* sql, int* row_count, int* column_count);
COM_EXPORT void com_sqlite_query_free(char** buf);
COM_EXPORT int com_sqlite_query_count(void* fd, const char* sql);

COM_EXPORT bool com_sqlite_is_table_exist(void* fd, const char* table_name);

COM_EXPORT int com_sqlite_table_row_count(void* fd, const char* table_name);
COM_EXPORT bool com_sqlite_table_clean(void* fd, const char* table_name);
COM_EXPORT bool com_sqlite_table_remove(void* fd, const char* table_name);

COM_EXPORT bool com_sqlite_begin_transaction(void* fd);
COM_EXPORT bool com_sqlite_commit_transaction(void* fd);
COM_EXPORT bool com_sqlite_rollback_transaction(void* fd);

COM_EXPORT void* com_sqlite_prepare(void* fd, const char* sql);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, bool val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, int8 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, uint8 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, int16 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, uint16 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, int32 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, uint32 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, int64 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, uint64 val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, double val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, float val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, const char* val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, const std::string& val);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, const uint8* data, int data_size);
COM_EXPORT int com_sqlite_bind(void* stmt, int pos, CPPBytes& bytes);
COM_EXPORT int com_sqlite_step(void* stmt);
COM_EXPORT int com_sqlite_reset(void* stmt);
COM_EXPORT int com_sqlite_finalize(void* stmt);

COM_EXPORT std::string com_sqlite_column_string(void* stmt, int pos);
COM_EXPORT bool com_sqlite_column_bool(void* stmt, int pos);
COM_EXPORT int8 com_sqlite_column_int8(void* stmt, int pos);
COM_EXPORT uint8 com_sqlite_column_uint8(void* stmt, int pos);
COM_EXPORT int16 com_sqlite_column_int16(void* stmt, int pos);
COM_EXPORT uint16 com_sqlite_column_uint16(void* stmt, int pos);
COM_EXPORT int32 com_sqlite_column_int32(void* stmt, int pos);
COM_EXPORT uint32 com_sqlite_column_uint32(void* stmt, int pos);
COM_EXPORT int64 com_sqlite_column_int64(void* stmt, int pos);
COM_EXPORT uint64 com_sqlite_column_uint64(void* stmt, int pos);
COM_EXPORT float com_sqlite_column_float(void* stmt, int pos);
COM_EXPORT double com_sqlite_column_double(void* stmt, int pos);
COM_EXPORT CPPBytes com_sqlite_column_bytes(void* stmt, int pos);

#endif /* __COM_SQLITE_H__ */
