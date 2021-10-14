#ifndef __COM_THREAD_H__
#define __COM_THREAD_H__

#include <atomic>
#include <queue>
#include <map>
#include <thread>
#include "com_base.h"
#include "com_log.h"

void com_thread_set_name(std::thread* t, const char* name);
void com_thread_set_name(std::thread& t, const char* name);
void com_thread_set_name(int tid, const char* name);
void com_thread_set_name(const char* name);
std::string com_thread_get_name(std::thread* t);
std::string com_thread_get_name(std::thread& t);
std::string com_thread_get_name(int tid);
std::string com_thread_get_name();
uint64 com_thread_get_tid_posix();
uint64 com_thread_get_tid();
uint64 com_thread_get_pid();

typedef struct
{
    std::thread* t;
    int running_flag;//-1=quit,0=pause,1=running,
} THREAD_POLL_INFO;

class ThreadPool
{
public:
    ThreadPool();
    virtual ~ThreadPool();
    ThreadPool& setThreadsCount(int minThreads, int maxThreads);
    ThreadPool& setQueueSize(int queue_size_per_thread);
    bool pushMessage(Message& msg);
    void waitAllDone(int timeout_ms = 0);
    void startThreadPool();
    void stopThreadPool();
    virtual void threadPoolRunner(Message& msg) {};
private:
    static void ThreadRunner(ThreadPool* poll);
    static void ThreadManager(ThreadPool* poll);
    void createThread(int count);
    void recycleThread();
    void startManagerThread();
    int getRunningCount();
    int getPauseCount();
    int getMessageCount();
private:
    std::queue<Message> msgs;
    std::map<std::thread::id, THREAD_POLL_INFO> threads;
    std::mutex mutex_msgs;
    std::mutex mutex_threads;
    CPPCondition condition;
    std::atomic<int> min_thread_count;
    std::atomic<int> max_thread_count;
    std::atomic<int> queue_size_per_thread;
    std::atomic<bool> running;
    std::thread thread_mgr;
};

template<class T>
class EasyThread
{
public:
    EasyThread()
    {
    }

    virtual ~EasyThread()
    {
        stopThread();
        //msgs.clear();
    }

    void setName(const char* name)
    {
        if (name != NULL)
        {
            this->name = name;
        }
    }

    std::string getName()
    {
        return this->name;
    }

    void startThread()
    {
        if (thread_runner.native_handle() == com_thread_get_tid_posix())
        {
            return;
        }
        running = true;
        thread_runner = std::thread(RunnerWapper, this);
    }

    void stopThread()
    {
        running = false;
        if (thread_runner.native_handle() != com_thread_get_tid_posix())
        {
            if (thread_runner.joinable())
            {
                thread_runner.join();
            }
        }
    }

    void pushMessage(const T& msg)
    {
        mutex_msgs.lock();
        msgs.push(msg);
        sem.post();
        mutex_msgs.unlock();
    }

    bool waitMessage(int timeout_ms)
    {
        return sem.wait(timeout_ms);
    }

    bool getMessage(T& msg)
    {
        mutex_msgs.lock();
        if (msgs.empty())
        {
            mutex_msgs.unlock();
            return false;
        }
        msg = msgs.front();
        msgs.pop();
        mutex_msgs.unlock();
        return true;;
    }

private:
    virtual void onLoop()
    {
        sem.wait(1000);
    }

    static void RunnerWapper(EasyThread* thread)
    {
        if (thread == NULL)
        {
            return;
        }
        com_thread_set_name(thread->name.c_str());
        while (thread->running)
        {
            uint64 time_diff = com_time_cpu_ms();
            thread->onLoop();
            if (com_time_cpu_ms() - time_diff <= 1)
            {
                LOG_W("！！！CPU COST HIGH FOR THREAD %s", thread->name.c_str());
            }
        }
        LOG_D("%s quit", thread->name.c_str());
    }
private:
    std::mutex mutex_msgs;
    std::queue<T> msgs;
    std::atomic<bool> running = {false};
    std::thread thread_runner;
    std::string name;
    CPPSem sem;
};


#endif /* __COM_THREAD_H__ */

