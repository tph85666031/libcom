#include "com_mem.h"
#include "com_file.h"
#include "com_thread.h"
#include "com_task.h"
#include "com_log.h"

class MemDataSyncItem final
{
public:
    std::string key;
    int flag = 0;
};

class MemDataCallbackItem final
{
public:
    cb_mem_data_change cb = NULL;
    void* ctx = NULL;
    bool operator<(const MemDataCallbackItem& item) const
    {
        return (item.cb != cb);
    };
};

class MemDataSyncManager final
{
public:
    MemDataSyncManager()
    {
        startMemDataSyncManager();
    };
    ~MemDataSyncManager()
    {
        LOG_I("called");
        stopMemDataSyncManager();
    };

    void setMessageID(uint32 id)
    {
        this->message_id = id;
    };

    void startMemDataSyncManager()
    {
        LOG_I("called");
        stopMemDataSyncManager();
        running = true;
        thread_runner = std::thread(ThreadRunner, this);
    };

    void stopMemDataSyncManager()
    {
        LOG_I("called");
        mutex_keys.lock();
        sem_keys.post();
        mutex_keys.unlock();
        running = false;
        if(thread_runner.joinable())
        {
            thread_runner.join();
        }
    };

    void notify(const char* key, int flag)
    {
        if(key == NULL)
        {
            return;
        }
        mutex_keys.lock();
        MemDataSyncItem item;
        item.key = key;
        item.flag = flag;
        keys.push(item);
        mutex_keys.unlock();
        sem_keys.post();
    };
    void addListener(const char* key, cb_mem_data_change cb, void* ctx)
    {
        if(key == NULL || cb == NULL)
        {
            return;
        }
        MemDataCallbackItem item;
        item.cb = cb;
        item.ctx = ctx;

        mutex_hooks.lock();
        auto it = hooks_cb.find(key);
        if(it != hooks_cb.end())
        {
            it->second.insert(item);
        }
        else
        {
            std::set<MemDataCallbackItem> cbs;
            cbs.insert(item);
            hooks_cb[key] = cbs;
        }
        mutex_hooks.unlock();
    }
    void addListener(const char* key, const char* task_name)
    {
        if(key == NULL || task_name == NULL)
        {
            return;
        }
        mutex_hooks.lock();
        auto it = hooks_task.find(key);
        if(it != hooks_task.end())
        {
            it->second.insert(task_name);
        }
        else
        {
            std::set<std::string> tasks;
            tasks.insert(task_name);
            hooks_task[key] = tasks;
        }
        mutex_hooks.unlock();
    }
    void removeListener(const char* key, cb_mem_data_change cb = NULL)
    {
        if(key == NULL)
        {
            return;
        }
        mutex_hooks.lock();
        auto it = hooks_cb.find(key);
        if(it == hooks_cb.end())
        {
            mutex_hooks.unlock();
            return;
        }
        if(cb == NULL)
        {
            hooks_cb.erase(it);
        }
        else
        {
            MemDataCallbackItem item;
            item.cb = NULL;
            it->second.erase(item);
        }
        mutex_hooks.unlock();
    }
    void removeListener(const char* key, const char* task_name = NULL)
    {
        if(key == NULL)
        {
            return;
        }
        mutex_hooks.lock();
        auto it = hooks_task.find(key);
        if(it == hooks_task.end())
        {
            mutex_hooks.unlock();
            return;
        }
        if(task_name == NULL)
        {
            hooks_task.erase(it);
        }
        else
        {
            it->second.erase(task_name);
        }
        mutex_hooks.unlock();
    }
private:
    static void ThreadRunner(MemDataSyncManager* ctx)
    {
        TIME_COST();
        com_thread_set_name("T-MemSync");
        LOG_I("running...");
        while(ctx->running)
        {
            if(ctx->sem_keys.wait(5000) == false)
            {
                continue;
            }
            if(ctx->running == false)
            {
                break;
            }
            ctx->mutex_keys.lock();
            if(ctx->keys.empty())
            {
                //没有数据增删改事件
                ctx->mutex_keys.unlock();
                continue;
            }

            MemDataSyncItem item = ctx->keys.front();
            ctx->keys.pop();
            ctx->mutex_keys.unlock();

            std::set<MemDataCallbackItem> cbs;
            std::set<std::string> tasks;
            ctx->mutex_hooks.lock();
            if(ctx->hooks_cb.count(item.key) > 0)
            {
                cbs = ctx->hooks_cb[item.key];
            }
            if(ctx->hooks_task.count(item.key) > 0)
            {
                tasks = ctx->hooks_task[item.key];
            }
            ctx->mutex_hooks.unlock();

            for(auto it = tasks.begin(); it != tasks.end(); it++)
            {
                Message msg(ctx->message_id);
                msg.set("key", item.key);
                msg.set("flag", item.flag);
                GetTaskManager().sendMessage(it->c_str(), msg);
            }

            for(auto it = cbs.begin(); it != cbs.end(); it++)
            {
                const MemDataCallbackItem& val = *it;
                if(val.cb != NULL)
                {
                    val.cb(item.key, item.flag, val.ctx);
                }
            }
        }
        LOG_I("quit...");
    }
private:
    std::thread thread_runner;
    std::atomic<bool> running = {false};

    std::mutex mutex_hooks;
    std::map<std::string, std::set<MemDataCallbackItem>> hooks_cb;
    std::map<std::string, std::set<std::string>> hooks_task;

    std::mutex mutex_keys;
    CPPSem sem_keys = {"sem_mem"};
    std::queue<MemDataSyncItem> keys;

    std::atomic<uint32> message_id = {0xFFFFFFFE};
};

static std::mutex mutex_share_mem;
static Message mem_msg;

static MemDataSyncManager& GetMemDataSyncManager()
{
    static MemDataSyncManager mem_data_sync_manager;
    return mem_data_sync_manager;
}

bool com_mem_is_key_exist(const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.isKeyExist(key);
}

void com_mem_set(const char* key, const bool val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const char val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const double val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const int8 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const int16 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const int32 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const int64 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const uint8 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const uint16 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const uint32 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const uint64 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const std::string& val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const char* val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, const uint8* val, int val_size)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val, val_size);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_set(const char* key, CPPBytes& bytes)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, bytes);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? DATA_SYNC_UPDATED : DATA_SYNC_NEW);
}

void com_mem_remove(const char* key)
{
    mutex_share_mem.lock();
    mem_msg.remove(key);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key, DATA_SYNC_REMOVED);
}

void com_mem_clear()
{
    mutex_share_mem.lock();
    mem_msg.removeAll();
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify("", DATA_SYNC_CLEARED);
}

bool com_mem_get_bool(const char* key, bool default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getBool(key, default_val);
}

char com_mem_get_char(const char* key, char default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getChar(key, default_val);
}

double com_mem_get_double(const char* key, double default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getDouble(key, default_val);
}

int8 com_mem_get_int8(const char* key, int8 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getInt8(key, default_val);
}

int16 com_mem_get_int16(const char* key, int16 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getInt16(key, default_val);
}

int32 com_mem_get_int32(const char* key, int32 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getInt32(key, default_val);
}

int64 com_mem_get_int64(const char* key, int64 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getInt64(key, default_val);
}

uint8 com_mem_get_uint8(const char* key, uint8 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getUInt8(key, default_val);
}

uint16 com_mem_get_uint16(const char* key, uint16 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getUInt16(key, default_val);
}

uint32 com_mem_get_uint32(const char* key, uint32 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getUInt32(key, default_val);
}

uint64 com_mem_get_uint64(const char* key, uint64 default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getUInt64(key, default_val);
}

std::string com_mem_get_string(const char* key, std::string default_val)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getString(key, default_val);
}

CPPBytes com_mem_get_bytes(const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.getBytes(key);
}

std::string com_mem_to_json()
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg.toJSON();
}

void com_mem_from_json(std::string json)
{
    mutex_share_mem.lock();
    mem_msg = Message::FromJSON(json);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify("", DATA_SYNC_RELOAD);
}

bool com_mem_to_file(std::string file)
{
    FILE* file_fd = com_file_open(file.c_str(), "w+");
    if(file_fd == NULL)
    {
        return false;
    }
    std::string json = com_mem_to_json();
    int ret = com_file_write(file_fd, json.data(), (int)json.size());
    com_file_close(file_fd);
    return (ret == (int)json.size());
}

void com_mem_from_file(std::string file)
{
    CPPBytes bytes = com_file_readall(file.c_str());
    com_mem_from_json(bytes.toString());
}

Message com_mem_to_message()
{
    std::lock_guard<std::mutex> lck(mutex_share_mem);
    return mem_msg;
}

void com_mem_from_message(Message msg)
{
    mutex_share_mem.lock();
    mem_msg = msg;
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify("", DATA_SYNC_RELOAD);
}

void com_mem_add_notify(const char* key, cb_mem_data_change cb, void* ctx)
{
    GetMemDataSyncManager().addListener(key, cb, ctx);
}

void com_mem_remove_notify(const char* key, cb_mem_data_change cb)
{
    GetMemDataSyncManager().removeListener(key, cb);
}

void com_mem_add_notify(const char* key, const char* task_name)
{
    GetMemDataSyncManager().addListener(key, task_name);
}

void com_mem_remove_notify(const char* key, const char* task_name)
{
    GetMemDataSyncManager().removeListener(key, task_name);
}

