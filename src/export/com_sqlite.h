#ifndef __COM_SQLITE_H__
#define __COM_SQLITE_H__

#include "com_base.h"

#define COM_SQL_ERR_OK      0
#define COM_SQL_ERR_FAILED  -1
#define COM_SQL_ERR_BUSY    -2

class DBQuery
{
public:
    DBQuery(const void* sqlite_fd, const char* sql);
    virtual ~DBQuery();
    int getRowCount();
    int getColumnCount();
    /* row && column index begin will 0, example(row=0-2,column=0-3):
    [0,0] [0,1] [0,2] [0,3]
    [1,0] [1,1] [1,2] [1,3]
    [2,0] [2,1] [2,2] [2,3]
    */
    const char* getItem(int row, int column);
private:
    std::vector<std::vector<std::string>> list;
};

void* com_sqlite_open(const char* file);
void com_sqlite_close(void* fd);

const char* com_sqlite_file_name(void* fd);

int com_sqlite_run_sql(void* fd, const char* sql);

int com_sqlite_insert(void* fd, const char* sql);
int com_sqlite_delete(void* fd, const char* sql);
int com_sqlite_update(void* fd, const char* sql);

//推荐使用DBQueryResult来获取查询结果
char** com_sqlite_query_F(void* fd, const char* sql, int* row_count, int* column_count);
void com_sqlite_query_free(char** buf);
int com_sqlite_query_count(void* fd, const char* sql);

bool com_sqlite_is_table_exist(void* fd, const char* table_name);

int com_sqlite_table_row_count(void* fd, const char* table_name);
bool com_sqlite_table_clean(void* fd, const char* table_name);
bool com_sqlite_table_remove(void* fd, const char* table_name);

bool com_sqlite_begin_transaction(void* fd);
bool com_sqlite_commit_transaction(void* fd);
bool com_sqlite_rollback_transaction(void* fd);

#endif /* __COM_SQLITE_H__ */
