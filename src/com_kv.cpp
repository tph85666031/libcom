#include "com_kv.h"
#include "com_log.h"
#include "com_file.h"
#include "unqlite.h"

static CPPMutex mutex_kv("mutex_kv");

static void kv_show_err(unqlite* pDb)
{
    if (pDb == NULL)
    {
        return;
    }
    const char* zErr = NULL;
    int iLen = 0;

    /* Extract the database error log */
    unqlite_config(pDb, UNQLITE_CONFIG_ERR_LOG, &zErr, &iLen);
    if (iLen > 0)
    {
        /* Output the DB error log */
        puts(zErr); /* Always null termniated */
    }
    return;
}

static void kv_destroy(unqlite* pDb)
{
    if (pDb == NULL)
    {
        return;
    }
    mutex_kv.lock();
    unqlite_lib_shutdown();
    mutex_kv.unlock();
    return;
}

void* com_kv_batch_start(const char* file)
{
    if (file == NULL)
    {
        return NULL;
    }
    unqlite* pDb = NULL;
    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_CREATE);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return NULL;
    }
    return pDb;
}

bool com_kv_batch_set(void* handle, const char* key, const void* data, int data_size)
{
    if (handle == NULL || key == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    unqlite* pDb = (unqlite*)handle;
    int ret = unqlite_kv_store(pDb, key, - 1, data, data_size);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        return false;
    }
    return true;
}

void com_kv_batch_stop(void* handle, bool rollback)
{
    if (handle == NULL)
    {
        return;
    }
    unqlite* pDb = (unqlite*)handle;
    if (rollback)
    {
        unqlite_rollback(pDb);
    }
    unqlite_close(pDb);
    kv_destroy(pDb);
    return;
}

bool com_kv_set(const char* file, const char* key, const void* data, int data_size)
{
    if (file == NULL || key == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    unqlite* pDb = NULL;

    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_CREATE);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        LOG_E("kv open failed");
        return  false;
    }

    ret = unqlite_kv_store(pDb, key, -1, data, data_size);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        LOG_E("kv store failed");
        return false;
    }

    /* Auto-commit the transaction and close our database */
    unqlite_close(pDb);
    kv_destroy(pDb);
    return true;
}

bool com_kv_set(const char* file, const char* key, const char* value)
{
    return com_kv_set(file, key, value, (int)strlen(value) + 1);
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

void com_kv_remove_all(const char* file)
{
    com_file_remove(file);
}

bool com_kv_remove(const char* file, const char* key)
{
    if (file == NULL || key == NULL)
    {
        return false;
    }
    unqlite* pDb = NULL;

    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READWRITE);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return false;
    }

    ret = unqlite_kv_delete(pDb, key, - 1);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return false;
    }

    /* Auto-commit the transaction and close our database */
    unqlite_close(pDb);
    kv_destroy(pDb);
    return true;
}

void com_kv_remove_front(const char* file, int count)
{
    std::vector<KVResult> results = com_kv_get_front(file, count);
    for (int i = 0; i < results.size(); i++)
    {
        com_kv_remove(file, results[i].key.c_str());
    }
}

void com_kv_remove_tail(const char* file, int count)
{
    std::vector<KVResult> results = com_kv_get_tail(file, count);
    for (int i = 0; i < results.size(); i++)
    {
        com_kv_remove(file, results[i].key.c_str());
    }
}


int com_kv_count(const char* file)
{
    if (file == NULL)
    {
        return -1;
    }
    unqlite* pDb = NULL;
    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READONLY);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return -1;
    }

    unqlite_kv_cursor* pCursor = NULL;
    ret = unqlite_kv_cursor_init(pDb, &pCursor);

    if (ret != UNQLITE_OK)
    {
        unqlite_close(pDb);
        kv_destroy(pDb);
        return -1;
    }

    int count = 0;
    for (unqlite_kv_cursor_first_entry(pCursor);
            unqlite_kv_cursor_valid_entry(pCursor);
            unqlite_kv_cursor_next_entry(pCursor))
    {
        count++;
    }

    unqlite_kv_cursor_release(pDb, pCursor);
    unqlite_close(pDb);
    kv_destroy(pDb);
    return count;
}

bool com_kv_exist(const char* file, const char* key)
{
    if (file == NULL || key == NULL)
    {
        return false;
    }
    unqlite* pDb = NULL;

    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READONLY);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return false;
    }

    int64 value_size = 0;
    ret = unqlite_kv_fetch(pDb, key, - 1, NULL, &value_size);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return false;
    }

    /* Auto-commit the transaction and close our database */
    unqlite_close(pDb);
    kv_destroy(pDb);
    return (value_size > 0);
}

std::vector<KVResult> com_kv_get_front(const char* file, int count)
{
    std::vector<KVResult> list;
    if (count <= 0)
    {
        return list;
    }

    if (file == NULL)
    {
        return list;
    }

    unqlite* pDb = NULL;
    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READONLY);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return list;
    }

    unqlite_kv_cursor* pCursor = NULL;
    ret = unqlite_kv_cursor_init(pDb, &pCursor);

    if (ret != UNQLITE_OK)
    {
        unqlite_close(pDb);
        kv_destroy(pDb);
        return list;
    }

    for (unqlite_kv_cursor_first_entry(pCursor);
            unqlite_kv_cursor_valid_entry(pCursor);
            unqlite_kv_cursor_next_entry(pCursor))
    {
        int key_size = 0;
        int64 data_size = 0;
        unqlite_kv_cursor_key(pCursor, NULL, &key_size);
        unqlite_kv_cursor_data(pCursor, NULL, &data_size);
        if (key_size <= 0 || data_size <= 0)
        {
            continue;
        }
        char* key = (char*)calloc(1, key_size + 1);
        if (key == NULL)
        {
            continue;
        }
        uint8* data = (uint8*)calloc(1, data_size);
        if (data == NULL)
        {
            free(key);
            continue;
        }

        unqlite_kv_cursor_key(pCursor, key, &key_size);
        unqlite_kv_cursor_data(pCursor, data, &data_size);

        KVResult result(key, data, data_size);
        free(key);
        free(data);

        list.push_back(result);
        if (list.size() >= count)
        {
            break;
        }
    }

    unqlite_kv_cursor_release(pDb, pCursor);

    unqlite_close(pDb);
    kv_destroy(pDb);
    return list;
}

std::vector<KVResult> com_kv_get_tail(const char* file, int count)
{
    std::vector<KVResult> list;
    if (file == NULL || count <= 0)
    {
        return list;
    }

    unqlite* pDb = NULL;
    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READONLY);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return list;
    }

    unqlite_kv_cursor* pCursor = NULL;
    ret = unqlite_kv_cursor_init(pDb, &pCursor);

    if (ret != UNQLITE_OK)
    {
        unqlite_close(pDb);
        kv_destroy(pDb);
        return list;
    }

    for (unqlite_kv_cursor_last_entry(pCursor);
            unqlite_kv_cursor_valid_entry(pCursor);
            unqlite_kv_cursor_prev_entry(pCursor))
    {
        int key_size = 0;
        int64 data_size = 0;
        unqlite_kv_cursor_key(pCursor, NULL, &key_size);
        unqlite_kv_cursor_data(pCursor, NULL, &data_size);
        if (key_size <= 0 || data_size <= 0)
        {
            continue;
        }
        char* key = (char*)calloc(1, key_size + 1);
        if (key == NULL)
        {
            continue;
        }
        uint8* data = (uint8*)calloc(1, data_size);
        if (data == NULL)
        {
            free(key);
            continue;
        }

        unqlite_kv_cursor_key(pCursor, key, &key_size);
        unqlite_kv_cursor_data(pCursor, data, &data_size);

        KVResult result(key, data, data_size);
        free(key);
        free(data);

        list.push_back(result);
        if (list.size() >= count)
        {
            break;
        }
    }

    unqlite_kv_cursor_release(pDb, pCursor);

    unqlite_close(pDb);
    kv_destroy(pDb);
    return list;
}

std::vector<std::string> com_kv_get_all_keys(const char* file)
{
    std::vector<std::string> keys;
    if (file == NULL)
    {
        return keys;
    }

    unqlite* pDb = NULL;
    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READONLY);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return keys;
    }

    unqlite_kv_cursor* pCursor = NULL;
    ret = unqlite_kv_cursor_init(pDb, &pCursor);

    if (ret != UNQLITE_OK)
    {
        unqlite_close(pDb);
        kv_destroy(pDb);
        return keys;
    }

    for (unqlite_kv_cursor_last_entry(pCursor);
            unqlite_kv_cursor_valid_entry(pCursor);
            unqlite_kv_cursor_prev_entry(pCursor))
    {
        int key_size = 0;
        unqlite_kv_cursor_key(pCursor, NULL, &key_size);
        if (key_size <= 0)
        {
            continue;
        }
        char* key = (char*)calloc(1, key_size + 1);
        if (key == NULL)
        {
            continue;
        }
        unqlite_kv_cursor_key(pCursor, key, &key_size);
        keys.push_back(key);
        free(key);
    }

    unqlite_kv_cursor_release(pDb, pCursor);

    unqlite_close(pDb);
    kv_destroy(pDb);
    return keys;
}

ByteArray com_kv_get_bytes(const char* file, const char* key)
{
    ByteArray bytes;
    if (file == NULL || key == NULL)
    {
        return bytes;
    }
    unqlite* pDb = NULL;

    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READONLY);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return bytes;
    }

    int64 value_size = 0;
    ret = unqlite_kv_fetch(pDb, key, -1, NULL, &value_size);
    if (ret != UNQLITE_OK || value_size <= 0)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return bytes;
    }

    uint8* value = (uint8*)calloc(1, value_size);
    if (value == NULL)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        return bytes;
    }

    ret = unqlite_kv_fetch(pDb, key, -1, value, &value_size);
    if (ret != UNQLITE_OK)
    {
        free(value);
        kv_show_err(pDb);
        kv_destroy(pDb);
        return bytes;
    }

    /* Auto-commit the transaction and close our database */
    unqlite_close(pDb);
    kv_destroy(pDb);

    bytes = ByteArray(value_size);
    bytes.append(value, value_size);
    free(value);
    return bytes;
}

bool com_kv_get_string(const char* file, const char* key, char* value, int64 value_size)
{
    if (file == NULL || key == NULL || value_size <= 0)
    {
        return false;
    }
    unqlite* pDb = NULL;

    int ret = unqlite_open(&pDb, file, UNQLITE_OPEN_READONLY);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        LOG_E("kv open failed");
        return false;
    }

    memset(value, 0, value_size);
    ret = unqlite_kv_fetch(pDb, key, - 1, value, &value_size);
    if (ret != UNQLITE_OK)
    {
        kv_show_err(pDb);
        kv_destroy(pDb);
        LOG_E("kv fetch failed");
        return false;
    }

    /* Auto-commit the transaction and close our database */
    unqlite_close(pDb);
    kv_destroy(pDb);
    value[value_size - 1] = '\0';
    return true;
}

std::string com_kv_get_string(const char* file, const char* key, const char* default_value)
{
    if (default_value == NULL)
    {
        default_value = "";
    }
    ByteArray bytes = com_kv_get_bytes(file, key);
    if (bytes.getDataSize() <= 0)
    {
        return default_value;
    }
    return bytes.toString();
}

bool com_kv_get_bool(const char* file, const char* key, bool default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return strtol(buf, NULL, 10) == 1 ? true : false;
}

int8 com_kv_get_int8(const char* file, const char* key, int8 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int8)strtol(buf, NULL, 10);
}

uint8 com_kv_get_uint8(const char* file, const char* key, uint8 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint8)strtoul(buf, NULL, 10);
}

int16 com_kv_get_int16(const char* file, const char* key, int16 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int16)strtol(buf, NULL, 10);
}

uint16 com_kv_get_uint16(const char* file, const char* key, uint16 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint16)strtoul(buf, NULL, 10);
}

int32 com_kv_get_int32(const char* file, const char* key, int32 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int32)strtol(buf, NULL, 10);
}

uint32 com_kv_get_uint32(const char* file, const char* key, uint32 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint32)strtoul(buf, NULL, 10);
}

int64 com_kv_get_int64(const char* file, const char* key, int64 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (int64)strtoll(buf, NULL, 10);
}

uint64 com_kv_get_uint64(const char* file, const char* key, uint64 default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return (uint64)strtoull(buf, NULL, 10);
}

double com_kv_get_double(const char* file, const char* key, double default_value)
{
    char buf[64] = {0};
    if (com_kv_get_string(file, key, buf, sizeof(buf)) == false)
    {
        return default_value;
    }
    return strtod(buf, NULL);
}

