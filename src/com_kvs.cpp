#include "com_kvs.h"
#include "com_file.h"
#include "com_log.h"
#include "com_sqlite.h"

#define KVS_TABLE_NAME "kvs"
#define KVS_TABLE_ID   "id"
#define KVS_TABLE_UUID "uuid"
#define KVS_TABLE_DATA "data"

static bool kvs_create_table(void* fd)
{
    std::string sql = com_string_format("CREATE TABLE \"%s\" ( \
                                        \"%s\"  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
                                        \"%s\"  TEXT UNIQUE NOT NULL, \
                                        \"%s\"  TEXT NOT NULL);",
                                        KVS_TABLE_NAME,
                                        KVS_TABLE_ID,
                                        KVS_TABLE_UUID,
                                        KVS_TABLE_DATA);
    return (com_sqlite_run_sql(fd, sql.c_str()) == BCP_SQL_ERR_OK);
}

static void* kvs_open(const char* file)
{
    void* fd = com_sqlite_open(file);
    if (fd == NULL)
    {
        return NULL;
    }
    if (com_sqlite_is_table_exist(fd, KVS_TABLE_NAME) == false)
    {
        if (kvs_create_table(fd) == false)
        {
            com_sqlite_close(fd);
            return NULL;
        }
    }

    return fd;
}

static void kvs_close(void* fd)
{
    com_sqlite_close(fd);
}

void* com_kvs_batch_start(const char* file)
{
    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return NULL;
    }
    if (com_sqlite_begin_transaction(fd) == false)
    {
        kvs_close(fd);
        return NULL;
    }
    return fd;
}

bool com_kvs_batch_set(void* handle, const char* key, const uint8* data, int data_size)
{
    if (handle == NULL || key == NULL || data == NULL)
    {
        return false;
    }
    std::string hex_str = com_bytes_to_hexstring(data, data_size);
    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KVS_TABLE_NAME,
                                        KVS_TABLE_UUID,
                                        KVS_TABLE_DATA,
                                        key,
                                        hex_str.c_str());
    return (com_sqlite_insert(handle, sql.c_str()) == 1);
}

bool com_kvs_batch_set(void* handle, const char* key, const char* data)
{
    if (handle == NULL || key == NULL || data == NULL)
    {
        return false;
    }

    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KVS_TABLE_NAME,
                                        KVS_TABLE_UUID,
                                        KVS_TABLE_DATA,
                                        key,
                                        data);

    return (com_sqlite_insert(handle, sql.c_str())==1);
}

bool com_kvs_batch_remove(void* handle, const char* key)
{
    if (handle == NULL || key == NULL)
    {
        return false;
    }

    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE %s=\"%s\";",
                                        KVS_TABLE_NAME,
                                        KVS_TABLE_UUID, key);

    return (com_sqlite_delete(handle, sql.c_str())==1);
}

void com_kvs_batch_stop(void* handle, bool rollback)
{
    if (handle == NULL)
    {
        return;
    }
    if (rollback)
    {
        com_sqlite_rollback_transaction(handle);
    }
    else
    {
        com_sqlite_commit_transaction(handle);
    }
    kvs_close(handle);

    return;
}

bool com_kvs_set(const char* file, const char* key, const uint8* data, int data_size)
{
    if (file == NULL || key == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return false;
    }

    std::string hex_str = com_bytes_to_hexstring((uint8*)data, data_size);
    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KVS_TABLE_NAME,
                                        KVS_TABLE_UUID,
                                        KVS_TABLE_DATA,
                                        key,
                                        hex_str.c_str());
    int ret = com_sqlite_insert(fd, sql.c_str());
    kvs_close(fd);
    return (ret==1);
}

bool com_kvs_set(const char* file, const char* key, const char* value)
{
    if (file == NULL || key == NULL || value == NULL)
    {
        return false;
    }
    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return false;
    }

    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KVS_TABLE_NAME,
                                        KVS_TABLE_UUID,
                                        KVS_TABLE_DATA,
                                        key,
                                        value);

    int ret = com_sqlite_insert(fd, sql.c_str());
    kvs_close(fd);
    return (ret==1);
}

bool com_kvs_set(const char* file, const char* key, bool value)
{
    return com_kvs_set(file, key, value ? "1" : "0");
}

bool com_kvs_set(const char* file, const char* key, int8 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, uint8 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, int16 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, uint16 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, int32 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, uint32 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, int64 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, uint64 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%llu", value);
    return com_kvs_set(file, key, buf);
}

bool com_kvs_set(const char* file, const char* key, double value)
{
    std::string val = std::to_string(value);
    return com_kvs_set(file, key, val.c_str());
}

bool com_kvs_remove(const char* file, const char* key)
{
    if (file == NULL || key == NULL)
    {
        return false;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return false;
    }
    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE %s=\"%s\";",
                                        KVS_TABLE_NAME,
                                        KVS_TABLE_UUID, key);

    int ret = com_sqlite_delete(fd, sql.c_str());
    kvs_close(fd);
    return (ret==1);
}

void com_kvs_remove_all(const char* file)
{
    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return;
    }
    com_sqlite_table_clean(fd, KVS_TABLE_NAME);
    kvs_close(fd);
    return;
}

void com_kvs_remove_front(const char* file, int count)
{
    if (file == NULL || count <= 0)
    {
        return;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return;
    }

    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\" IN(SELECT \"%s\" FROM \"%s\" ORDER BY \"%s\" LIMIT %d)",
                                        KVS_TABLE_NAME, KVS_TABLE_ID,
                                        KVS_TABLE_ID, KVS_TABLE_NAME,
                                        KVS_TABLE_ID, count);
    com_sqlite_delete(fd, sql.c_str());
    kvs_close(fd);
    return;
}

void com_kvs_remove_tail(const char* file, int count)
{
    if (file == NULL || count <= 0)
    {
        return;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return;
    }

    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\" IN(SELECT \"%s\" FROM \"%s\" ORDER BY \"%s\" DESC LIMIT %d)",
                                        KVS_TABLE_NAME, KVS_TABLE_ID,
                                        KVS_TABLE_ID, KVS_TABLE_NAME,
                                        KVS_TABLE_ID, count);
    com_sqlite_delete(fd, sql.c_str());
    kvs_close(fd);
    return;
}

int com_kvs_count(const char* file)
{
    if (file == NULL)
    {
        return -1;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return -1;
    }

    int ret = com_sqlite_table_row_count(fd, KVS_TABLE_NAME);
    kvs_close(fd);
    return ret;
}

bool com_kvs_exist(const char* file, const char* key)
{
    if (file == NULL || key == NULL)
    {
        return false;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return false;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" WHERE %s=\"%s\"",
                                        KVS_TABLE_NAME, KVS_TABLE_UUID, key);
    DBQuery query(fd, sql.c_str());
    kvs_close(fd);
    if (query.getRowCount() <= 0)
    {
        return false;
    }
    const char* uuid = query.getItem(0, 1);
    const char* data = query.getItem(0, 2);

    if (uuid == NULL || data == NULL)
    {
        return false;
    }
    return true;
}

std::vector<KVResult> com_kvs_get_front(const char* file, int count)
{
    std::vector<KVResult> list;
    if (file == NULL || count <= 0)
    {
        return list;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return list;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" ORDER BY \"%s\" LIMIT %d",
                                        KVS_TABLE_NAME, KVS_TABLE_ID, count);
    DBQuery query(fd, sql.c_str());
    kvs_close(fd);
    for (int i = 0; i < query.getRowCount(); i++)
    {
        const char* uuid = query.getItem(i, 1);
        const char* data = query.getItem(i, 2);

        KVResult result(uuid, (uint8*)data, com_string_size(data));
        list.push_back(result);
    }
    return list;
}

std::vector<KVResult> com_kvs_get_tail(const char* file, int count)
{
    std::vector<KVResult> list;
    if (file == NULL || count <= 0)
    {
        return list;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return list;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" ORDER BY \"%s\" DESC LIMIT %d",
                                        KVS_TABLE_NAME, KVS_TABLE_ID, count);
    DBQuery query(fd, sql.c_str());
    kvs_close(fd);
    for (int i = 0; i < query.getRowCount(); i++)
    {
        const char* uuid = query.getItem(i, 1);
        const char* data = query.getItem(i, 2);

        KVResult result(uuid, (uint8*)data, com_string_size(data));
        list.push_back(result);
    }
    return list;
}

std::vector<std::string> com_kvs_get_all_keys(const char* file)
{
    std::vector<std::string> keys;
    if (file == NULL)
    {
        return keys;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return keys;
    }
    std::string sql = com_string_format("SELECT %s FROM \"%s\" ORDER BY \"%s\"",
                                        KVS_TABLE_UUID, KVS_TABLE_NAME, KVS_TABLE_ID);
    DBQuery query(fd, sql.c_str());
    kvs_close(fd);
    if (query.getRowCount() <= 0)
    {
        return keys;
    }
    for (int i = 0; i < query.getRowCount(); i++)
    {
        const char* uuid = query.getItem(i, 0);
        if (uuid != NULL)
        {
            keys.push_back(uuid);
        }
    }
    return keys;
}

ByteArray com_kvs_get_bytes(const char* file, const char* key)
{
    ByteArray bytes;
    if (file == NULL || key == NULL)
    {
        return bytes;
    }
    std::string buf = com_kvs_get_string(file, key, NULL);
    return ByteArray::FromHexString(buf.c_str());
}

bool com_kvs_get_string(const char* file, const char* key, char* value, int value_size)
{
    if (file == NULL || key == NULL || value == NULL || value_size <= 0)
    {
        return false;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return false;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" WHERE %s=\"%s\"",
                                        KVS_TABLE_NAME, KVS_TABLE_UUID, key);
    DBQuery dbResult(fd, sql.c_str());
    kvs_close(fd);
    if (dbResult.getRowCount() <= 0)
    {
        return false;
    }
    const char* uuid = dbResult.getItem(0, 1);
    const char* data = dbResult.getItem(0, 2);

    if (uuid == NULL || data == NULL)
    {
        return false;
    }
    strncpy(value, data, value_size);
    value[value_size - 1] = '\0';
    return true;
}

std::string com_kvs_get_string(const char* file, const char* key, const char* default_value)
{
    if (default_value == NULL)
    {
        default_value = "";
    }
    if (file == NULL || key == NULL)
    {
        return default_value;
    }

    void* fd = kvs_open(file);
    if (fd == NULL)
    {
        return default_value;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" WHERE %s=\"%s\"",
                                        KVS_TABLE_NAME, KVS_TABLE_UUID, key);
    DBQuery query(fd, sql.c_str());
    kvs_close(fd);
    if (query.getRowCount() <= 0)
    {
        return default_value;
    }
    const char* uuid = query.getItem(0, 1);
    const char* data = query.getItem(0, 2);

    if (uuid == NULL || data == NULL)
    {
        return default_value;
    }
    std::string val;
    val.append(data);
    return val;
}

bool com_kvs_get_bool(const char* file, const char* key, bool default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return strtol(buf, NULL, 10) == 1 ? true : false;
}

int8 com_kvs_get_int8(const char* file, const char* key, int8 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int8)strtol(buf, NULL, 10);
}

uint8 com_kvs_get_uint8(const char* file, const char* key, uint8 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint8)strtoul(buf, NULL, 10);
}

int16 com_kvs_get_int16(const char* file, const char* key, int16 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int16)strtol(buf, NULL, 10);
}

uint16 com_kvs_get_uint16(const char* file, const char* key, uint16 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint16)strtoul(buf, NULL, 10);
}

int32 com_kvs_get_int32(const char* file, const char* key, int32 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int32)strtol(buf, NULL, 10);
}

uint32 com_kvs_get_uint32(const char* file, const char* key, uint32 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint32)strtoul(buf, NULL, 10);
}

int64 com_kvs_get_int64(const char* file, const char* key, int64 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int64)strtoll(buf, NULL, 10);
}

uint64 com_kvs_get_uint64(const char* file, const char* key, uint64 default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint64)strtoull(buf, NULL, 10);
}

double com_kvs_get_double(const char* file, const char* key, double default_value)
{
    char buf[64];
    if (com_kvs_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return strtod(buf, NULL);
}

