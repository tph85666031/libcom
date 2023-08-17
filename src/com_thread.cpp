#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <tlhelp32.h>
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
#elif defined(__APPLE__)
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#else
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include "com_thread.h"
#include "com_file.h"
#include "com_log.h"

bool com_process_exist(int pid)
{
    if(pid < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if(process == NULL)
    {
        return false;
    }
    DWORD ret = WaitForSingleObject(process, INFINITE);
    CloseHandle(process);
    return (ret == WAIT_TIMEOUT);
#else
    return (kill(pid, 0) == 0);
#endif
}

int com_process_join(int pid)
{
    if(pid < 0)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    WaitForSingleObject(process, INFINITE);
    unsigned long code = 0;
    GetExitCodeProcess(process, &code);
    CloseHandle(process);
    return (int)code;
#else
    int status = 0;
    if(waitpid(pid, &status, 0) == -1)
    {
        return -1;
    }
    if(WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    else if(WIFSIGNALED(status))
    {
        return WTERMSIG(status);
    }
    else if(WCOREDUMP(status))
    {
        return -2;
    }
    return -1;
#endif
}

int com_process_create(const char* app, std::vector<std::string> args)
{
    if(app == NULL)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    std::string val = app;
    for(size_t i = 0; i < args.size(); i++)
    {
        val += " " + args[i];
    }

    std::vector<char> cmd;
    cmd.assign(val.begin(), val.end());
    cmd.push_back('\0');
    if(CreateProcessA(NULL, &cmd[0], NULL, NULL, false, 0, NULL, NULL, &si, &pi) == false)
    {
        return -1;
    }
    return pi.dwProcessId;
#else
    //屏蔽SIGCHLD信号防止出现僵尸进程
    struct sigaction sig_action_old;
    memset(&sig_action_old, 0, sizeof(sig_action_old));
    sigaction(SIGCHLD, NULL, &sig_action_old);
    signal(SIGCHLD, SIG_IGN);
    int ret = fork();
    if(ret != 0)
    {
        sigaction(SIGCHLD, &sig_action_old, NULL);
        return ret;
    }

    for(int fd = 3; fd < sysconf(_SC_OPEN_MAX); fd++)
    {
        close(fd);
    }

    size_t argc = args.size() + 2;
    char** argv = new char* [argc];
    argv[0] = strdup(app);
    for(size_t i = 0; i < args.size(); i++)
    {
        argv[i + 1] = strdup(args[i].c_str());
    }
    argv[argc - 1] = NULL;
    ret = execvp(app, (char* const*)argv);
    for(size_t i = 0; i < argc; i++)
    {
        printf("failed,ret=%d,errno=%d,argv[%zu]=%s\n", ret, errno, i, argv[i]);
        if(argv[i] != NULL)
        {
            free(argv[i]);
        }
    }
    delete[] argv;
    exit(0);
#endif
}

int com_process_get_pid(const char* name)
{
    if(name == NULL)
    {
#if defined(_WIN32) || defined(_WIN64)
        return (int)GetCurrentProcessId();
#else
        return (int)getpid();
#endif
    }

#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        return -1;
    }
    PROCESSENTRY32 pe;
    int pid = 0;
    for(bool ret = Process32First(handle_snapshot, &pe); ret; ret = Process32Next(handle_snapshot, &pe))
    {
        if(com_string_equal(pe.szExeFile, name))
        {
            pid = pe.th32ProcessID;
            break;
        }
    }
    CloseHandle(handle_snapshot);
    return pid;
#else
    std::map<std::string, int> list;
    com_dir_list("/proc", list);
    std::string name_expect = com_path_name(name);
    char name_cur[128];
    for(auto it = list.begin(); it != list.end(); it++)
    {
        if(it->second != FILE_TYPE_DIR)
        {
            continue;
        }
        std::string content = com_file_readall(com_string_format("%s/stat", it->first.c_str()).c_str()).toString();
        if(content.empty())
        {
            continue;
        }
        int pid = 0;
        int ppid = 0;
        int gid = 0;
        name_cur[0] = '\0';
        if(sscanf(content.c_str(), "%d %s %*s %d %d %*d", &pid, name_cur, &ppid, &gid) != 4)
        {
            continue;
        }
        com_string_replace(name_cur, ')', '\0');
        name_cur[sizeof(name_cur) - 1] = '\0';
        if(com_string_equal(name_expect.c_str(), name_cur + 1))
        {
            return pid;
        }
    }
    return -1;
#endif
}

std::vector<int> com_process_get_pid_all(const char* name)
{
    std::vector<int> pids;
    if(name == NULL)
    {
#if defined(_WIN32) || defined(_WIN64)
        pids.push_back((int)GetCurrentProcessId());
#else
        pids.push_back((int)getpid());
#endif
        return pids;
    }

#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        return pids;
    }
    PROCESSENTRY32 pe;
    int pid = 0;
    for(bool ret = Process32First(handle_snapshot, &pe); ret; ret = Process32Next(handle_snapshot, &pe))
    {
        if(com_string_equal(pe.szExeFile, name))
        {
            pids.push_back(pe.th32ProcessID);
        }
    }
    CloseHandle(handle_snapshot);
#else
    std::map<std::string, int> list;
    com_dir_list("/proc", list);
    char name_cur[128];
    for(auto it = list.begin(); it != list.end(); it++)
    {
        if(it->second != FILE_TYPE_DIR)
        {
            continue;
        }
        std::string content = com_file_readall(com_string_format("%s/stat", it->first.c_str()).c_str()).toString();
        if(content.empty())
        {
            continue;
        }
        int pid = 0;
        int ppid = 0;
        int gid = 0;
        name_cur[0] = '\0';
        if(sscanf(content.c_str(), "%d (%s) %*s %d %d %*d", &pid, name_cur, &ppid, &gid) != 4)
        {
            continue;
        }
        name_cur[sizeof(name_cur) - 1] = '\0';
        if(com_string_equal(name, name_cur))
        {
            pids.push_back(pid);
        }
    }
#endif
    return pids;
}

int com_process_get_ppid(int pid)
{
    if(pid < 0)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(pid == 0)
    {
        pid = com_process_get_pid();
    }
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        return -1;
    }
    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(PROCESSENTRY32);
    int ppid = 0;
    for(bool ret = Process32First(handle_snapshot, &pe); ret; ret = Process32Next(handle_snapshot, &pe))
    {
        if(pe.th32ProcessID == pid)
        {
            ppid = pe.th32ParentProcessID;
        }
    }
    CloseHandle(handle_snapshot);
    return ppid;
#else
    if(pid == 0)
    {
        return getppid();
    }
    else
    {
        std::string content = com_file_readall(com_string_format("/proc/%d/stat", pid).c_str()).toString();
        if(content.empty())
        {
            return -1;
        }
        int pid_check = 0;
        int ppid = 0;
        int gid = 0;
        if(sscanf(content.c_str(), "%d %*s %*s %d %d %*d", &pid_check, &ppid, &gid) != 3)
        {
            return -1;
        }
        return ppid;
    }
#endif
}

std::string com_process_get_name(int pid)
{
    if(pid < 0)
    {
        return std::string();
    }
    if(pid == 0)
    {
        pid = com_process_get_pid();
    }
#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        return std::string();
    }
    PROCESSENTRY32 pe;
    for(bool ret = Process32First(handle_snapshot, &pe); ret; ret = Process32Next(handle_snapshot, &pe))
    {

        if(pe.th32ProcessID == pid)
        {
            return pe.szExeFile;
        }
    }
    CloseHandle(handle_snapshot);
    return std::string();
#else
    std::vector<char> app(4096);
    int ret = readlink(com_string_format("/proc/%d/exe", pid).c_str(), &app[0], app.size() - 1);
    if(ret <= 0)
    {
        return std::string();
    }
    app[ret] = '\0';
    return com_path_name(&app[0]);
#endif
}

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

void com_thread_set_name(std::thread* t, const char* name)
{
    if(t == NULL || name == NULL)
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
    if(t != NULL)
    {
        pthread_getname_np(t->native_handle(), buf, sizeof(buf));
    }
#endif
    std::string name = buf;
    return name;
}

void com_thread_set_name(std::thread& t, const char* name)
{
    if(name == NULL)
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

void com_thread_set_name(uint64 tid_posix, const char* name)
{
#if __linux__ == 1
    if(tid_posix > 0 && name != NULL)
    {
        char tmp[16];
        strncpy(tmp, name, sizeof(tmp));
        tmp[sizeof(tmp) - 1] = '\0';
        pthread_setname_np(tid_posix, tmp);
    }
#endif
}

std::string com_thread_get_name(uint64 tid_posix)
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
    if(!::IsDebuggerPresent())
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
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {
    }
#elif defined(__APPLE__)
    pthread_setname_np(name);
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
        mov[nameCache], eax    //    nameCache = pTIB->pvArbitary
    }
    if(nameCache == NULL)
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
    thread_mgr_running = false;
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

void ThreadPool::ThreadLoop(ThreadPool* poll)
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
        if(poll->msgs.empty())
        {
            poll->mutex_msgs.unlock();
            continue;
        }
        Message msg = std::move(poll->msgs.front());
        poll->msgs.pop();
        poll->mutex_msgs.unlock();
        poll->threadPoolRunner(msg);
    }

    //本线程退出，设置标记为退出
    poll->mutex_threads.lock();
    if(poll->threads.count(tid) > 0)
    {
        THREAD_POLL_INFO& des = poll->threads[tid];
        des.running_flag = -1;
    }
    poll->mutex_threads.unlock();

    LOG_D("thread pool quit[%llu]", com_thread_get_tid());
    return;
}

void ThreadPool::startManagerThread()
{
    thread_mgr_running = true;
    thread_mgr = std::thread(ThreadManager, this);
}

void ThreadPool::createThread(int count)
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

void ThreadPool::recycleThread()
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

ThreadPool& ThreadPool::setThreadsCount(int min_threads, int max_threads)
{
    if(min_threads > 0 && max_threads > 0 && min_threads <= max_threads)
    {
        this->min_thread_count = min_threads;
        this->max_thread_count = max_threads;
    }
    return *this;
}

ThreadPool& ThreadPool::setQueueSize(int queue_size_per_thread)
{
    if(queue_size_per_thread > 0)
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

int ThreadPool::getPauseCount()
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

int ThreadPool::getMessageCount()
{
    int count = 0;
    mutex_msgs.lock();
    count = (int)msgs.size();
    mutex_msgs.unlock();
    return count;
}

void ThreadPool::waitAllDone(int timeout_ms)
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

void ThreadPool::startThreadPool()
{
    startManagerThread();
}

void ThreadPool::stopThreadPool()
{
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
        if(all_finished)
        {
            break;
        }
        else
        {
            com_sleep_ms(100);
        }
    }
    while(true);

    mutex_threads.lock();
    std::map<std::thread::id, THREAD_POLL_INFO>::iterator it;
    for(it = threads.begin(); it != threads.end(); it++)
    {
        THREAD_POLL_INFO& des = it->second;
        if(des.t != NULL && des.t->joinable())
        {
            des.t->join();
            delete des.t;
        }
    }
    threads.clear();
    mutex_threads.unlock();
    if(thread_mgr.joinable())
    {
        thread_mgr.join();
    }
}

bool ThreadPool::pushPoolMessage(Message&& msg)
{
    if(thread_mgr_running == false)
    {
        LOG_W("thread pool not running");
        return false;
    }
    mutex_msgs.lock();
    int count = max_thread_count * 10000;
    if(count < 1000)
    {
        count = 1000;
    }
    if((int)msgs.size() > count)
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

bool ThreadPool::pushPoolMessage(const Message& msg)
{
    if(thread_mgr_running == false)
    {
        LOG_W("thread pool not running");
        return false;
    }
    mutex_msgs.lock();
    int count = max_thread_count * 10000;
    if(count < 1000)
    {
        count = 1000;
    }
    if((int)msgs.size() > count)
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

