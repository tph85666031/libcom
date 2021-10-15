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
uint64_t com_thread_get_tid_posix();
uint64_t com_thread_get_tid();
uint64_t com_thread_get_pid();

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

#endif /* __COM_THREAD_H__ */

