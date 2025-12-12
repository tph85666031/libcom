#ifndef __COM_THREAD_H__
#define __COM_THREAD_H__

#include <atomic>
#include <queue>
#include <map>
#include <thread>
#include "com_base.h"
#include "com_log.h"

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

template <typename T>
class COM_EXPORT ComThreadPool
{
public:
    ComThreadPool()
    {
        thread_mgr_running = false;
        allow_duplicate_message = true;
        min_thread_count = 2;
        max_thread_count = std::max(4, (int)std::thread::hardware_concurrency());
        queue_size_per_thread = 5;
    }
    virtual ~ComThreadPool()
    {
        stopThreadPool();
    }
    ComThreadPool& setThreadsCount(int min_threads, int max_threads)
    {
        if(min_threads > 0 && max_threads > 0 && min_threads <= max_threads)
        {
            this->min_thread_count = min_threads;
            this->max_thread_count = max_threads;
        }
        return *this;
    }
    ComThreadPool& setQueueSize(int queue_size_per_thread)
    {
        if(queue_size_per_thread > 0)
        {
            this->queue_size_per_thread = queue_size_per_thread;
        }
        return *this;
    }
    ComThreadPool& setAllowDuplicateMessage(bool allow)
    {
        this->allow_duplicate_message = allow;
        return *this;
    }
    bool pushPoolMessage(T&& msg, bool urgent = false)
    {
        if(thread_mgr_running == false)
        {
            LOG_W("thread pool not running");
            return false;
        }
        mutex_msgs.lock();
        if(allow_duplicate_message == false)
        {
            for(size_t i = 0; i < msgs.size(); i++)
            {
                if(msgs[i] == msg)
                {
                    mutex_msgs.unlock();
                    return false;
                }
            }
        }
        if(urgent)
        {
            msgs.push_front(msg);
        }
        else
        {
            msgs.push_back(msg);
        }
        mutex_msgs.unlock();
        return condition.notifyOne();
    };
    bool pushPoolMessage(const T& msg, bool urgent = false)
    {
        if(thread_mgr_running == false)
        {
            LOG_W("thread pool not running");
            return false;
        }
        mutex_msgs.lock();
        msgs.push_back(msg);
        mutex_msgs.unlock();
        return condition.notifyOne();
    };
    void waitAllDone(int timeout_ms = 0)
    {
        if(thread_mgr.joinable() == false)
        {
            LOG_E("poll is not running");
            return;
        }
        do
        {
            if(getRunningCount() == 0 && getMessageCount() == 0)
            {
                break;
            }
            if(timeout_ms > 0)
            {
                timeout_ms -= 1000;
                if(timeout_ms <= 0)
                {
                    break;
                }
            }
            com_sleep_ms(1000);
        }
        while(true);
    }
    void startThreadPool()
    {
        startManagerThread();
    }
    void stopThreadPool(bool force = false)
    {
        LOG_I("stop thread pool enter");
        thread_mgr_running = false;
        do
        {
            bool all_finished = true;
            condition.notifyAll();
            mutex_threads.lock();
            std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
            for(it = threads.begin(); it != threads.end(); it++)
            {
                THREAD_POLL_INFO& des = it->second;
                if(des.running_flag != -1)
                {
                    all_finished = false;
                    break;
                }
            }
            mutex_threads.unlock();

            if(force || all_finished)
            {
                break;
            }
            com_sleep_ms(100);
        }
        while(true);

        LOG_I("stop all threads");

        mutex_threads.lock();
        std::map<std::thread::id, THREAD_POLL_INFO> threads_tmp = threads;
        mutex_threads.unlock();

        for(auto it = threads_tmp.begin(); it != threads_tmp.end(); it++)
        {
            THREAD_POLL_INFO& des = it->second;
            if(des.t != NULL && des.t->joinable())
            {
                des.t->join();
                delete des.t;
            }
        }

        mutex_threads.lock();
        threads.clear();
        mutex_threads.unlock();

        if(thread_mgr.joinable())
        {
            thread_mgr.join();
        }
    }
    virtual void threadPoolRunner(T& msg) {};
private:
    static void ThreadLoop(ComThreadPool* poll)
    {
        if(poll == NULL)
        {
            return;
        }
        std::thread::id tid = std::this_thread::get_id();
        while(poll->thread_mgr_running)
        {
            if(poll->getMessageCount() <= 0)  //数据已处理完
            {
                if(poll->getRunningCount() > poll->min_thread_count)
                {
                    //运行线程数量超过最低值,退出本线程
                    break;
                }

                //标记本线程为暂停状态
                poll->mutex_threads.lock();
                if(poll->threads.count(tid) > 0)
                {
                    THREAD_POLL_INFO& des = poll->threads[tid];
                    des.running_flag = 0;
                }
                poll->mutex_threads.unlock();

                //等待数据
                poll->condition.wait();
                continue;
            }

            //更新线程状态为执行中
            poll->mutex_threads.lock();
            if(poll->threads.count(tid) > 0)
            {
                THREAD_POLL_INFO& des = poll->threads[tid];
                des.running_flag = 1;
            }
            poll->mutex_threads.unlock();


            //继续获取数据进行处理
            poll->mutex_msgs.lock();
            while(poll->thread_mgr_running && poll->msgs.empty() == false)
            {
                T msg(std::move(poll->msgs.front()));
                poll->msgs.pop_front();
                poll->mutex_msgs.unlock();
                poll->threadPoolRunner(msg);
                poll->mutex_msgs.lock();
            }
            poll->mutex_msgs.unlock();
        }

        //本线程退出，设置标记为退出
        poll->mutex_threads.lock();
        if(poll->threads.count(tid) > 0)
        {
            THREAD_POLL_INFO& des = poll->threads[tid];
            des.running_flag = -1;
            LOG_D("pool thread exit set flag -1");
        }

        poll->mutex_threads.unlock();

        LOG_D("thread pool quit[%llu]", com_thread_get_tid());
        return;
    }
    static void ThreadManager(ComThreadPool* poll)
    {
        if(poll == NULL)
        {
            return;
        }
        com_thread_set_name("T-PoolMGR");
        poll->createThread(poll->min_thread_count);
        while(poll->thread_mgr_running)
        {
            poll->recycleThread();
            int threads_running_count = poll->getRunningCount();
            int threads_pause_count = poll->getPauseCount();
            int threads_count = threads_running_count + threads_pause_count;
            //LOG_D("threads_running_count=%d,threads_pause_count=%d",threads_running_count,threads_pause_count);
            int msg_count = poll->getMessageCount();
            if(threads_count < poll->max_thread_count &&  poll->queue_size_per_thread > 0)
            {
                int increase_count = (msg_count / poll->queue_size_per_thread) - threads_count;
                if(increase_count > poll->max_thread_count - threads_count)
                {
                    increase_count = poll->max_thread_count - threads_count;
                }
                if(increase_count > 0)
                {
                    LOG_D("thread increased, count=%d", increase_count);
                    poll->createThread(increase_count);
                }
            }
            int sleep_count = 1;
            while(poll->thread_mgr_running && sleep_count-- > 0)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    void createThread(int count)
    {
        mutex_threads.lock();
        for(int i = 0; i < count; i++)
        {
            std::thread* t = new std::thread(ThreadLoop, this);
            com_thread_set_name(t, com_string_format("T-Poll %llu", com_thread_get_tid()).c_str());
            THREAD_POLL_INFO des;
            des.t = t;
            des.running_flag = 0;
            threads[t->get_id()] = des;
        }
        mutex_threads.unlock();
    }
    void recycleThread()
    {
        mutex_threads.lock();
        std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
        for(it = threads.begin(); it != threads.end();)
        {
            THREAD_POLL_INFO& des = it->second;
            if(des.running_flag == -1 && des.t != NULL && des.t->joinable())
            {
                des.t->join();
                delete des.t;
                it = threads.erase(it);
            }
            else
            {
                it++;
            }
        }
        mutex_threads.unlock();
    }
    void startManagerThread()
    {
        thread_mgr_running = true;
        thread_mgr = std::thread(ThreadManager, this);
    }
    int getRunningCount()
    {
        int count = 0;
        mutex_threads.lock();
        std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
        for(it = threads.begin(); it != threads.end(); it++)
        {
            THREAD_POLL_INFO& des = it->second;
            if(des.running_flag == 1)
            {
                count++;
            }
        }
        mutex_threads.unlock();
        return count;
    }
    int getPauseCount()
    {
        int count = 0;
        mutex_threads.lock();
        std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
        for(it = threads.begin(); it != threads.end(); it++)
        {
            THREAD_POLL_INFO& des = it->second;
            if(des.running_flag == 0)
            {
                count++;
            }
        }
        mutex_threads.unlock();
        return count;
    }
    int getMessageCount()
    {
        int count = 0;
        mutex_msgs.lock();
        count = (int)msgs.size();
        mutex_msgs.unlock();
        return count;
    }
private:
    std::deque<T> msgs;
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
    };
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

#endif /* __COM_THREAD_H__ */
