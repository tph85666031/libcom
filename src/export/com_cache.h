#ifndef __COM_CACHE_H__
#define __COM_CACHE_H__

#include "com_base.h"

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    int64_t expiretime_ms = -1;
    int retry_count = -1;
    int user_flag = 0;
} CACHE_FLAG;
#pragma pack(pop)

class Cache
{
public:
    Cache();
    Cache(const char* uuid);
    Cache(const std::string& uuid);
    Cache(const char* uuid, uint8_t* data, int data_size);
    Cache(const std::string& uuid, uint8_t* data, int data_size);
    virtual ~Cache();
    std::string& getUUID();
    uint8_t* getData();
    int getDataSize();
    int64_t getExpireTime();
    int getRetryCount();
    int getUserFlag();
    bool expired();
    Cache& setData(CPPBytes& bytes);
    Cache& setData(const uint8_t* data, int data_size);
    Cache& setData(const char* data);
    Cache& setData(const char* data, int data_size);
    Cache& setUUID(const char* uuid);
    Cache& setUUID(const std::string& uuid);
    Cache& setExpireTime(int64_t expiretime_ms);
    Cache& setRetryCount(int retry_count);
    Cache& setUserFlag(int flag);
    bool empty();
    std::string toString();
    std::string toHexString(bool upper = false);
private:
    std::string uuid;
	CPPBytes bytes;
    CACHE_FLAG flag;
};


/*
CacheManager由一级内存缓存+二级磁盘存储构成
数据增删改查优先从一级缓冲中获取，若一级缓存中没有命中再从磁盘缓存中获取
一级缓存会定期刷写到磁盘缓存（顺序不定）
*/
class CacheManager
{
public:
    CacheManager();
    virtual ~CacheManager();

    CacheManager& setFlushInterval(int interval_s);
    CacheManager& setFlushCount(int count);
    CacheManager& setMemoryCacheCount(int count);
    
    bool startManager(const char* db_file);
    void stopManager();

    void append(const char* uuid, uint8_t* data, int data_size);
    void append(const std::string& uuid, uint8_t* data, int data_size);
    void append(Cache& cache);
    void removeByUUID(const char* uuid);
    void removeByUUID(const std::string& uuid);
    void removeFirst(int count);
    void removeLast(int count);
    void clear(bool clearFile = false);

    bool waitCache(int timeout_ms);
    Cache getFirst();
    Cache getLast();
    std::vector<std::string> getAllUUID();
    std::vector<Cache> getFirst(int count);
    std::vector<Cache> getLast(int count);
    Cache getByUUID(const char* uuid);
    Cache getByUUID(const std::string& uuid);
    int getCacheCount();
    void flushToDisk();
    void dilution(int step);
    void increaseRetryCount(const std::string& uuid, int count = 1);
    void increaseRetryCount(const char* uuid, int count = 1);
    void decreaseRetryCount(const std::string& uuid, int count = 1);
    void decreaseRetryCount(const char* uuid, int count = 1);
private:
    bool initDB();
    void removeExpired();
    void removeRetryFinished();
    void setRetryCount(const std::string& uuid, int count);
    void setRetryCount(const char* uuid, int count);
    void insertHead(Cache& cache);
    void insertHead(const char* uuid, uint8_t* data, int data_size);
    void insertHead(const std::string& uuid, uint8_t* data, int data_size);
    void insertTail(Cache& cache);
    void insertTail(const char* uuid, uint8_t* data, int data_size);
    void insertTail(const std::string& uuid, uint8_t* data, int data_size);
    static void ThreadFlushRunner(CacheManager* cache_manager);

private:
    std::string db_file;
    CPPMutex mutex_sqlite_fd;
    void* sqlite_fd = NULL;
    int mem_cache_count_max = -1;
    int flush_interval_s = 30;
    int flush_count = 100;
    std::vector<Cache> list;
    CPPMutex mutex_list = {"CacheManager::mutex_list"};
    CPPSem sem_list = {"CacheManager::sem_list"};
    std::thread thread_flush;
    std::atomic<bool> thread_flush_running;
};

#endif /* __COM_CACHE_H__ */
