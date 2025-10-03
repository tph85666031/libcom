#include "com_sync.h"
#include "com_log.h"

SyncManager& GetSyncManager()
{
    static SyncManager sync_manager;
    return sync_manager;
}

SyncMessage::SyncMessage()
{
}

SyncMessage::SyncMessage(ComSem* sem)
{
    this->sem = sem;
}

SyncMessage::SyncMessage(const SyncMessage& msg)
{
    if(this == &msg)
    {
        return;
    }
    sem = msg.sem;
    data = msg.data;
}

SyncMessage::SyncMessage(SyncMessage&& msg)
{
    if(this == &msg)
    {
        return;
    }
    sem = msg.sem;
    data = std::move(msg.data);
}

SyncMessage::~SyncMessage()
{
}

SyncMessage& SyncMessage::operator=(const SyncMessage& msg)
{
    if(this != &msg)
    {
        sem = msg.sem;
        data = msg.data;
    }
    return *this;
}

SyncMessage& SyncMessage::operator=(SyncMessage&& msg)
{
    if(this != &msg)
    {
        sem = msg.sem;
        data = std::move(msg.data);
    }
    return *this;
}

SyncAdapter::SyncAdapter()
{
    uuid = 0;
    sem_cache_size_max = 100;
}

SyncAdapter::~SyncAdapter()
{
    clearSem();
}

void SyncAdapter::syncPrepare(uint64 uuid)
{
    std::lock_guard<std::mutex> lck(mutex_sync_map);
    if(sync_map.count(uuid) > 0)
    {
        return;
    }
    sync_map[uuid] = SyncMessage(popSem());
}

void SyncAdapter::syncCancel(uint64 uuid)
{
    std::lock_guard<std::mutex> lck(mutex_sync_map);
    if(sync_map.count(uuid) > 0)
    {
        SyncMessage& sync = sync_map[uuid];
        pushSem(sync.sem);
        sync_map.erase(uuid);
    }
    return;
}

ComBytes SyncAdapter::syncWait(uint64 uuid, int timeout_ms)
{
    ComSem* sem = NULL;
    {
        std::lock_guard<std::mutex> lck(mutex_sync_map);
        if(sync_map.count(uuid) <= 0)
        {
            return ComBytes();
        }
        SyncMessage& sync = sync_map[uuid];
        sem = sync.sem;
    }
    if(sem == NULL)
    {
        return ComBytes();
    }

    bool ret = sem->wait(timeout_ms);
    pushSem(sem);
    {
        std::lock_guard<std::mutex> lck(mutex_sync_map);
        if(sync_map.count(uuid) <= 0)
        {
            return ComBytes();
        }
        if(ret == false)
        {
            sync_map.erase(uuid);
            return ComBytes();
        }
        SyncMessage sync = std::move(sync_map[uuid]);
        sync_map.erase(uuid);
        return sync.data;
    }
}

bool SyncAdapter::syncWait(uint64 uuid, ComBytes& reply, int timeout_ms)
{
    ComSem* sem = NULL;
    {
        std::lock_guard<std::mutex> lck(mutex_sync_map);
        if(sync_map.count(uuid) <= 0)
        {
            return false;
        }
        SyncMessage& sync = sync_map[uuid];
        sem = sync.sem;
    }
    if(sem == NULL)
    {
        return false;
    }

    bool ret = sem->wait(timeout_ms);
    pushSem(sem);
    {
        std::lock_guard<std::mutex> lck(mutex_sync_map);
        if(sync_map.count(uuid) <= 0)
        {
            return false;
        }
        if(ret == false)
        {
            sync_map.erase(uuid);
            return false;
        }
        SyncMessage& sync = sync_map[uuid];
        reply = sync.data;
        sync_map.erase(uuid);
        return true;
    }
}

bool SyncAdapter::syncPost(uint64 uuid, const void* data, int data_size)
{
    std::lock_guard<std::mutex> lck(mutex_sync_map);
    if(sync_map.count(uuid) <= 0)
    {
        return false;
    }
    SyncMessage& sync = sync_map[uuid];
    if(sync.sem == NULL)
    {
        return false;
    }
    if(data != NULL && data_size > 0)
    {
        sync.data.clear();
        sync.data.append((uint8*)data, data_size);
    }
    return sync.sem->post();
}

void SyncAdapter::setSemCount(int count)
{
    if(count > 0)
    {
        sem_cache_size_max = count;
    }
}

uint64 SyncAdapter::createUUID()
{
    return uuid++;
}

void SyncAdapter::pushSem(ComSem* sem)
{
    if(sem == NULL)
    {
        return;
    }
    std::lock_guard<std::mutex> lck(mutex_sem_cache);
    if((int)sem_cache.size() > sem_cache_size_max)
    {
        delete sem;
    }
    else
    {
        sem->reset();
        sem_cache.push(sem);
    }
}

ComSem* SyncAdapter::popSem()
{
    std::lock_guard<std::mutex> lck(mutex_sem_cache);
    if(sem_cache.empty())
    {
        return new ComSem();
    }
    ComSem* sem = sem_cache.front();
    sem_cache.pop();
    return sem;
}

void SyncAdapter::clearSem()
{
    mutex_sem_cache.lock();
    while(sem_cache.empty() == false)
    {
        ComSem* sem = sem_cache.front();
        sem_cache.pop();
        if(sem != NULL)
        {
            delete sem;
        }
    }
    mutex_sem_cache.unlock();

    mutex_sync_map.lock();
    for(auto it = sync_map.begin(); it != sync_map.end(); it++)
    {
        SyncMessage& sync = it->second;
        if(sync.sem != NULL)
        {
            delete sync.sem;
        }
    }
    sync_map.clear();
    mutex_sync_map.unlock();
}

SyncManager::SyncManager()
{
}

SyncManager::~SyncManager()
{
    for(auto it = adapters.begin(); it != adapters.end(); it++)
    {
        if(it->second != NULL)
        {
            delete it->second;
        }
    }
    adapters.clear();
}

SyncAdapter& SyncManager::getAdapter(const char* name)
{
    std::string name_str;
    if(name != NULL)
    {
        name_str = name;
    }
    std::lock_guard<std::mutex> lck(mutex_adapters);
    if(adapters.count(name_str) <= 0)
    {
        adapters[name_str] = new SyncAdapter();
    }
    return *adapters[name_str];
}

SyncAdapter& SyncManager::getAdapter(const std::string& name)
{
    return getAdapter(name.c_str());
}

