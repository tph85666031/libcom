#ifndef __COM_SYNC_H__
#define __COM_SYNC_H__

#include "com_base.h"

class COM_EXPORT SyncMessage final
{
public:
    SyncMessage();
    SyncMessage(ComSem* sem);
    SyncMessage(const SyncMessage& msg);
    SyncMessage(SyncMessage&& msg);

    ~SyncMessage();
    SyncMessage& operator=(const SyncMessage& msg);
    SyncMessage& operator=(SyncMessage&& msg);
    ComSem* sem = NULL;
    ComBytes data;
};

class COM_EXPORT SyncAdapter
{
public:
    SyncAdapter();
    virtual ~SyncAdapter();

    void syncPrepare(uint64 uuid);
    ComBytes syncWait(uint64 uuid, int timeout_ms = 1000);
    bool syncWait(uint64 uuid, ComBytes& reply, int timeout_ms = 1000);
    bool syncPost(uint64 uuid, const void* data, int data_size);
    void syncCancel(uint64 uuid);

    void setSemCount(int count);
    uint64 createUUID();
private:
    void pushSem(ComSem* sem);
    ComSem* popSem();
    void clearSem();
private:
    std::mutex mutex_sync_map;
    std::map<uint64, SyncMessage> sync_map;

    std::mutex mutex_sem_cache;
    std::queue<ComSem*> sem_cache;

    std::atomic<int> sem_cache_size_max;
    std::atomic<uint64> uuid;
};

class COM_EXPORT SyncManager
{
public:
    SyncManager();
    virtual ~SyncManager();

    SyncAdapter& getAdapter(const char* name);
    SyncAdapter& getAdapter(const std::string& name);
private:
    std::mutex mutex_adapters;
    std::map<std::string, SyncAdapter*> adapters;
};

COM_EXPORT SyncManager& GetSyncManager();

#endif /* __COM_SYNC_H__ */

