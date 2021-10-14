#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct
{
    DWORD dwType; // must be 0x1000
    LPCSTR szName; // pointer to name (in user addr space)
    DWORD dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
#pragma pack(pop)
#else
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include "com_thread.h"
#include "com_log.h"

uint64 com_thread_get_tid()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetCurrentThreadId();
#else
    return (uint64)syscall(SYS_gettid);
#endif
}

uint64 com_thread_get_tid_posix()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetCurrentThreadId();
#else
    return (uint64)pthread_self();
#endif
}

uint64 com_thread_get_pid()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetCurrentProcessId();
#else
    return (uint64)getpid();
#endif
}

void com_thread_set_name(std::thread* t, const char* name)
{
    if (t == NULL || name == NULL)
    {
        return;
    }
#if __linux__ == 1
    char tmp[16];
    strncpy(tmp, name, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';
    pthread_setname_np(t->native_handle(), tmp);
#endif
}

std::string com_thread_get_name(std::thread* t)
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
#if __linux__ == 1
    if (t != NULL)
    {
        pthread_getname_np(t->native_handle(), buf, sizeof(buf));
    }
#endif
    std::string name = buf;
    return name;
}

void com_thread_set_name(std::thread& t, const char* name)
{
    if (name == NULL)
    {
        return;
    }
#if __linux__ == 1
    char tmp[16];
    strncpy(tmp, name, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';
    pthread_setname_np(t.native_handle(), tmp);
#endif
}

std::string com_thread_get_name(std::thread& t)
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
#if __linux__ == 1
    pthread_getname_np(t.native_handle(), buf, sizeof(buf));
#endif
    std::string name = buf;
    return name;
}

void com_thread_set_name(int tid_posix, const char* name)
{
#if __linux__ == 1
    if (tid_posix > 0 && name != NULL)
    {
        char tmp[16];
        strncpy(tmp, name, sizeof(tmp));
        tmp[sizeof(tmp) - 1] = '\0';
        pthread_setname_np(tid_posix, tmp);
    }
#endif
}

std::string com_thread_get_name(int tid_posix)
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
#if __linux__ == 1
    pthread_getname_np(tid_posix, buf, sizeof(buf));
#endif
    std::string name = buf;
    return name;
}

void com_thread_set_name(const char* name)
{
#if defined(_WIN32) || defined(_WIN64)
    if (!::IsDebuggerPresent())
    {
        return;
    }
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = com_thread_get_tid();
    info.dwFlags = 0;
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD),
                       reinterpret_cast<DWORD_PTR*>(&info));
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
#else
    char tmp[16];
    strncpy(tmp, name, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';
    prctl(PR_SET_NAME, tmp);
#endif
}

std::string com_thread_get_name()
{
    std::string name;
#if defined(_WIN32) && !defined(_WIN64)
    char* nameCache = NULL;
    __asm
    {
        mov eax, fs:[0x18]    //    Locate the caller's TIB
        mov eax, [eax + 0x14]    //    Read the pvArbitary field in the TIB
        mov [nameCache], eax    //    nameCache = pTIB->pvArbitary
    }
    if (nameCache == NULL)
    {
        return name;
    }
    //strncpy(name, nameCache, size);
    name.append(nameCache);
#elif __linux__ == 1
    char buf[32];
    memset(buf, 0, sizeof(buf));
    prctl(PR_GET_NAME, buf);
    buf[sizeof(buf) - 1] = '\0';
    name = buf;
#endif
    return name;
}

ThreadPool::ThreadPool()
{
    running = true;
    min_thread_count = 2;
    max_thread_count = 10;
    queue_size_per_thread = 5;
}

ThreadPool::~ThreadPool()
{
    stopThreadPool();
}

void ThreadPool::ThreadManager(ThreadPool* poll)
{
    if (poll == NULL)
    {
        return;
    }
    com_thread_set_name("T-PoolMGR");
    poll->createThread(poll->min_thread_count);
    while (poll->running)
    {
        poll->recycleThread();
        int threads_running_count = poll->getRunningCount();
        int threads_pause_count = poll->getPauseCount();
        int threads_count = threads_running_count + threads_pause_count;
        int msg_count = poll->getMessageCount();
        if (threads_count < poll->max_thread_count &&  poll->queue_size_per_thread > 0)
        {
            int increase_count = (msg_count / poll->queue_size_per_thread) - threads_count;
            if (increase_count > poll->max_thread_count - threads_count)
            {
                increase_count = poll->max_thread_count - threads_count;
            }
            if (increase_count > 0)
            {
                LOG_D("thread increased, count=%d", increase_count);
                poll->createThread(increase_count);
            }
        }
        int sleep_count = 1;
        while (poll->running && sleep_count-- > 0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void ThreadPool::ThreadRunner(ThreadPool* poll)
{
    if (poll == NULL)
    {
        return;
    }
    while (poll->running)
    {
        if (poll->getMessageCount() <= 0)//数据已处理完
        {
            if (poll->getRunningCount() > poll->min_thread_count)
            {
                //运行线程数量超过最低值,退出本线程
                break;
            }

            //标记本线程为暂停状态
            poll->mutex_threads.lock();
            if (poll->threads.count(std::this_thread::get_id()) > 0)
            {
                THREAD_POLL_INFO des = poll->threads[std::this_thread::get_id()];
                des.running_flag = 0;
                poll->threads[std::this_thread::get_id()] = des;
            }
            poll->mutex_threads.unlock();
            poll->condition.wait();
            continue;
        }

        //更新线程状态为执行中
        poll->mutex_threads.lock();
        if (poll->threads.count(std::this_thread::get_id()) > 0)
        {
            THREAD_POLL_INFO des = poll->threads[std::this_thread::get_id()];
            des.running_flag = 1;
            poll->threads[std::this_thread::get_id()] = des;
        }
        poll->mutex_threads.unlock();

        //继续获取数据进行处理
        poll->mutex_msgs.lock();
        if (poll->msgs.empty())
        {
            poll->mutex_msgs.unlock();
            continue;
        }
        Message msg = poll->msgs.front();
        poll->msgs.pop();
        poll->mutex_msgs.unlock();
        poll->threadPoolRunner(msg);
    }

    //本线程退出，设置标记为退出
    poll->mutex_threads.lock();
    if (poll->threads.count(std::this_thread::get_id()) > 0)
    {
        THREAD_POLL_INFO des = poll->threads[std::this_thread::get_id()];
        des.running_flag = -1;
        poll->threads[std::this_thread::get_id()] = des;
    }
    poll->mutex_threads.unlock();

    LOG_D("thread pool quit");
    return;
}

void ThreadPool::startManagerThread()
{
    thread_mgr = std::thread(ThreadManager, this);
}

void ThreadPool::createThread(int count)
{
    mutex_threads.lock();
    for (int i = 0; i < count; i++)
    {
        std::thread* t = new std::thread(ThreadRunner, this);
        com_thread_set_name(t, com_string_format("T-Poll %llu", com_thread_get_tid()).c_str());
        THREAD_POLL_INFO des;
        des.t = t;
        des.running_flag = 0;
        threads[t->get_id()] = des;
    }
    mutex_threads.unlock();
}

void ThreadPool::recycleThread()
{
    mutex_threads.lock();
    std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
    for (it = threads.begin(); it != threads.end();)
    {
        THREAD_POLL_INFO des = it->second;
        if (des.running_flag == -1 && des.t != NULL && des.t->joinable())
        {
            des.t->join();
            delete des.t;
            threads.erase(it++);
        }
        else
        {
            it++;
        }
    }
    mutex_threads.unlock();
}

ThreadPool& ThreadPool::setThreadsCount(int min_threads, int max_threads)
{
    if (min_threads > 0 && max_threads > 0 && min_threads <= max_threads)
    {
        this->min_thread_count = min_threads;
        this->max_thread_count = max_threads;
    }
    return *this;
}

ThreadPool& ThreadPool::setQueueSize(int queue_size_per_thread)
{
    if (queue_size_per_thread > 0)
    {
        this->queue_size_per_thread = queue_size_per_thread;
    }
    return *this;
}

int ThreadPool::getRunningCount()
{
    int count = 0;
    mutex_threads.lock();
    std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
    for (it = threads.begin(); it != threads.end(); it++)
    {
        THREAD_POLL_INFO des = it->second;
        if (des.running_flag == 1)
        {
            count++;
        }
    }
    mutex_threads.unlock();
    return count;
}

int ThreadPool::getPauseCount()
{
    int count = 0;
    mutex_threads.lock();
    std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
    for (it = threads.begin(); it != threads.end(); it++)
    {
        THREAD_POLL_INFO des = it->second;
        if (des.running_flag == 0)
        {
            count++;
        }
    }
    mutex_threads.unlock();
    return count;
}

int ThreadPool::getMessageCount()
{
    int count = 0;
    mutex_msgs.lock();
    count = msgs.size();
    mutex_msgs.unlock();
    return count;
}

void ThreadPool::waitAllDone(int timeout_ms)
{
    if (thread_mgr.joinable() == false)
    {
        LOG_E("poll is not running");
        return;
    }
    do
    {
        if (getRunningCount() == 0 && getMessageCount() == 0)
        {
            break;
        }
        if (timeout_ms > 0)
        {
            timeout_ms -= 1000;
            if (timeout_ms <= 0)
            {
                break;
            }
        }
        com_sleep_ms(1000);
    }
    while (true);
}

void ThreadPool::startThreadPool()
{
    startManagerThread();
}

void ThreadPool::stopThreadPool()
{
    running = false;
    do
    {
        bool all_finished = true;
        condition.notifyAll();
        mutex_threads.lock();
        std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
        for (it = threads.begin(); it != threads.end(); it++)
        {
            THREAD_POLL_INFO des = it->second;
            if (des.running_flag != -1)
            {
                all_finished = false;
                break;
            }
        }
        mutex_threads.unlock();
        if (all_finished)
        {
            break;
        }
        else
        {
            com_sleep_ms(100);
        }
    }
    while (true);

    mutex_threads.lock();
    std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
    for (it = threads.begin(); it != threads.end(); it++)
    {
        THREAD_POLL_INFO des = it->second;
        if (des.t != NULL && des.t->joinable())
        {
            des.t->join();
            delete des.t;
        }
    }
    threads.clear();
    mutex_threads.unlock();
    if (thread_mgr.joinable())
    {
        thread_mgr.join();
    }
}

bool ThreadPool::pushMessage(Message& msg)
{
    if (running == false)
    {
        return false;
    }
    mutex_msgs.lock();
    int count = max_thread_count * 10000;
    if (count < 1000)
    {
        count = 1000;
    }
    if ((int)msgs.size() > count)
    {
        mutex_msgs.unlock();
        LOG_W("thread pool queue is full");
        return false;
    }
    msgs.push(msg);
    mutex_msgs.unlock();
    condition.notifyOne();
    return true;
}

