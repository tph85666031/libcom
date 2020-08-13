#include "com_cache.h"
#include "com_thread.h"
#include "com_file.h"
#include "com_sqlite.h"
#include "com_log.h"

using namespace std;

#define CACHEMANAGER_FLUSH_CHECKINTERVAL_MS   1000

#define CACHE_MANAGER_TABLE_NAME   "cm"
#define CACHE_MANAGER_TABLE_ID     "id"
#define CACHE_MANAGER_TABLE_UUID   "uuid"
#define CACHE_MANAGER_TABLE_RETRY  "retry"
#define CACHE_MANAGER_TABLE_EXPIRE "expire"
#define CACHE_MANAGER_TABLE_FLAG   "flag"
#define CACHE_MANAGER_TABLE_DATA   "data"

Cache::Cache()
{
}

Cache::Cache(const char* uuid)
{
    if (uuid != NULL)
    {
        this->uuid = uuid;
    }
}

Cache::Cache(const std::string& uuid)
{
    this->uuid = uuid;
}

Cache::Cache(const char* uuid, uint8* data, int data_size)
{
    if (uuid != NULL)
    {
        this->uuid = uuid;
    }
    this->bytes = ByteArray(data, data_size);
}

Cache::Cache(const std::string& uuid, uint8* data, int data_size)
{
    this->uuid = uuid;
    this->bytes = ByteArray(data, data_size);
}

Cache::~Cache()
{
}

std::string& Cache::getUUID()
{
    return uuid;
}

uint8* Cache::getData()
{
    return bytes.getData();
}

int Cache::getDataSize()
{
    return bytes.getDataSize();
}

int64 Cache::getExpireTime()
{
    return this->flag.expiretime_ms;
}

int Cache::getRetryCount()
{
    return this->flag.retry_count;
}

int Cache::getUserFlag()
{
    return this->flag.user_flag;
}

bool Cache::expired()
{
    uint64 time = com_time_rtc_ms();
    if (time <= 1577808000000)//2020-01-01 00:00:00
    {
        return false;
    }
    if (flag.expiretime_ms > 0 && com_time_rtc_ms() > (uint64)flag.expiretime_ms)
    {
        return true;
    }
    return false;
}

bool Cache::empty()
{
    return bytes.empty();
}

std::string Cache::toString()
{
    return bytes.toString();
}

std::string Cache::toHexString(bool upper)
{
    return bytes.toHexString(upper);
}

Cache& Cache::setData(ByteArray& bytes)
{
    this->bytes = bytes;
    return *this;
}

Cache& Cache::setData(const uint8* data, int data_size)
{
    this->bytes = ByteArray(data, data_size);
    return *this;
}

Cache& Cache::setData(const char* data)
{
    setData(data, strlen(data));
    return *this;
}

Cache& Cache::setData(const char* data, int data_size)
{
    this->bytes = ByteArray(data, data_size);
    return *this;
}

Cache& Cache::setUUID(const char* uuid)
{
    if (uuid != NULL)
    {
        this->uuid = uuid;
    }
    return *this;
}

Cache& Cache::setUUID(const std::string& uuid)
{
    this->uuid = uuid;
    return *this;
}

Cache& Cache::setExpireTime(int64 expiretime_ms)
{
    this->flag.expiretime_ms = expiretime_ms;
    return *this;
}

Cache& Cache::setRetryCount(int retry_count)
{
    this->flag.retry_count = retry_count;
    return *this;
}

Cache& Cache::setUserFlag(int user_flag)
{
    this->flag.user_flag = user_flag;
    return *this;
}

void CacheManager::ThreadFlushRunner(CacheManager* manager)
{
    if (manager == NULL)
    {
        return;
    }
    int interval = 0;
    bool need_flush = false;
    while (manager->thread_flush_running)
    {
        need_flush = false;
        if (manager->flush_interval_s > 0)
        {
            if (interval >= manager->flush_interval_s * 1000)
            {
                need_flush = true;
                interval  = 0;
                LOG_D("interval matched, flush to disk");
            }
            interval += CACHEMANAGER_FLUSH_CHECKINTERVAL_MS;
        }
        if (manager->flush_count > 0)
        {
            manager->mutex_list.lock();
            if ((int)manager->list.size() >= manager->flush_count)
            {
                need_flush = true;
                LOG_D("count matched, flush to disk");
            }
            manager->mutex_list.unlock();
        }
        if (need_flush)
        {
            manager->flushToDisk();
            manager->removeExpired();
            manager->removeRetryFinished();
        }
        com_sleep_ms(CACHEMANAGER_FLUSH_CHECKINTERVAL_MS);
    }
    return;
}

CacheManager::CacheManager()
{
}

CacheManager::~CacheManager()
{
    stop();
}

bool CacheManager::start(const char* db_file)
{
    if (db_file == NULL)
    {
        return false;
    }
    this->db_file = db_file;
    thread_flush_running = true;
    thread_flush = std::thread(ThreadFlushRunner, this);
    return true;
}

void CacheManager::stop()
{
    thread_flush_running = false;
    if (thread_flush.joinable())
    {
        thread_flush.join();
    }
    flushToDisk();
    mutex_sqlite_fd.lock();
    com_sqlite_close(sqlite_fd);
    sqlite_fd = NULL;
    mutex_sqlite_fd.unlock();
}

bool CacheManager::initDB()
{
    AutoMutex a(mutex_sqlite_fd);
    if (com_file_type(db_file.c_str()) != FILE_TYPE_FILE)
    {
        com_sqlite_close(sqlite_fd);
        sqlite_fd = NULL;
    }
    if (sqlite_fd != NULL)
    {
        return true;
    }
    sqlite_fd = com_sqlite_open(db_file.c_str());
    if (sqlite_fd == NULL)
    {
        return false;
    }
    if (com_sqlite_is_table_exist(sqlite_fd, CACHE_MANAGER_TABLE_NAME) == false)
    {
        std::string sql = com_string_format("CREATE TABLE \"%s\" ( \
                                        \"%s\"  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
                                        \"%s\"  TEXT UNIQUE NOT NULL, \
                                        \"%s\"  INTEGER NOT NULL, \
                                        \"%s\"  INTEGER NOT NULL, \
                                        \"%s\"  INTEGER NOT NULL, \
                                        \"%s\"  TEXT NOT NULL);",
                                            CACHE_MANAGER_TABLE_NAME,
                                            CACHE_MANAGER_TABLE_ID,
                                            CACHE_MANAGER_TABLE_UUID,
                                            CACHE_MANAGER_TABLE_RETRY,
                                            CACHE_MANAGER_TABLE_EXPIRE,
                                            CACHE_MANAGER_TABLE_FLAG,
                                            CACHE_MANAGER_TABLE_DATA);
        if (com_sqlite_run_sql(sqlite_fd, sql.c_str()) != BCP_SQL_ERR_OK)
        {
            com_sqlite_close(sqlite_fd);
            sqlite_fd = NULL;
            return false;
        }
    }
    return true;
}

void CacheManager::insertHead(const char* uuid, uint8* data, int data_size)
{
    Cache cache(uuid, data, data_size);
    insertHead(cache);
}

void CacheManager::insertHead(const std::string& uuid, uint8* data, int data_size)
{
    Cache cache(uuid, data, data_size);
    insertHead(cache);
}

void CacheManager::insertHead(Cache& cache)
{
    bool need_flush = false;
    mutex_list.lock();
    if (mem_cache_count_max == 0) //禁用内存缓存,直接写入磁盘
    {
        std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s,%s,%s,%s) VALUES ('%s',%d,%lld,%d,'%s');",
                                            CACHE_MANAGER_TABLE_NAME,
                                            CACHE_MANAGER_TABLE_UUID,
                                            CACHE_MANAGER_TABLE_RETRY,
                                            CACHE_MANAGER_TABLE_EXPIRE,
                                            CACHE_MANAGER_TABLE_FLAG,
                                            CACHE_MANAGER_TABLE_DATA,
                                            cache.getUUID().c_str(),
                                            cache.getRetryCount(),
                                            cache.getExpireTime(),
                                            cache.getUserFlag(),
                                            cache.toHexString().c_str());

        initDB();
        mutex_sqlite_fd.lock();
        if (com_sqlite_run_sql(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
        {
            com_sqlite_close(sqlite_fd);
            sqlite_fd = NULL;
        }
        mutex_sqlite_fd.unlock();
    }
    else
    {
        list.insert(list.begin(), cache);
        if ((int)list.size() > mem_cache_count_max)
        {
            need_flush = true;
        }
    }
    sem_list.post();
    mutex_list.unlock();
    if (need_flush)
    {
        flushToDisk();
    }
}

void CacheManager::insertTail(const char* uuid, uint8* data, int data_size)
{
    Cache cache(uuid, data, data_size);
    insertTail(cache);
}

void CacheManager::insertTail(const std::string& uuid, uint8* data, int data_size)
{
    Cache cache(uuid, data, data_size);
    insertTail(cache);
}

void CacheManager::insertTail(Cache& cache)
{
    bool need_flush = false;
    mutex_list.lock();
    if (mem_cache_count_max == 0) //禁用内存缓存,直接写入磁盘
    {
        std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s,%s,%s,%s) VALUES ('%s',%d,%lld,%d,'%s');",
                                            CACHE_MANAGER_TABLE_NAME,
                                            CACHE_MANAGER_TABLE_UUID,
                                            CACHE_MANAGER_TABLE_RETRY,
                                            CACHE_MANAGER_TABLE_EXPIRE,
                                            CACHE_MANAGER_TABLE_FLAG,
                                            CACHE_MANAGER_TABLE_DATA,
                                            cache.getUUID().c_str(),
                                            cache.getRetryCount(),
                                            cache.getExpireTime(),
                                            cache.getUserFlag(),
                                            cache.toHexString().c_str());

        initDB();
        mutex_sqlite_fd.lock();
        int ret = com_sqlite_run_sql(sqlite_fd, sql.c_str());
        if (ret != BCP_SQL_ERR_OK)
        {
            if (ret == BCP_SQL_ERR_FAILED)
            {
                com_sqlite_close(sqlite_fd);
                sqlite_fd = NULL;
            }
            LOG_W("save to cache file failed");
        }
        mutex_sqlite_fd.unlock();
    }
    else
    {
        list.push_back(cache);
        if ((int)list.size() > mem_cache_count_max)
        {
            need_flush = true;
        }
    }
    sem_list.post();
    mutex_list.unlock();
    if (need_flush)
    {
        flushToDisk();
    }
}

void CacheManager::append(const char* uuid, uint8* data, int data_size)
{
    insertTail(uuid, data, data_size);
}

void CacheManager::append(const std::string& uuid, uint8* data, int data_size)
{
    insertTail(uuid, data, data_size);
}

void CacheManager::append(Cache& cache)
{
    insertTail(cache);
}

void CacheManager::removeByUUID(const char* uuid)
{
    if (uuid == NULL)
    {
        return;
    }
    return removeByUUID(std::string(uuid));
}

void CacheManager::removeByUUID(const std::string& uuid)
{
    mutex_list.lock();
    vector<Cache>::iterator it;
    for (it = list.begin(); it != list.end(); it++)
    {
        if (it->getUUID() == uuid)
        {
            list.erase(it);
            break;
        }
    }
    mutex_list.unlock();

    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE %s=\"%s\";",
                                        CACHE_MANAGER_TABLE_NAME,
                                        CACHE_MANAGER_TABLE_UUID, uuid.c_str());

    initDB();
    mutex_sqlite_fd.lock();
    if (com_sqlite_delete(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
    {
        com_sqlite_close(sqlite_fd);
        sqlite_fd = NULL;
    }
    mutex_sqlite_fd.unlock();
}

void CacheManager::removeFirst(int count)
{
    initDB();
    mutex_sqlite_fd.lock();
    int count_disk = com_sqlite_table_row_count(sqlite_fd, CACHE_MANAGER_TABLE_NAME);
    if (count_disk > count)
    {
        count_disk = count;
    }
    int count_mem = count - count_disk;
    if (count_disk > 0)
    {
        std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\" IN(SELECT \"%s\" FROM \"%s\" ORDER BY \"%s\" LIMIT %d)",
                                            CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_ID,
                                            CACHE_MANAGER_TABLE_ID, CACHE_MANAGER_TABLE_NAME,
                                            CACHE_MANAGER_TABLE_ID, count);
        if (com_sqlite_delete(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
        {
            com_sqlite_close(sqlite_fd);
            sqlite_fd = NULL;
        }
    }
    mutex_sqlite_fd.unlock();
    if (count_mem > 0)
    {
        mutex_list.lock();
        if (count_mem > (int)list.size())
        {
            count_mem = list.size();
        }
        list.erase(list.begin(), list.begin() + count_mem);
        mutex_list.unlock();
    }
}

void CacheManager::removeLast(int count)
{
    mutex_list.lock();
    int count_mem = list.size();
    if (count_mem > count)
    {
        count_mem = count;
    }
    if (count_mem > 0)
    {
        list.erase(list.end() - count_mem, list.end());
    }
    mutex_list.unlock();

    int count_disk = count - count_mem;
    if (count_disk > 0)
    {
        std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\" IN(SELECT \"%s\" FROM \"%s\" ORDER BY \"%s\" DESC LIMIT %d)",
                                            CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_ID,
                                            CACHE_MANAGER_TABLE_ID, CACHE_MANAGER_TABLE_NAME,
                                            CACHE_MANAGER_TABLE_ID, count);
        initDB();
        mutex_sqlite_fd.lock();
        if (com_sqlite_delete(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
        {
            com_sqlite_close(sqlite_fd);
            sqlite_fd = NULL;
        }
        mutex_sqlite_fd.unlock();
    }
}

void CacheManager::removeExpired()
{
    uint64 time = com_time_rtc_ms();
    if (time > 1577808000000)//2020-01-01 00:00:00
    {
        std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\">0 AND \"%s\"<%lld",
                                            CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_EXPIRE,
                                            CACHE_MANAGER_TABLE_EXPIRE, com_time_rtc_ms());
        initDB();
        mutex_sqlite_fd.lock();
        if (com_sqlite_delete(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
        {
            com_sqlite_close(sqlite_fd);
            sqlite_fd = NULL;
        }
        mutex_sqlite_fd.unlock();
    }
}

void CacheManager::removeRetryFinished()
{
    std::string sql = com_string_format("DELETE FROM \"%s\" WHERE \"%s\"=0",
                                        CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_RETRY);
    initDB();
    mutex_sqlite_fd.lock();
    if (com_sqlite_delete(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
    {
        com_sqlite_close(sqlite_fd);
        sqlite_fd = NULL;
    }
    mutex_sqlite_fd.unlock();
}

void CacheManager::setRetryCount(const std::string& uuid, int count)
{
    setRetryCount(uuid.c_str(), count);
}

void CacheManager::setRetryCount(const char* uuid, int count)
{
    if(uuid==NULL)
    {
        return;
    }
    mutex_list.lock();
    vector<Cache>::iterator it;
    for (it = list.begin(); it != list.end(); it++)
    {
        if (com_string_equal(it->getUUID().c_str(), uuid))
        {
            it->setRetryCount(count);
            mutex_list.unlock();
            return;
        }
    }
    mutex_list.unlock();

    std::string sql = com_string_format("UPDATE \"%s\" SET \"%s\"=%d WHERE \"%s\"='%s'",
                                        CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_RETRY, count,
                                        CACHE_MANAGER_TABLE_UUID, uuid);
    initDB();
    mutex_sqlite_fd.lock();
    if (com_sqlite_update(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
    {
        com_sqlite_close(sqlite_fd);
        sqlite_fd = NULL;
    }
    mutex_sqlite_fd.unlock();
}

void CacheManager::increaseRetryCount(const std::string& uuid, int count)
{
    increaseRetryCount(uuid.c_str(), count);
}

void CacheManager::increaseRetryCount(const char* uuid, int count)
{
    Cache cache = getByUUID(uuid);
    if (cache.empty() || cache.getRetryCount() < 0)
    {
        return;
    }
    setRetryCount(uuid, cache.getRetryCount() + count);
}

void CacheManager::decreaseRetryCount(const std::string& uuid, int count)
{
    decreaseRetryCount(uuid.c_str(), count);
}

void CacheManager::decreaseRetryCount(const char* uuid, int count)
{
    Cache cache = getByUUID(uuid);
    if (cache.empty() || cache.getRetryCount() < 0)
    {
        return;
    }
    count = cache.getRetryCount() - count;
    if (count <= 0)
    {
        removeByUUID(uuid);
    }
    else
    {
        setRetryCount(uuid, count);
    }
}

void CacheManager::clear(bool clear_file)
{
    mutex_list.lock();
    list.clear();
    mutex_list.unlock();
    if (clear_file)
    {
        initDB();
        mutex_sqlite_fd.lock();
        com_sqlite_table_clean(sqlite_fd, CACHE_MANAGER_TABLE_NAME);
        mutex_sqlite_fd.unlock();
    }
}

bool CacheManager::waitCache(int timeout_ms)
{
    return sem_list.wait(timeout_ms);
}

int CacheManager::getCacheCount()
{
    int count = 0;
    mutex_list.lock();
    count += list.size();
    mutex_list.unlock();
    initDB();
    mutex_sqlite_fd.lock();
    int val = com_sqlite_table_row_count(sqlite_fd, CACHE_MANAGER_TABLE_NAME);
    mutex_sqlite_fd.unlock();
    if (val > 0)
    {
        count += val;
    }
    return count;
}

std::vector<Cache> CacheManager::getFirst(int count)
{
    std::vector<Cache> rets;
    //get from file
    std::string sql = com_string_format("SELECT * FROM \"%s\" ORDER BY \"%s\" LIMIT %d",
                                        CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_ID, count);
    initDB();
    mutex_sqlite_fd.lock();
    DBQuery query(sqlite_fd, sql.c_str());
    mutex_sqlite_fd.unlock();
    for (int i = 0; i < query.getRowCount(); i++)
    {
        const char* uuid = query.getItem(i, 1);
        const char* retry = query.getItem(i, 2);
        const char* expire = query.getItem(i, 3);
        const char* flag = query.getItem(i, 4);
        const char* data = query.getItem(i, 5);

        ByteArray bytes = ByteArray::FromHexString(data);

        Cache cache(uuid);
        cache.setRetryCount(strtol(retry, NULL, 10));
        cache.setExpireTime(strtoll(expire, NULL, 10));
        cache.setUserFlag(strtol(flag, NULL, 10));
        cache.setData(bytes);
        rets.push_back(cache);
    }
    if ((int)rets.size() < count)
    {
        //get from list
        count = count - rets.size();
        mutex_list.lock();
        if (list.empty() == false)
        {
            if (count > (int)list.size())
            {
                count = list.size();
            }
            for (int i = 0; i < count; i++)
            {
                rets.push_back(list.at(i));
            }
        }
        mutex_list.unlock();
    }
    return rets;
}

std::vector<Cache> CacheManager::getLast(int count)
{
    std::vector<Cache> rets;
    //get from list
    mutex_list.lock();
    if (list.empty() == false)
    {
        if (count > (int)list.size())
        {
            count = list.size();
        }
        for (size_t i = list.size(); i < list.size() - count; i--)
        {
            rets.push_back(list.at(i));
        }
    }
    mutex_list.unlock();
    if ((int)rets.size() < count)
    {
        count = count - rets.size();
        //get from file
        std::string sql = com_string_format("SELECT * FROM \"%s\" ORDER BY \"%s\" DESC LIMIT %d",
                                            CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_ID, count);
        initDB();
        mutex_sqlite_fd.lock();
        DBQuery query(sqlite_fd, sql.c_str());
        mutex_sqlite_fd.unlock();
        for (int i = 0; i < query.getRowCount(); i++)
        {
            const char* uuid = query.getItem(i, 1);
            const char* retry = query.getItem(i, 2);
            const char* expire = query.getItem(i, 3);
            const char* flag = query.getItem(i, 4);
            const char* data = query.getItem(i, 5);

            ByteArray bytes = ByteArray::FromHexString(data);

            Cache cache(uuid);
            cache.setRetryCount(strtol(retry, NULL, 10));
            cache.setExpireTime(strtoll(expire, NULL, 10));
            cache.setUserFlag(strtol(flag, NULL, 10));
            cache.setData(bytes);
            rets.push_back(cache);
        }
    }
    return rets;
}

//获取最先添加的数据
Cache CacheManager::getFirst()
{
    std::vector<Cache> caches = getFirst(1);
    if (caches.empty())
    {
        Cache cache;
        return cache;
    }
    return caches[0];
}

//获取最后添加的数据
Cache CacheManager::getLast()
{
    std::vector<Cache> caches = getLast(1);
    if (caches.empty())
    {
        Cache cache;
        return cache;
    }
    return caches[0];
}

void CacheManager::dilution(int step)
{
    flushToDisk();
    std::string sql = com_string_format("SELECT %s FROM \"%s\" ORDER BY \"%s\"",
                                        CACHE_MANAGER_TABLE_UUID, CACHE_MANAGER_TABLE_NAME,
                                        CACHE_MANAGER_TABLE_ID);
    initDB();
    AutoMutex a(mutex_sqlite_fd);
    DBQuery query(sqlite_fd, sql.c_str());
    if (query.getRowCount() <= 0)
    {
        return;
    }
    com_sqlite_begin_transaction(sqlite_fd);
    for (int i = 0; i < query.getRowCount(); i = i + 1 + step)
    {
        const char* uuid = query.getItem(i, 0);
        if (uuid != NULL)
        {
            sql = com_string_format("DELETE FROM \"%s\" WHERE %s=\"%s\";",
                                    CACHE_MANAGER_TABLE_NAME,
                                    CACHE_MANAGER_TABLE_UUID, uuid);

            if (com_sqlite_delete(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
            {
                com_sqlite_close(sqlite_fd);
                sqlite_fd = NULL;
            }
        }
    }
    com_sqlite_commit_transaction(sqlite_fd);
}

std::vector<std::string> CacheManager::getAllUUID()
{
    std::vector<std::string> uuids;

    //get from list
    mutex_list.lock();
    vector<Cache>::iterator it;
    for (it = list.begin(); it != list.end(); it++)
    {
        uuids.push_back(it->getUUID());
    }
    mutex_list.unlock();

    //get form file
    std::string sql = com_string_format("SELECT %s FROM \"%s\" ORDER BY \"%s\"",
                                        CACHE_MANAGER_TABLE_UUID, CACHE_MANAGER_TABLE_NAME,
                                        CACHE_MANAGER_TABLE_ID);
    initDB();
    mutex_sqlite_fd.lock();
    DBQuery query(sqlite_fd, sql.c_str());
    mutex_sqlite_fd.unlock();
    for (int i = 0; i < query.getRowCount(); i++)
    {
        const char* uuid = query.getItem(i, 0);
        if (uuid != NULL)
        {
            uuids.push_back(uuid);
        }
    }
    return uuids;
}

Cache CacheManager::getByUUID(const char* uuid)
{
    if (uuid == NULL)
    {
        return Cache();
    }
    return getByUUID(std::string(uuid));
}

Cache CacheManager::getByUUID(const std::string& uuid)
{
    //get from list
    mutex_list.lock();
    vector<Cache>::iterator it;
    for (it = list.begin(); it != list.end(); it++)
    {
        if (it->getUUID() == uuid)
        {
            mutex_list.unlock();
            return *it;
        }
    }
    mutex_list.unlock();
    //get form file
    Cache cache(uuid);
    std::string sql = com_string_format("SELECT * FROM \"%s\" WHERE %s=\"%s\"",
                                        CACHE_MANAGER_TABLE_NAME, CACHE_MANAGER_TABLE_UUID, uuid.c_str());
    initDB();
    mutex_sqlite_fd.lock();
    DBQuery query(sqlite_fd, sql.c_str());
    mutex_sqlite_fd.unlock();
    if (query.getRowCount() > 0)
    {
        const char* retry = query.getItem(0, 2);
        const char* expire = query.getItem(0, 3);
        const char* flag = query.getItem(0, 4);
        const char* data = query.getItem(0, 5);
        ByteArray bytes = ByteArray::FromHexString(data);

        cache.setRetryCount(strtol(retry, NULL, 10));
        cache.setExpireTime(strtoll(expire, NULL, 10));
        cache.setUserFlag(strtol(flag, NULL, 10));
        cache.setData(bytes);
    }
    return cache;
}

void CacheManager::flushToDisk()
{
    initDB();
    mutex_list.lock();
    AutoMutex a(mutex_sqlite_fd);
    com_sqlite_begin_transaction(sqlite_fd);
    vector<Cache>::iterator it;
    for (it = list.begin(); it != list.end(); it++)
    {
        std::string sql = com_string_format("REPLACE INTO \"%s\" (%s,%s,%s,%s,%s) VALUES ('%s',%d,%lld,%d,'%s');",
                                            CACHE_MANAGER_TABLE_NAME,
                                            CACHE_MANAGER_TABLE_UUID,
                                            CACHE_MANAGER_TABLE_RETRY,
                                            CACHE_MANAGER_TABLE_EXPIRE,
                                            CACHE_MANAGER_TABLE_FLAG,
                                            CACHE_MANAGER_TABLE_DATA,
                                            it->getUUID().c_str(),
                                            it->getRetryCount(),
                                            it->getExpireTime(),
                                            it->getUserFlag(),
                                            it->toHexString().c_str());

        if (com_sqlite_run_sql(sqlite_fd, sql.c_str()) == BCP_SQL_ERR_FAILED)
        {
            com_sqlite_close(sqlite_fd);
            sqlite_fd = NULL;
        }

    }
    list.clear();
    com_sqlite_commit_transaction(sqlite_fd);
    mutex_list.unlock();
}

CacheManager& CacheManager::setFlushInterval(int interval_s)
{
    this->flush_interval_s = interval_s;
    return *this;
}

CacheManager& CacheManager::setFlushCount(int count)
{
    this->flush_count = count;
    return *this;
}

CacheManager& CacheManager::setMemoryCacheCount(int count)
{
    this->mem_cache_count_max = count;
    return *this;
}

