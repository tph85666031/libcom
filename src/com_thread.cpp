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
#include <dirent.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <libproc.h>
#else
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>
union semun
{
    int              val;    /* Value for SETVAL */
    struct semid_ds* buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short*  array;  /* Array for GETALL, SETALL */
    struct seminfo*  __buf;  /* Buffer for IPC_INFO (Linux-specific) */
};
#endif

#include "com_thread.h"
#include "com_file.h"
#include "com_crc.h"
#include "com_log.h"

#if defined(_WIN32) || defined(_WIN64)
#define PROCESS_SEM_VALID(x)        (x!=NULL)
#define PROCESS_MUTEX_VALID(x)      (x!=NULL)
#define PROCESS_COND_VALID(x)       (x!=NULL)
#define PROCESS_MEM_VALID(x)        (x!=NULL)
#define PROCESS_SEM_DEFAULT_VALUE   (NULL)
#define PROCESS_MUTEX_DEFAULT_VALUE (NULL)
#define PROCESS_COND_DEFAULT_VALUE  (NULL)
#define PROCESS_MEM_DEFAULT_VALUE   (NULL)
#else
#define PROCESS_SEM_VALID(x)        (x>=0)
#define PROCESS_MUTEX_VALID(x)      (x>=0)
#define PROCESS_COND_VALID(x)       (x>=0)
#define PROCESS_MEM_VALID(x)        (x>=0)
#define PROCESS_SEM_DEFAULT_VALUE   (-1)
#define PROCESS_MUTEX_DEFAULT_VALUE (-1)
#define PROCESS_COND_DEFAULT_VALUE  (-1)
#define PROCESS_MEM_DEFAULT_VALUE   (-1)
#endif

ProcInfo::ProcInfo()
{
    valid = false;
    stat = -1;
    pid = -1;
    gid = -1;
    ppid = -1;
    rpid = -1;//valid for MacOS only
    session_id = -1;
    thread_count = 0;
}

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
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    std::string val = app;
    for(size_t i = 0; i < args.size(); i++)
    {
        val += " " + args[i];
    }

    std::wstring val_w = com_wstring_from_utf8(ComBytes(val));
    std::vector<wchar_t> cmd;
    cmd.assign(val_w.begin(), val_w.end());
    cmd.push_back(L'\0');
    if(CreateProcessW(NULL, &cmd[0], NULL, NULL, false, 0, NULL, NULL, &si, &pi) == false)
    {
        return -1;
    }
    return pi.dwProcessId;
#else
    //屏蔽SIGCHLD信号防止出现僵尸进程
    sigset_t sig_pocess_mask;
    struct sigaction sig_action_old;
    sigset_t sig_pocess_mask_old;
    memset(&sig_action_old, 0, sizeof(sig_action_old));
    sigaction(SIGCHLD, NULL, &sig_action_old);
    sigemptyset(&sig_pocess_mask);
    sigaddset(&sig_pocess_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sig_pocess_mask, &sig_pocess_mask_old);
    signal(SIGCHLD, SIG_IGN);
    int ret = fork();
    if(ret != 0)
    {
        sigaction(SIGCHLD, &sig_action_old, NULL);
        sigprocmask(SIG_SETMASK, &sig_pocess_mask_old, NULL);
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
    PROCESSENTRY32W pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    int pid = -1;
    for(bool ret = Process32FirstW(handle_snapshot, &pe); ret; ret = Process32NextW(handle_snapshot, &pe))
    {
        std::string name_cur = com_wstring_to_utf8(pe.szExeFile).toString();
        if(com_string_match(name_cur.c_str(), name))
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
    std::vector<char> name_cur(4096);
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
        int pid = -1;
        int ppid = -1;
        int gid = -1;
        if(sscanf(content.c_str(), "%d %*s %*s %d %d %*d", &pid, &ppid, &gid) != 3)
        {
            continue;
        }

        int ret = readlink(com_string_format("%s/exe", it->first.c_str()).c_str(), &name_cur[0], name_cur.size() - 1);
        if(ret <= 0)
        {
            continue;
        }
        name_cur[ret] = '\0';
        if(com_string_match(com_path_name(&name_cur[0]).c_str(), name))
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
    PROCESSENTRY32W pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    int pid = 0;
    for(bool ret = Process32FirstW(handle_snapshot, &pe); ret; ret = Process32NextW(handle_snapshot, &pe))
    {
        std::string name_cur = com_wstring_to_utf8(pe.szExeFile).toString();
        if(com_string_match(name_cur.c_str(), name))
        {
            pids.push_back(pe.th32ProcessID);
        }
    }
    CloseHandle(handle_snapshot);
#else
    std::map<std::string, int> list;
    com_dir_list("/proc", list);
    std::vector<char> name_cur(4096);
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
        int pid = -1;
        int ppid = -1;
        int gid = -1;
        if(sscanf(content.c_str(), "%d %*s %*s %d %d %*d", &pid, &ppid, &gid) != 3)
        {
            continue;
        }
        int ret = readlink(com_string_format("%s/exe", it->first.c_str()).c_str(), &name_cur[0], name_cur.size() - 1);
        if(ret <= 0)
        {
            continue;
        }
        name_cur[ret] = '\0';
        if(com_string_match(com_path_name(&name_cur[0]).c_str(), name))
        {
            pids.push_back(pid);
        }
    }
#endif
    return pids;
}

int com_process_get_ppid(int pid)
{
#if defined(_WIN32) || defined(_WIN64)
    if(pid < 0)
    {
        pid = com_process_get_pid();
    }
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, pid);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        LOG_E("failed");
        return -1;
    }
    PROCESSENTRY32W pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    int ppid = -1;
    for(bool ret = Process32FirstW(handle_snapshot, &pe); ret; ret = Process32NextW(handle_snapshot, &pe))
    {
        if(pe.th32ProcessID == pid)
        {
            ppid = pe.th32ParentProcessID;
            break;
        }
    }
    CloseHandle(handle_snapshot);
    return ppid;
#elif defined(__APPLE__) && defined(__MACH__)
    if(pid <= 0)
    {
        return getppid();
    }
    else
    {
        struct proc_taskallinfo info;
        int ret = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &info, sizeof(info));
        if(ret > 0)
        {
            return info.pbsd.pbi_ppid;
        }
    }
    return -1;
#else
    if(pid < 0)
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

ProcInfo com_process_get(int pid)
{
    ProcInfo info;
    if(pid < 0)
    {
        return info;
    }
#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, pid);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        return info;
    }
    PROCESSENTRY32W pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    bool ret = Process32FirstW(handle_snapshot, &pe);
    if(ret == false)
    {
        LOG_E("failed, error code is:%d", GetLastError());
        return info;
    }
    for(; ret; ret = Process32NextW(handle_snapshot, &pe))
    {
        if(pe.th32ProcessID == pid)
        {
            info.pid = pid;
            info.ppid = pe.th32ParentProcessID;
            info.thread_count = pe.cntThreads;
            DWORD session_id = 0;
            info.valid = ProcessIdToSessionId(pe.th32ProcessID, &session_id);
            info.session_id = (int)session_id;
            info.path = com_wstring_to_utf8(pe.szExeFile).toString();
            break;
        }
    }
    CloseHandle(handle_snapshot);
    return info;
#else
    std::string content = com_file_readall(com_string_format("/proc/%d/stat", pid).c_str()).toString();
    if(content.empty())
    {
        return info;
    }
    char proc_status = ' ';
    if(sscanf(content.c_str(),
              "%d %*s %c %d %d %d %*d %*d"\
              "%*u %*u %*u %*u %*u"\
              "%*u %*u %*u %*u"\
              "%*d %*d"\
              "%d",
              &info.pid, &proc_status, &info.ppid, &info.pgrp, &info.session_id, &info.thread_count) == 6)
    {
        switch(proc_status)
        {
            case 'I':
                info.stat = PROC_STAT_IDEL;
                break;
            case 'R':
                info.stat = PROC_STAT_RUNNING;
                break;
            case 'S':
                info.stat = PROC_STAT_SLEEP_INTERRUPTIBLE;
                break;
            case 'D':
                info.stat = PROC_STAT_SLEEP_UNINTERRUPTIBLE;
                break;
            case 'X':
            case 'x':
                info.stat = PROC_STAT_DEAD;
                break;
            case 'Z':
                info.stat = PROC_STAT_ZOMBIE;
                break;
            case 'T':
            case 't':
                info.stat = PROC_STAT_TRACED;
                break;
            case 'P':
                info.stat = PROC_STAT_PARKED;
                break;
        }
        info.valid = true;
        info.path = com_process_get_path(info.pid);
        content = com_file_readall(com_string_format("/proc/%d/status", pid).c_str()).toString();
        ByteStreamReader reader(content);
        std::string line;
        while(reader.readLine(line))
        {
            if(com_string_start_with(line.c_str(), "Uid:"))
            {
                sscanf(line.c_str(), "%*s %d %d %d %d", &info.uid, &info.euid, &info.suid, &info.fsuid);
            }
            else if(com_string_start_with(line.c_str(), "Gid:"))
            {
                sscanf(line.c_str(), "%*s %d %d %d %d", &info.gid, &info.egid, &info.sgid, &info.fsgid);
            }
        }
    }
    return info;
#endif
}

std::map<int, ProcInfo> com_process_get_all()
{
    std::map<int, ProcInfo> infos;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        return infos;
    }
    PROCESSENTRY32W pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    for(bool ret = Process32FirstW(handle_snapshot, &pe); ret; ret = Process32NextW(handle_snapshot, &pe))
    {
        ProcInfo info;
        info.pid = pe.th32ProcessID;
        info.ppid = pe.th32ParentProcessID;
        info.thread_count = pe.cntThreads;
        DWORD session_id = 0;
        info.valid = ProcessIdToSessionId(pe.th32ParentProcessID, &session_id);
        info.session_id = (int)session_id;
        info.path = com_wstring_to_utf8(pe.szExeFile).toString();
        infos[pe.th32ProcessID] = info;
    }
    CloseHandle(handle_snapshot);
    return infos;
#else
    std::vector<int> list_pids;
    DIR* dir = opendir("/proc/");
    if(dir == NULL)
    {
        return infos;
    }
    struct dirent* ptr = NULL;
    while((ptr = readdir(dir)) != NULL)
    {
#if __ANDROID__ != 1
        if(ptr->d_name == NULL)
        {
            continue;
        }
#endif
        if(ptr->d_type != DT_DIR)
        {
            continue;
        }
        list_pids.push_back(strtol(ptr->d_name, NULL, 10));
    }
    closedir(dir);

    for(size_t i = 0; i < list_pids.size(); i++)
    {
        ProcInfo info = com_process_get(list_pids[i]);
        if(info.valid)
        {
            infos[info.pid] = info;
        }
    }
    return infos;
#endif
}

ProcInfo com_process_get_parent(int pid)
{
    return com_process_get(com_process_get_ppid(pid));
}

std::vector<ProcInfo> com_process_get_parent_all(int pid)
{
    int pid_cur = com_process_get_ppid(pid);
    std::vector<ProcInfo> result;
    std::map<int, ProcInfo> infos = com_process_get_all();
    while(pid_cur > 0)
    {
        auto it = infos.find(pid_cur);
        if(it == infos.end())
        {
            break;
        }
        result.push_back(it->second);
        if(it->second.ppid == pid_cur)
        {
            break;
        }
        pid_cur = it->second.ppid;
    }
    return result;
}

std::vector<ProcInfo> com_process_get_child(int pid)
{
    std::vector<ProcInfo> result;
    std::map<int, ProcInfo> infos = com_process_get_all();
    for(auto it = infos.begin(); it != infos.end(); it++)
    {
        if(it->second.ppid == pid)
        {
            result.push_back(it->second);
        }
    }
    return result;
}

std::vector<ProcInfo> com_process_get_child_all(int pid)
{
    std::vector<ProcInfo> result;
    std::queue<int> vals;
    std::map<int, ProcInfo> proc_all = com_process_get_all();
    do
    {
        for(auto it = proc_all.begin(); it != proc_all.end(); it++)
        {
            if(it->second.ppid == pid)
            {
                vals.push(it->second.pid);
                result.push_back(it->second);
            }
        }
        if(vals.empty())
        {
            break;
        }
        pid = vals.front();
        vals.pop();
    }
    while(true);
    return result;
}

std::string com_process_get_path(int pid)
{
    if(pid < 0)
    {
        pid = com_process_get_pid();
    }
#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPALL, pid);
    if(INVALID_HANDLE_VALUE == handle_snapshot)
    {
        LOG_E("failed");
        return std::string();
    }
    PROCESSENTRY32W pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    for(bool ret = Process32FirstW(handle_snapshot, &pe); ret; ret = Process32NextW(handle_snapshot, &pe))
    {
        if(pe.th32ProcessID == pid)
        {
            CloseHandle(handle_snapshot);
            return com_wstring_to_utf8(pe.szExeFile).toString();
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
    return std::string(&app[0], ret);
#endif
}

std::string com_process_get_name(int pid)
{
    return com_path_name(com_process_get_path(pid).c_str());
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

std::vector<uint64> com_thread_get_tid_all(int pid)
{
#if defined(_WIN32) || defined(_WIN64)
    return std::vector<uint64>();
#elif __linux__ == 1
    if(pid < 0)
    {
        pid = com_process_get_pid();
    }
    std::string path = com_string_format("/proc/%d/task", pid);
    std::map<std::string, int> list;
    com_dir_list(path.c_str(), list);
    std::vector<uint64> tids;
    tids.push_back(pid);
    for(auto it = list.begin(); it != list.end(); it++)
    {
        if(it->second != FILE_TYPE_DIR)
        {
            continue;
        }
        std::string name = com_path_name(it->first.c_str());
        if(name.empty())
        {
            continue;
        }
        uint64 tid = strtoul(name.c_str(), NULL, 10);
        if(tid != (uint64)pid)
        {
            tids.push_back(tid);
        }
    }
    return tids;
#else
    return std::vector<uint64>();
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

ComThreadPool::ComThreadPool()
{
    thread_mgr_running = false;
    allow_duplicate_message = false;
    min_thread_count = 2;
    max_thread_count = 10;
    queue_size_per_thread = 5;
}

ComThreadPool::~ComThreadPool()
{
    stopThreadPool();
}

void ComThreadPool::ThreadManager(ComThreadPool* poll)
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

void ComThreadPool::ThreadLoop(ComThreadPool* poll)
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
            Message msg(std::move(poll->msgs.front()));
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

void ComThreadPool::startManagerThread()
{
    thread_mgr_running = true;
    thread_mgr = std::thread(ThreadManager, this);
}

void ComThreadPool::createThread(int count)
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

void ComThreadPool::recycleThread()
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

ComThreadPool& ComThreadPool::setThreadsCount(int min_threads, int max_threads)
{
    if(min_threads > 0 && max_threads > 0 && min_threads <= max_threads)
    {
        this->min_thread_count = min_threads;
        this->max_thread_count = max_threads;
    }
    return *this;
}

ComThreadPool& ComThreadPool::setQueueSize(int queue_size_per_thread)
{
    if(queue_size_per_thread > 0)
    {
        this->queue_size_per_thread = queue_size_per_thread;
    }
    return *this;
}

ComThreadPool& ComThreadPool::setAllowDuplicateMessage(bool allow)
{
    this->allow_duplicate_message = allow;
    return *this;
}

int ComThreadPool::getRunningCount()
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

int ComThreadPool::getPauseCount()
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

int ComThreadPool::getMessageCount()
{
    int count = 0;
    mutex_msgs.lock();
    count = (int)msgs.size();
    mutex_msgs.unlock();
    return count;
}

void ComThreadPool::waitAllDone(int timeout_ms)
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

void ComThreadPool::startThreadPool()
{
    startManagerThread();
}

void ComThreadPool::stopThreadPool(bool force)
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

bool ComThreadPool::pushPoolMessage(Message&& msg, bool urgent)
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
    condition.notifyOne();
    return true;
}

bool ComThreadPool::pushPoolMessage(const Message& msg, bool urgent)
{
    if(thread_mgr_running == false)
    {
        LOG_W("thread pool not running");
        return false;
    }
    mutex_msgs.lock();
    msgs.push_back(msg);
    mutex_msgs.unlock();
    condition.notifyOne();
    return true;
}

ComProcessSem::ComProcessSem()
{
    sem = PROCESS_SEM_DEFAULT_VALUE;
}

ComProcessSem::ComProcessSem(const char* name, int init_val)
{
    sem = PROCESS_SEM_DEFAULT_VALUE;
    init(name, init_val);
}

ComProcessSem::~ComProcessSem()
{
    uninit();
}

bool ComProcessSem::init(const char* name, int init_val)
{
    if(name == NULL)
    {
        return false;
    }
#if __linux__ == 1 || defined(__APPLE__)
    key_t key = ftok(name, 0);
    if(key == -1)
    {
        key = com_crc32((uint8*)name, strlen(name)) & 0x7FFFFFFF;
        LOG_D("failed to create key, use crc32 value instead,key=0x%x", key);
    }
    int flag = 0644;
    sem = semget(key, 1, flag);
    if(sem >= 0)
    {
        LOG_I("%s already created", name);
        return true;
    }
    flag |= IPC_CREAT;
    sem = semget(key, 1, flag);
    if(init_val > 0)
    {
        union semun arg;
        arg.val = init_val;
        semctl(sem, 0, SETVAL, arg);
    }
    return (sem >= 0);
#elif defined(_WIN32) || defined(_WIN64)
    std::wstring str_name = com_wstring_from_utf8(ComBytes(name));
    process_sem_t handle = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, true, str_name.c_str());
    if(handle == NULL)
    {
        handle = CreateSemaphoreW(NULL, init_val, 1, str_name.c_str());
    }
    sem = handle;
    return (sem != NULL);
#endif
}

void ComProcessSem::uninit(bool destroy)
{
    if(PROCESS_SEM_VALID(sem) == false)
    {
        return;
    }
#if __linux__ == 1 || defined(__APPLE__)
    if(destroy)
    {
        semctl(sem, 0, IPC_RMID);
    }
#elif defined(_WIN32) || defined(_WIN64)
    CloseHandle(sem);
#endif
    sem = PROCESS_SEM_DEFAULT_VALUE;
}

int ComProcessSem::post()
{
    if(PROCESS_SEM_VALID(sem) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    int ret = semop(sem, &buf, 1);
    if(ret == 0)
    {
        return 0;
    }
    else if(errno == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(ReleaseSemaphore(sem, 1, NULL) == 0)
    {
        return -1;
    }
    return 0;
#endif
}

int ComProcessSem::wait(int timeout_ms)
{
    if(PROCESS_SEM_VALID(sem) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;

    int ret = 0;
    int err_no = 0;
    if(timeout_ms <= 0)
    {
        ret = semop(sem, &buf, 1);
        err_no = errno;
    }
    else
    {
#if __linux__ == 1
        //semtimedop使用相对时间
        struct timespec ts;
        memset(&ts, 0, sizeof(struct timespec));
        ts.tv_nsec = ((int64)timeout_ms % 1000) * 1000 * 1000;
        ts.tv_sec = timeout_ms / 1000;
        ret = semtimedop(sem, &buf, 1, &ts);
        err_no = errno;
#elif defined(__APPLE__)
        int wait_ms = 100;
        buf.sem_flg |= IPC_NOWAIT;
        while(timeout_ms > 0)
        {
            ret = semop(sem, &buf, 1);
            err_no = errno;

            if(ret == 0 || err_no != EAGAIN)
            {
                break;
            }

            // 会修改errno 为 timeout
            com_sleep_ms(wait_ms);
            timeout_ms -= wait_ms;
        }
#endif
    }

    if(ret == 0)
    {
        return 0;
    }
    else if(err_no == EAGAIN)
    {
        return -2;
    }
    else if(err_no == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(timeout_ms <= 0)
    {
        timeout_ms = INFINITE;
    }
    int ret = WaitForSingleObject(sem, timeout_ms);
    if(ret == WAIT_OBJECT_0)
    {
        return 0;
    }
    else if(ret == WAIT_TIMEOUT)
    {
        return -2;
    }
    else
    {
        return GetLastError();
    }
#endif
}

ComProcessMutex::ComProcessMutex()
{
    mutex = PROCESS_MUTEX_DEFAULT_VALUE;
}

ComProcessMutex::ComProcessMutex(const char* name)
{
    mutex = PROCESS_MUTEX_DEFAULT_VALUE;
    init(name);
}

ComProcessMutex::~ComProcessMutex()
{
    uninit();
}

bool ComProcessMutex::init(const char* name)
{
    if(name == NULL)
    {
        return false;
    }
#if __linux__ == 1 || defined(__APPLE__)
    key_t key = ftok(name, 0);
    if(key == -1)
    {
        key = com_crc32((uint8*)name, strlen(name)) & 0x7FFFFFFF;
        LOG_D("failed to create key, use crc32 value instead,key=0x%x", key);
    }
    int flag = 0644;
    mutex = semget(key, 1, flag);
    if(mutex >= 0)
    {
        LOG_I("%s already created,key=0x%x,mutex=%d", name, key, mutex);
        return true;
    }
    flag |= IPC_CREAT;
    mutex = semget(key, 1, flag);
    union semun arg;
    arg.val = 1;
    semctl(mutex, 0, SETVAL, arg);
    return (mutex >= 0);
#elif defined(_WIN32) || defined(_WIN64)
    std::wstring str_name = com_wstring_from_utf8(ComBytes(name));
    process_mutex_t handle = OpenMutexW(MUTEX_ALL_ACCESS, true, str_name.c_str());
    if(handle == NULL)
    {
        handle = CreateMutexW(NULL, false, str_name.c_str());
    }
    mutex = handle;
    return (mutex != NULL);
#endif
}

void ComProcessMutex::uninit(bool destroy)
{
    if(PROCESS_MUTEX_VALID(mutex) == false)
    {
        return;
    }
#if __linux__ == 1 || defined(__APPLE__)
    if(destroy)
    {
        semctl(mutex, 0, IPC_RMID);
    }
#elif defined(_WIN32) || defined(_WIN64)
    CloseHandle(mutex);
#endif
    mutex = PROCESS_MUTEX_DEFAULT_VALUE;
}

void ComProcessMutex::reset(bool to_unlocked)
{
#if __linux__ == 1 || defined(__APPLE__)
    union semun arg;
    arg.val = to_unlocked ? 1 : 0;
    semctl(mutex, 0, SETVAL, arg);
#elif defined(_WIN32) || defined(_WIN64)
    LOG_I("not required by win");
#endif
}

int ComProcessMutex::lock()
{
    if(PROCESS_MUTEX_VALID(mutex) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;

    int ret = semop(mutex, &buf, 1);
    int err_no = errno;

    if(ret == 0)
    {
        return 0;
    }
    else if(err_no == EAGAIN)
    {
        return -2;
    }
    else if(err_no == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    int ret = WaitForSingleObject(mutex, INFINITE);
    if(ret == WAIT_OBJECT_0)
    {
        return 0;
    }
    else if(ret == WAIT_TIMEOUT)
    {
        LOG_E("failed,ret=%d", ret);
        return -2;
    }
    else
    {
        LOG_E("failed");
        return GetLastError();
    }
#endif
}

int ComProcessMutex::trylock()
{
    if(PROCESS_MUTEX_VALID(mutex) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO | IPC_NOWAIT;

    int ret = semop(mutex, &buf, 1);
    int err_no = errno;

    if(ret == 0)
    {
        return 0;
    }
    else if(err_no == EAGAIN)
    {
        return -2;
    }
    else if(err_no == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    int ret = WaitForSingleObject(mutex, 0);
    if(ret == WAIT_OBJECT_0)
    {
        return 0;
    }
    else if(ret == WAIT_TIMEOUT)
    {
        return -2;
    }
    else
    {
        return GetLastError();
    }
#endif
}

int ComProcessMutex::unlock()
{
    if(PROCESS_MUTEX_VALID(mutex) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    int ret = semop(mutex, &buf, 1);
    if(ret == 0)
    {
        return 0;
    }
    else if(errno == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(ReleaseMutex(mutex) == 0)
    {
        LOG_E("failed");
        return -1;
    }
    return 0;
#endif
}

ComProcessCondition::ComProcessCondition()
{
    cond = PROCESS_COND_DEFAULT_VALUE;
}

ComProcessCondition::ComProcessCondition(const char* name)
{
    cond = PROCESS_COND_DEFAULT_VALUE;
    init(name);
}

ComProcessCondition::~ComProcessCondition()
{
    uninit();
}

bool ComProcessCondition::init(const char* name)
{
    if(name == NULL)
    {
        return false;
    }
#if __linux__ == 1 || defined(__APPLE__)
    key_t key = ftok(name, 0);
    if(key == -1)
    {
        key = com_crc32((uint8*)name, strlen(name)) & 0x7FFFFFFF;
        LOG_D("failed to create key, use crc32 value instead,key=0x%x", key);
    }
    int flag = 0644;
    cond = semget(key, 1, flag);
    if(cond >= 0)
    {
        LOG_I("%s already created", name);
        return true;
    }
    flag |= IPC_CREAT;
    cond = semget(key, 1, flag);
    return (cond >= 0);
#elif defined(_WIN32) || defined(_WIN64)
    std::wstring str_name = com_wstring_from_utf8(ComBytes(name));
    process_cond_t handle = OpenEventW(EVENT_ALL_ACCESS, true, str_name.c_str());
    if(handle == NULL)
    {
        handle = CreateEventW(NULL, true, false, str_name.c_str());
    }
    cond = handle;
    return (cond != NULL);
#endif
}

void ComProcessCondition::uninit(bool destroy)
{
    if(PROCESS_COND_VALID(cond) == false)
    {
        return;
    }
#if __linux__ == 1 || defined(__APPLE__)
    if(destroy)
    {
        semctl(cond, 0, IPC_RMID);
    }
#elif defined(_WIN32) || defined(_WIN64)
    CloseHandle(cond);
#endif
    cond = PROCESS_COND_DEFAULT_VALUE;
}

int ComProcessCondition::wait(int timeout_ms)
{
    if(PROCESS_COND_VALID(cond) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;

    int ret = 0;
    int err_no = 0;
    if(timeout_ms <= 0)
    {
        ret = semop(cond, &buf, 1);
        err_no = errno;
    }
    else
    {
#if __linux__ == 1
        //semtimedop使用相对时间
        struct timespec ts;
        memset(&ts, 0, sizeof(struct timespec));
        ts.tv_nsec = ((int64)timeout_ms % 1000) * 1000 * 1000;
        ts.tv_sec = timeout_ms / 1000;
        ret = semtimedop(cond, &buf, 1, &ts);
        err_no = errno;
#elif defined(__APPLE__)
        int wait_ms = 100;
        buf.sem_flg |= IPC_NOWAIT;
        while(timeout_ms > 0)
        {
            ret = semop(cond, &buf, 1);
            err_no = errno;

            if(ret == 0 || err_no != EAGAIN)
            {
                break;
            }

            // 会修改errno 为 timeout
            com_sleep_ms(wait_ms);
            timeout_ms -= wait_ms;
        }
#endif
    }

    if(ret == 0)
    {
        return 0;
    }
    else if(err_no == EAGAIN)
    {
        return -2;
    }
    else if(err_no == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(timeout_ms <= 0)
    {
        timeout_ms = INFINITE;
    }
    int ret = WaitForSingleObject(cond, timeout_ms);
    ResetEvent(cond);
    if(ret == WAIT_OBJECT_0)
    {
        return 0;
    }
    else if(ret == WAIT_TIMEOUT)
    {
        return -2;
    }
    else
    {
        return GetLastError();
    }
#endif
}

int ComProcessCondition::notifyOne()
{
    if(PROCESS_COND_VALID(cond) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    int count = semctl(cond, 0, GETNCNT);
    if(count <= 0)
    {
        return 0;
    }
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    int ret = semop(cond, &buf, 1);
    if(ret == 0)
    {
        return 0;
    }
    else if(errno == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#else
    LOG_W("not support,will wake-up all instances, or use ComProcessSem instead");
    if(SetEvent(cond) == 0)
    {
        return -1;
    }
    return 0;
#endif
}

int ComProcessCondition::notifyAll()
{
    if(PROCESS_COND_VALID(cond) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    int count = semctl(cond, 0, GETNCNT);
    if(count <= 0)
    {
        return 0;
    }
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = count;
    buf.sem_flg = SEM_UNDO;
    int ret = semop(cond, &buf, 1);
    if(ret == 0)
    {
        return 0;
    }
    else if(errno == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(SetEvent(cond) == 0)
    {
        return -1;
    }
    return 0;
#endif
}

ComShareMemoryV::ComShareMemoryV()
{
    msg = NULL;
    mem = PROCESS_MEM_DEFAULT_VALUE;
}

ComShareMemoryV::ComShareMemoryV(const char* name, int size)
{
    msg = NULL;
    mem = PROCESS_MEM_DEFAULT_VALUE;
    init(name, size);
}

ComShareMemoryV::~ComShareMemoryV()
{
    uninit();
}

void* ComShareMemoryV::init(const char* name, int size)
{
    if(name == NULL || size <= 0)
    {
        return NULL;
    }
    uninit();
#if __linux__ == 1 || defined(__APPLE__)
    key_t key = ftok(name, 1);
    if(key == -1)
    {
        key = com_crc32((uint8*)name, strlen(name)) & 0x7FFFFFFF;
        LOG_W("failed to create key, use crc32 value instead:0x%x", key);
    }
    int flag = 0666 | IPC_CREAT;
#if defined(__APPLE__)
    unsigned long max_shm_size = 0;
    size_t len = sizeof(max_shm_size);
    if(sysctlbyname("kern.sysv.shmmax", &max_shm_size, &len, NULL, 0) == 0)
    {
        if(max_shm_size < size)
        {
            LOG_E("size is larger then kern.sysv.shmmax:%d>%u", size, max_shm_size);
            return NULL;
        }
    }
#endif
    mem = shmget(key, size, flag);
    msg = shmat(mem, NULL, 0);
#elif defined(_WIN32) || defined(_WIN64)
    mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, name);
    if(mem == NULL)
    {
        mem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name);
    }
    msg = MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
    return msg;
}

void ComShareMemoryV::uninit()
{
#if __linux__ == 1 || defined(__APPLE__)
    if(msg != NULL)
    {
        shmdt(msg);
    }
    if(PROCESS_MEM_VALID(mem))
    {
        shmctl(mem, IPC_RMID, NULL);
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(msg != NULL)
    {
        UnmapViewOfFile(msg);
    }
    if(PROCESS_MEM_VALID(mem))
    {
        CloseHandle(mem);
    }
#endif
    mem = PROCESS_MEM_DEFAULT_VALUE;
    msg = NULL;
}

void* ComShareMemoryV::getAddr()
{
    return msg;
}

ComShareMemoryMap::ComShareMemoryMap()
{
    msg = NULL;
    msg_size = 0;
    mem = PROCESS_MEM_DEFAULT_VALUE;
}

ComShareMemoryMap::ComShareMemoryMap(const char* name, int size)
{
    msg = NULL;
    msg_size = 0;
    mem = PROCESS_MEM_DEFAULT_VALUE;
    init(name, size);
}

ComShareMemoryMap::~ComShareMemoryMap()
{
    uninit();
}

void* ComShareMemoryMap::init(const char* file, int size)
{
    if(file == NULL || size <= 0)
    {
        return NULL;
    }
    uninit();
#if defined(_WIN32) || defined(_WIN64)
    mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, file);
    if(mem == NULL)
    {
        mem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, file);
    }
    msg = MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
    int fd = open(file, O_CREAT | O_RDWR, 0666);
    if(fd < 0)
    {
        return NULL;
    }
    ftruncate(fd, size);
    msg = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if(msg == MAP_FAILED)
    {
        return NULL;
    }
    msg_size = size;
#endif
    return msg;
}

void ComShareMemoryMap::uninit()
{
#if defined(_WIN32) || defined(_WIN64)
    if(msg != NULL)
    {
        UnmapViewOfFile(msg);
    }
    if(PROCESS_MEM_VALID(mem))
    {
        CloseHandle(mem);
    }
#else
    if(msg != NULL)
    {
        munmap(msg, msg_size);
    }
#endif
    mem = PROCESS_MEM_DEFAULT_VALUE;
    msg = NULL;
}

void* ComShareMemoryMap::getAddr()
{
    return msg;
}

