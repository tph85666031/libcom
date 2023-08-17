#include "com_kv.h"
#include "com_file.h"
#include "com_log.h"
#include "com_sqlite.h"

#define KV_TABLE_NAME "kv"
#define KV_TABLE_ID   "id"
#define KV_TABLE_UUID "uuid"
#define KV_TABLE_DATA "data"

static bool kv_create_table(void* fd)
{
    std::string sql = com_string_format("CREATE TABLE \"%s\" ( \
                                        \"%s\"  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
                                        \"%s\"  TEXT UNIQUE NOT NULL, \
                                        \"%s\"  TEXT NOT NULL);",
                                        KV_TABLE_NAME,
                                        KV_TABLE_ID,
                                        KV_TABLE_UUID,
                                        KV_TABLE_DATA);
    return (com_sqlite_run_sql(fd, sql.c_str()) == 0);
}

static void* kv_open(const char* file)
{
    void* fd = com_sqlite_open(file);
    if(fd == NULL)
    {
        return NULL;
    }
    if(com_sqlite_is_table_exist(fd, KV_TABLE_NAME) == false)
    {
        if(kv_create_table(fd) == false)
        {
            com_sqlite_close(fd);
            return NULL;
        }
    }

    return fd;
}

static void kv_close(void* fd)
{
    com_sqlite_close(fd);
}

void* com_kv_batch_start(const char* file)
{
    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return NULL;
    }
    if(com_sqlite_begin_transaction(fd) == false)
    {
        kv_close(fd);
        return NULL;
    }
    return fd;
}

bool com_kv_batch_set(void* handle, const char* key, const uint8* data, int data_size)
{
    if(handle == NULL || key == NULL || data == NULL)
    {
        return false;
    }
    std::string hex_str = com_bytes_to_hexstring(data, data_size);
    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KV_TABLE_NAME,
                                        KV_TABLE_UUID,
                                        KV_TABLE_DATA,
                                        key,
                                        hex_str.c_str());
    return (com_sqlite_insert(handle, sql.c_str()) == 1);
}

bool com_kv_batch_set(void* handle, const char* key, const char* data)
{
    if(handle == NULL || key == NULL || data == NULL)
    {
        return false;
    }

    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KV_TABLE_NAME,
                                        KV_TABLE_UUID,
                                        KV_TABLE_DATA,
                                        key,
                                        data);

    return (com_sqlite_insert(handle, sql.c_str()) == 1);
}

bool com_kv_batch_remove(void* handle, const char* key)
{
    if(handle == NULL || key == NULL)
    {
        return false;
    }

    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE %s=\"%s\";",
                                        KV_TABLE_NAME,
                                        KV_TABLE_UUID, key);

    return (com_sqlite_delete(handle, sql.c_str()) == 1);
}

void com_kv_batch_stop(void* handle, bool rollback)
{
    if(handle == NULL)
    {
        return;
    }
    if(rollback)
    {
        com_sqlite_rollback_transaction(handle);
    }
    else
    {
        com_sqlite_commit_transaction(handle);
    }
    kv_close(handle);

    return;
}

bool com_kv_set(const char* file, const char* key, const uint8* data, int data_size)
{
    if(file == NULL || key == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return false;
    }

    std::string hex_str = com_bytes_to_hexstring((uint8*)data, data_size);
    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KV_TABLE_NAME,
                                        KV_TABLE_UUID,
                                        KV_TABLE_DATA,
                                        key,
                                        hex_str.c_str());
    int ret = com_sqlite_insert(fd, sql.c_str());
    kv_close(fd);
    return (ret == 1);
}

bool com_kv_set(const char* file, const char* key, const char* value)
{
    if(file == NULL || key == NULL || value == NULL)
    {
        return false;
    }
    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return false;
    }

    std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s) VALUES ('%s', '%s');",
                                        KV_TABLE_NAME,
                                        KV_TABLE_UUID,
                                        KV_TABLE_DATA,
                                        key,
                                        value);

    int ret = com_sqlite_insert(fd, sql.c_str());
    kv_close(fd);
    return (ret == 1);
}

bool com_kv_set(const char* file, const char* key, bool value)
{
    return com_kv_set(file, key, value ? "1" : "0");
}

bool com_kv_set(const char* file, const char* key, int8 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, uint8 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, int16 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, uint16 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, int32 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, uint32 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, int64 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, uint64 value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%llu", value);
    return com_kv_set(file, key, buf);
}

bool com_kv_set(const char* file, const char* key, double value)
{
    std::string val = std::to_string(value);
    return com_kv_set(file, key, val.c_str());
}

bool com_kv_remove(const char* file, const char* key)
{
    if(file == NULL || key == NULL)
    {
        return false;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return false;
    }
    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE %s=\"%s\";",
                                        KV_TABLE_NAME,
                                        KV_TABLE_UUID, key);

    int ret = com_sqlite_delete(fd, sql.c_str());
    kv_close(fd);
    return (ret == 1);
}

void com_kv_remove_all(const char* file)
{
    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return;
    }
    com_sqlite_table_clean(fd, KV_TABLE_NAME);
    kv_close(fd);
    return;
}

void com_kv_remove_front(const char* file, int count)
{
    if(file == NULL || count <= 0)
    {
        return;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return;
    }

    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\" IN(SELECT \"%s\" FROM \"%s\" ORDER BY \"%s\" LIMIT %d)",
                                        KV_TABLE_NAME, KV_TABLE_ID,
                                        KV_TABLE_ID, KV_TABLE_NAME,
                                        KV_TABLE_ID, count);
    com_sqlite_delete(fd, sql.c_str());
    kv_close(fd);
    return;
}

void com_kv_remove_tail(const char* file, int count)
{
    if(file == NULL || count <= 0)
    {
        return;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return;
    }

    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\" IN(SELECT \"%s\" FROM \"%s\" ORDER BY \"%s\" DESC LIMIT %d)",
                                        KV_TABLE_NAME, KV_TABLE_ID,
                                        KV_TABLE_ID, KV_TABLE_NAME,
                                        KV_TABLE_ID, count);
    com_sqlite_delete(fd, sql.c_str());
    kv_close(fd);
    return;
}

int com_kv_count(const char* file)
{
    if(file == NULL)
    {
        return -1;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return -1;
    }

    int ret = com_sqlite_table_row_count(fd, KV_TABLE_NAME);
    kv_close(fd);
    return ret;
}

bool com_kv_exist(const char* file, const char* key)
{
    if(file == NULL || key == NULL)
    {
        return false;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return false;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" WHERE %s=\"%s\"",
                                        KV_TABLE_NAME, KV_TABLE_UUID, key);
    DBQuery query(fd, sql.c_str());
    kv_close(fd);
    if(query.getCount() <= 0)
    {
        return false;
    }
    std::string uuid = query.getItem(0, KV_TABLE_UUID);
    std::string data = query.getItem(0, KV_TABLE_DATA);

    if(uuid.empty() || data.empty())
    {
        return false;
    }
    return true;
}

std::vector<KVResult> com_kv_get_front(const char* file, int count)
{
    std::vector<KVResult> list;
    if(file == NULL || count <= 0)
    {
        return list;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return list;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" ORDER BY \"%s\" LIMIT %d",
                                        KV_TABLE_NAME, KV_TABLE_ID, count);
    DBQuery query(fd, sql.c_str());
    kv_close(fd);
    for(int i = 0; i < query.getCount(); i++)
    {
        std::string uuid = query.getItem(i, KV_TABLE_UUID);
        std::string data = query.getItem(i, KV_TABLE_DATA);

        KVResult result(uuid.c_str(), (uint8*)data.c_str(), data.length());
        list.push_back(result);
    }
    return list;
}

std::vector<KVResult> com_kv_get_tail(const char* file, int count)
{
    std::vector<KVResult> list;
    if(file == NULL || count <= 0)
    {
        return list;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return list;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" ORDER BY \"%s\" DESC LIMIT %d",
                                        KV_TABLE_NAME, KV_TABLE_ID, count);
    DBQuery query(fd, sql.c_str());
    kv_close(fd);
    for(int i = 0; i < query.getCount(); i++)
    {
        std::string uuid = query.getItem(i, KV_TABLE_UUID);
        std::string data = query.getItem(i, KV_TABLE_DATA);

        KVResult result(uuid.c_str(), (uint8*)data.c_str(), data.length());
        list.push_back(result);
    }
    return list;
}

std::vector<std::string> com_kv_get_all_keys(const char* file)
{
    std::vector<std::string> keys;
    if(file == NULL)
    {
        return keys;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return keys;
    }
    std::string sql = com_string_format("SELECT %s FROM \"%s\" ORDER BY \"%s\"",
                                        KV_TABLE_UUID, KV_TABLE_NAME, KV_TABLE_ID);
    DBQuery query(fd, sql.c_str());
    kv_close(fd);
    if(query.getCount() <= 0)
    {
        return keys;
    }
    for(int i = 0; i < query.getCount(); i++)
    {
        std::string uuid = query.getItem(i, KV_TABLE_UUID);
        if(uuid.empty() == false)
        {
            keys.push_back(uuid);
        }
    }
    return keys;
}

CPPBytes com_kv_get_bytes(const char* file, const char* key)
{
    CPPBytes bytes;
    if(file == NULL || key == NULL)
    {
        return bytes;
    }
    std::string buf = com_kv_get_string(file, key, NULL);
    return CPPBytes::FromHexString(buf.c_str());
}

bool com_kv_get_string(const char* file, const char* key, char* value, int value_size)
{
    if(file == NULL || key == NULL || value == NULL || value_size <= 0)
    {
        return false;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return false;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" WHERE %s=\"%s\"",
                                        KV_TABLE_NAME, KV_TABLE_UUID, key);
    DBQuery dbResult(fd, sql.c_str());
    kv_close(fd);
    if(dbResult.getCount() <= 0)
    {
        return false;
    }
    std::string uuid = dbResult.getItem(0, KV_TABLE_UUID);
    std::string data = dbResult.getItem(0, KV_TABLE_DATA);

    if(uuid.empty() || data.empty())
    {
        return false;
    }
    strncpy(value, data.c_str(), value_size);
    value[value_size - 1] = '\0';
    return true;
}

std::string com_kv_get_string(const char* file, const char* key, const char* default_value)
{
    if(default_value == NULL)
    {
        default_value = "";
    }
    if(file == NULL || key == NULL)
    {
        return default_value;
    }

    void* fd = kv_open(file);
    if(fd == NULL)
    {
        return default_value;
    }
    std::string sql = com_string_format("SELECT * FROM \"%s\" WHERE %s=\"%s\"",
                                        KV_TABLE_NAME, KV_TABLE_UUID, key);
    DBQuery query(fd, sql.c_str());
    kv_close(fd);
    if(query.getCount() <= 0)
    {
        return default_value;
    }
    std::string uuid = query.getItem(0, KV_TABLE_UUID);
    std::string data = query.getItem(0, KV_TABLE_DATA);

    if(uuid.empty() || data.empty())
    {
        return default_value;
    }
    return data;
}

bool com_kv_get_bool(const char* file, const char* key, bool default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return strtol(buf, NULL, 10) == 1 ? true : false;
}

int8 com_kv_get_int8(const char* file, const char* key, int8 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int8)strtol(buf, NULL, 10);
}

uint8 com_kv_get_uint8(const char* file, const char* key, uint8 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint8)strtoul(buf, NULL, 10);
}

int16 com_kv_get_int16(const char* file, const char* key, int16 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int16)strtol(buf, NULL, 10);
}

uint16 com_kv_get_uint16(const char* file, const char* key, uint16 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint16)strtoul(buf, NULL, 10);
}

int32 com_kv_get_int32(const char* file, const char* key, int32 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int32)strtol(buf, NULL, 10);
}

uint32 com_kv_get_uint32(const char* file, const char* key, uint32 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint32)strtoul(buf, NULL, 10);
}

int64 com_kv_get_int64(const char* file, const char* key, int64 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int64)strtoll(buf, NULL, 10);
}

uint64 com_kv_get_uint64(const char* file, const char* key, uint64 default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint64)strtoull(buf, NULL, 10);
}

double com_kv_get_double(const char* file, const char* key, double default_value)
{
    char buf[64];
    if(com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return strtod(buf, NULL);
}

KVResult::KVResult(const char* key, uint8* data, int dataSize)
{
    this->data = NULL;
    this->data_size = 0;
    if (key != NULL && data != NULL && dataSize > 0)
    {
        this->key = key;
        this->data = new uint8[dataSize]();
        this->data_size = dataSize;
        memcpy(this->data, data, dataSize);
    }
}

KVResult::KVResult(const KVResult& result)
{
    if (this->data != NULL)
    {
        delete[] this->data;
    }
    this->key = result.key;
    this->data = NULL;
    this->data_size = 0;

    if (result.data != NULL && result.data_size > 0)
    {
        this->data = new uint8[result.data_size]();
        this->data_size = result.data_size;
        memcpy(this->data, result.data, result.data_size);
    }
}

KVResult& KVResult::operator=(const KVResult& result)
{
    if (this != &result)
    {
        if (this->data != NULL)
        {
            delete[] this->data;
        }

        this->key = result.key;
        this->data = NULL;
        this->data_size = 0;


        if (result.data != NULL && result.data_size > 0)
        {
            this->data = new uint8[result.data_size]();
            this->data_size = result.data_size;
            memcpy(this->data, result.data, result.data_size);
        }
    }
    return *this;
}

KVResult::~KVResult()
{
    if (data != NULL)
    {
        delete[] data;
        data = NULL;
    }
}


