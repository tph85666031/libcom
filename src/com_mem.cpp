#include "com_mem.h"
#include "com_file.h"
#include "com_thread.h"
#include "com_log.h"

class MemDataSyncItem final
{
public:
    std::string key;
    int flag;
};

class MemDataSyncManager final
{
public:
    MemDataSyncManager()
    {
    };
    ~MemDataSyncManager()
    {
        stopMemDataSyncManager();
    };

    void startMemDataSyncManager()
    {
        stopMemDataSyncManager();
        running = true;
        thread_runner = std::thread(ThreadRunner, this);
    };

    void stopMemDataSyncManager()
    {
        mutex.lock();
        sem_keys.post();
        mutex.unlock();
        running = false;
        if (thread_runner.joinable())
        {
            thread_runner.join();
        }
    };
    
    void notify(const char* key, int flag)
    {
        if (key == NULL)
        {
            return;
        }
        mutex.lock();
        MemDataSyncItem item;
        item.key = key;
        item.flag = flag;
        keys.push(item);
        sem_keys.post();
        mutex.unlock();
    };
    void add(const char* key, cb_mem_data_change cb)
    {
        if (key == NULL || cb == NULL)
        {
            return;
        }
        mutex.lock();
        std::map<std::string, std::vector<cb_mem_data_change>>::iterator it;
        it = hooks.find(key);
        if (it != hooks.end())
        {
            it->second.push_back(cb);
        }
        else
        {
            std::vector<cb_mem_data_change> cbs;
            cbs.push_back(cb);
            hooks[key] = cbs;
        }
        mutex.unlock();
    }
    void remove(const char* key, cb_mem_data_change cb)
    {
        if (key == NULL || cb == NULL)
        {
            return;
        }
        mutex.lock();
        std::map<std::string, std::vector<cb_mem_data_change>>::iterator it;
        it = hooks.find(key);
        if (it != hooks.end())
        {
            hooks.erase(it);
        }
        mutex.unlock();
    }
private:
    static void ThreadRunner(MemDataSyncManager* ctx)
    {
        TIME_COST();
        com_thread_set_name("T-MemSync");
        LOG_I("running...");
        while (ctx->running)
        {
            if (ctx->sem_keys.wait(5000) == false)
            {
                continue;
            }
            if (ctx->running == false)
            {
                break;
            }
            ctx->mutex.lock();
            if (ctx->keys.empty() == false)
            {
                MemDataSyncItem item = ctx->keys.front();
                ctx->keys.pop();
                if (ctx->hooks.count(item.key) != 0)
                {
                    std::vector<cb_mem_data_change> cbs = ctx->hooks[item.key];
                    for (size_t i = 0; i < cbs.size(); i++)
                    {
                        if (cbs[i] != NULL)
                        {
                            cbs[i](item.key, item.flag);
                        }
                    }
                }
            }
            ctx->mutex.unlock();
        }
        LOG_I("quit...");
    }
private:
    std::thread thread_runner;
    std::atomic<bool> running = {false};
    std::map<std::string, std::vector<cb_mem_data_change>> hooks;
    CPPMutex mutex;
    CPPSem sem_keys = {"sem_mem"};
    std::queue<MemDataSyncItem> keys;
};

static std::mutex mutex_share_mem;
static Message mem_msg;

static MemDataSyncManager& GetMemDataSyncManager()
{
    static MemDataSyncManager mem_data_sync_manager;
    return mem_data_sync_manager;
}

void InitMemDataSyncManager()
{
    GetMemDataSyncManager().startMemDataSyncManager();
}

void UninitMemDataSyncManager()
{
    GetMemDataSyncManager().stopMemDataSyncManager();
}

bool com_mem_is_key_exist(const char* key)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.isKeyExist(key);
}

void com_mem_set(const char* key, const bool val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const char val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const double val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const int8 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const int16 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const int32 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const int64 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const uint8 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const uint16 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const uint32 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const uint64 val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const std::string& val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const char* val)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, const uint8* val, int val_size)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, val, val_size);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_set(const char* key, ByteArray& bytes)
{
    mutex_share_mem.lock();
    bool exist = mem_msg.isKeyExist(key);
    mem_msg.set(key, bytes);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key,  exist ? 0 : 1);
}

void com_mem_remove(const char* key)
{
    mutex_share_mem.lock();
    mem_msg.remove(key);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify(key, -1);
}

void com_mem_remove_all()
{
    mutex_share_mem.lock();
    mem_msg.removeAll();
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify("", -2);
}

bool com_mem_get_bool(const char* key, bool default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getBool(key, default_val);
}

char com_mem_get_char(const char* key, char default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getChar(key, default_val);
}

double com_mem_get_double(const char* key, double default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getDouble(key, default_val);
}

int8 com_mem_get_int8(const char* key, int8 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getInt8(key, default_val);
}

int16 com_mem_get_int16(const char* key, int16 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getInt16(key, default_val);
}

int32 com_mem_get_int32(const char* key, int32 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getInt32(key, default_val);
}

int64 com_mem_get_int64(const char* key, int64 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getInt64(key, default_val);
}

uint8 com_mem_get_uint8(const char* key, uint8 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getUInt8(key, default_val);
}

uint16 com_mem_get_uint16(const char* key, uint16 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getUInt16(key, default_val);
}

uint32 com_mem_get_uint32(const char* key, uint32 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getUInt32(key, default_val);
}

uint64 com_mem_get_uint64(const char* key, uint64 default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getUInt64(key, default_val);
}

std::string com_mem_get_string(const char* key, std::string default_val)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getString(key, default_val);
}

ByteArray com_mem_get_bytes(const char* key)
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.getBytes(key);
}

std::string com_mem_to_json()
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg.toJSON();
}

void com_mem_from_json(std::string json)
{
    mutex_share_mem.lock();
    mem_msg = Message::FromJSON(json);
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify("", 2);
}

bool com_mem_to_file(std::string file)
{
    FILE* file_fd = com_file_open(file.c_str(), "w+");
    if (file_fd == NULL)
    {
        return false;
    }
    std::string json = com_mem_to_json();
    int ret = com_file_write(file_fd, json.data(), json.size());
    com_file_close(file_fd);
    return (ret == (int)json.size());
}

void com_mem_from_file(std::string file)
{
    ByteArray bytes = com_file_readall(file.c_str());
    com_mem_from_json(bytes.toString());
}

Message com_mem_to_message()
{
    AutoMutex mutex(&mutex_share_mem);
    return mem_msg;
}

void com_mem_from_message(Message msg)
{
    mutex_share_mem.lock();
    mem_msg = msg;
    mutex_share_mem.unlock();
    GetMemDataSyncManager().notify("", 2);
}

void com_mem_add_notify(const char* key, cb_mem_data_change cb)
{
    GetMemDataSyncManager().add(key, cb);
}

void com_mem_remove_notify(const char* key, cb_mem_data_change cb)
{
    GetMemDataSyncManager().remove(key, cb);
}

