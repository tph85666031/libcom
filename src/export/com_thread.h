#ifndef __COM_THREAD_H__
#define __COM_THREAD_H__

#include <atomic>
#include <queue>
#include <map>
#include <thread>
#include "com_base.h"

#if defined(_WIN32) || defined(_WIN64)
typedef HANDLE process_sem_t;
typedef HANDLE process_mutex_t;
typedef HANDLE process_cond_t;
typedef HANDLE process_share_mem_t;
#else
typedef int process_sem_t;
typedef int process_mutex_t;
typedef int process_cond_t;
typedef int process_share_mem_t;
#endif

#define PROC_STAT_IDEL                   0
#define PROC_STAT_RUNNING                1
#define PROC_STAT_SLEEP_INTERRUPTIBLE    2
#define PROC_STAT_SLEEP_UNINTERRUPTIBLE  3
#define PROC_STAT_DEAD                   4
#define PROC_STAT_ZOMBIE                 5
#define PROC_STAT_TRACED                 6
#define PROC_STAT_PARKED                 7

class COM_EXPORT ProcInfo
{
public:
    ProcInfo();
public:
    bool valid = false;
    std::string path;
    int stat = -1;
    int pid = -1;
    int pgrp = -1;
    int uid = -1;
    int euid = -1;
    int suid = -1;
    int fsuid = -1;
    int gid = -1;
    int egid = -1;
    int sgid = -1;
    int fsgid = -1;
    int ppid = -1;
    int rpid = -1;//valid for MacOS only
    int session_id = -1;
    int thread_count = 0;
};

COM_EXPORT bool com_process_exist(int pid);
COM_EXPORT int com_process_join(int pid);
COM_EXPORT int com_process_create(const char* app, std::vector<std::string> args = std::vector<std::string>());
COM_EXPORT int com_process_get_pid(const char* name = NULL);
COM_EXPORT std::vector<int> com_process_get_pid_all(const char* name = NULL);
COM_EXPORT int com_process_get_ppid(int pid = -1);
COM_EXPORT std::string com_process_get_path(int pid = -1);
COM_EXPORT std::string com_process_get_name(int pid = -1);

COM_EXPORT void com_thread_set_name(std::thread* t, const char* name);
COM_EXPORT void com_thread_set_name(std::thread& t, const char* name);
COM_EXPORT void com_thread_set_name(uint64 tid_posix, const char* name);
COM_EXPORT void com_thread_set_name(const char* name);
COM_EXPORT ProcInfo com_process_get(int pid);
COM_EXPORT std::map<int, ProcInfo> com_process_get_all();
COM_EXPORT ProcInfo com_process_get_parent(int pid = -1);
COM_EXPORT std::vector<ProcInfo> com_process_get_parent_all(int pid = -1);
COM_EXPORT std::vector<ProcInfo> com_process_get_child(int pid);
COM_EXPORT std::vector<ProcInfo> com_process_get_child_all(int pid);
COM_EXPORT std::string com_thread_get_name(std::thread* t);
COM_EXPORT std::string com_thread_get_name(std::thread& t);
COM_EXPORT std::string com_thread_get_name(uint64 tid_posix);
COM_EXPORT std::string com_thread_get_name();
COM_EXPORT uint64 com_thread_get_tid_posix();
COM_EXPORT uint64 com_thread_get_tid();
COM_EXPORT std::vector<uint64> com_thread_get_tid_all(int pid = -1);

#pragma pack(push)
#pragma pack(4)
typedef struct
{
    std::thread* t;
    int running_flag;//-1=quit,0=pause,1=running,
} THREAD_POLL_INFO;
#pragma pack(pop)

class COM_EXPORT ComThreadPool
{
public:
    ComThreadPool();
    virtual ~ComThreadPool();
    ComThreadPool& setThreadsCount(int min_threads, int max_threads);
    ComThreadPool& setQueueSize(int queue_size_per_thread);
    ComThreadPool& setAllowDuplicateMessage(bool allow);
    bool pushPoolMessage(Message&& msg, bool urgent = false);
    bool pushPoolMessage(const Message& msg, bool urgent = false);
    void waitAllDone(int timeout_ms = 0);
    void startThreadPool();
    void stopThreadPool(bool force = false);
    virtual void threadPoolRunner(Message& msg) {};
private:
    static void ThreadLoop(ComThreadPool* poll);
    static void ThreadManager(ComThreadPool* poll);
    void createThread(int count);
    void recycleThread();
    void startManagerThread();
    int getRunningCount();
    int getPauseCount();
    int getMessageCount();
private:
    std::deque<Message> msgs;
    std::map<std::thread::id, THREAD_POLL_INFO> threads;
    std::mutex mutex_msgs;
    std::mutex mutex_threads;
    ComCondition condition;
    std::atomic<int> min_thread_count;
    std::atomic<int> max_thread_count;
    std::atomic<int> queue_size_per_thread;
    std::atomic<bool> thread_mgr_running;
    std::thread thread_mgr;
    bool allow_duplicate_message;
};

template <typename T>
class COM_EXPORT ThreadRunner
{
public:
    ThreadRunner()
    {
        thread_runner_running = true;
        thread_runner = std::thread(ThreadLoop, this);
    };
    virtual ~ThreadRunner()
    {
        thread_runner_running = false;
        if(thread_runner.joinable())
        {
            thread_runner.join();
        }
    };

    bool pushRunnerMessage(const T& msg)
    {
        if(thread_runner_running == false)
        {
            return false;
        }
        mutex_msgs.lock();
        msgs.push(msg);
        mutex_msgs.unlock();
        condition.notifyOne();
        return true;
    };

    bool pushRunnerMessage(T&& msg)
    {
        if(thread_runner_running == false)
        {
            return false;
        }
        mutex_msgs.lock();
        msgs.push(msg);
        mutex_msgs.unlock();
        condition.notifyOne();
        return true;
    };

    size_t getMessageCount()
    {
        std::lock_guard<std::mutex> lck(mutex_msgs);
        return msgs.size();
    }
private:
    virtual void threadRunner(T& msg) {};
    static void ThreadLoop(ThreadRunner<T>* ctx)
    {
        if(ctx == NULL)
        {
            return;
        }
        while(ctx->thread_runner_running)
        {
            ctx->condition.wait(1000);
            if(ctx->thread_runner_running == false)
            {
                break;
            }

            ctx->mutex_msgs.lock();
            if(ctx->msgs.empty())
            {
                ctx->mutex_msgs.unlock();
                continue;
            }
            T msg = ctx->msgs.front();
            ctx->msgs.pop();
            ctx->mutex_msgs.unlock();
            ctx->threadRunner(msg);
        }
        return;
    };
private:
    std::mutex mutex_msgs;
    std::queue<T> msgs;
    ComCondition condition;

    std::thread thread_runner;
    std::atomic<bool> thread_runner_running = {false};
};

class COM_EXPORT ComProcessSem
{
public:
    ComProcessSem();
    ComProcessSem(const char* name, int init_val = 0);//通过构造函数创建
    virtual ~ComProcessSem();

    bool init(const char* name, int init_val = 0);//通过init方法创建
    void uninit(bool destroy = false);//destroy=true将从系统删除此Sem，(非WIn系统下)其它进程使用此命名的Sem下的API会立即返回
    int post();
    int wait(int timeout_ms = 0);
protected:
    process_sem_t sem;
};
typedef ComProcessSem DEPRECATED("Use ComProcessSem instead") CPPProcessSem;

class COM_EXPORT ComProcessMutex
{
public:
    ComProcessMutex();
    ComProcessMutex(const char* name);//通过构造函数创建
    virtual ~ComProcessMutex();

    bool init(const char* name);//通过init方法创建
    void uninit(bool destroy = false);//destroy=true将从系统删除此Mutex，其它进程使用此命名的Mutex下的API会立即返回

    int lock();
    int trylock();
    int unlock();
    void reset(bool to_unlocked = true);
protected:
    process_mutex_t mutex;
};
typedef ComProcessMutex DEPRECATED("Use ComProcessMutex instead") CPPProcessMutex;

class COM_EXPORT ComProcessCondition
{
public:
    ComProcessCondition();
    ComProcessCondition(const char* name);//通过构造函数创建
    virtual ~ComProcessCondition();

    //相同name，不同的offset也算不同的cond
    bool init(const char* name);//通过init方法创建
    void uninit(bool destroy = false);//destroy=true将从系统删除此Condition，其它进程使用此命名的Condition下的API会立即返回

    int wait(int timeout_ms = 0);
    int notifyOne();
    int notifyAll();
private:
    process_cond_t cond;
};
typedef ComProcessCondition DEPRECATED("Use ComProcessCondition instead") CPPProcessCondition;

class COM_EXPORT ComShareMemoryV
{
public:
    ComShareMemoryV();
    ComShareMemoryV(const char* name, int size);//通过构造函数创建
    virtual ~ComShareMemoryV();

    void* init(const char* name, int size);//通过init方法创建
    void uninit();
    void* getAddr();
private:
    process_share_mem_t mem;
    void* msg;
};
typedef ComShareMemoryV DEPRECATED("Use ComShareMemoryV instead") CPPShareMemoryV;

class COM_EXPORT ComShareMemoryMap
{
public:
    ComShareMemoryMap();
    ComShareMemoryMap(const char* name, int size);//通过构造函数创建
    virtual ~ComShareMemoryMap();
    void* init(const char* name, int size);//通过init方法创建
    void uninit();
    void* getAddr();
private:
    process_share_mem_t mem;
    int msg_size;
    void* msg;
};
typedef ComShareMemoryMap DEPRECATED("Use ComShareMemoryMap instead") CPPShareMemoryMap;

#endif /* __COM_THREAD_H__ */

