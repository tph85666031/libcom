#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#ifdef __GLIBC__
#include <signal.h>
#include <execinfo.h>
#include <sys/signalfd.h>
#endif
#include "com_base.h"
#include "com_stack.h"
#include "com_thread.h"
#include "com_file.h"
#include "com_log.h"
#include "com_config.h"

#define FILE_ADDR2LINE  "addr2line"
#define FILE_OBJDUMP    "objdump"

static std::atomic<void*> signal_cb_user_data;
static std::atomic<signal_cb> signal_cb_func;

static bool com_stack_name_of_pid(uint64 pid, char* name, int name_size)
{
    if(name == NULL || name_size <= 0)
    {
        return false;
    }
#if __linux__ == 1
    memset(name, 0, name_size);
    char proc_pid_path[128];
    char buf[128];
    sprintf(proc_pid_path, "/proc/%llu/status", pid);
    FILE* fp = fopen(proc_pid_path, "r");
    if(NULL != fp)
    {
        if(fgets(buf, sizeof(buf), fp) == NULL)
        {
            fclose(fp);
            return false;
        }
        fclose(fp);
        sscanf(buf, "%*s %s", name);
        return true;
    }
#endif
    return false;
}

static std::string stack_addr2line(const char* info, const char* file_addr2line, const char* file_objdump)
{
    if(info == NULL)
    {
        return std::string();
    }

    if(file_addr2line == NULL)
    {
        file_addr2line = FILE_ADDR2LINE;
    }

    if(file_objdump == NULL)
    {
        file_objdump = FILE_OBJDUMP;
    }

    std::string file;
    std::string function;
    std::string offset;
    std::string offset_absolute;
    std::string val = info;

    std::string::size_type pos_file = val.find_first_of("(");
    if(pos_file == std::string::npos)
    {
        return std::string();
    }

    file = val.substr(0, pos_file);
    std::string::size_type pos_function = val.find_first_of("+", pos_file + 1);
    if(pos_function != std::string::npos)
    {
        function = val.substr(pos_file + 1, pos_function - pos_file - 1);
        std::string::size_type pos_offset = val.find_first_of(")", pos_function + 1);
        if(pos_offset != std::string::npos)
        {
            offset = val.substr(pos_function + 1, pos_offset - pos_function - 1);
        }
    }

    std::string::size_type pos_offset_absolute_begin = val.find_first_of("[", pos_function + 1);
    std::string::size_type pos_offset_absolute_end = val.find_first_of("]", pos_function + 1);
    if(pos_offset_absolute_begin != std::string::npos && pos_offset_absolute_end != std::string::npos)
    {
        offset_absolute = val.substr(pos_offset_absolute_begin + 1, pos_offset_absolute_end - pos_offset_absolute_begin - 1);
    }

    com_string_trim(file);
    com_string_trim(function);
    com_string_trim(offset);
    com_string_trim(offset_absolute);

    uint64 addr_base = 0;
    std::string result;
    if(function.empty() == false)
    {
        result = com_run_shell_with_output("%s -t \"%s\" | grep %s 2>/dev/null",
                                           FILE_OBJDUMP, file.c_str(), function.c_str());
        if(result.empty())
        {
            return std::string();
        }
        std::vector<std::string> addrs = com_string_split(result.c_str(), " ");
        if(addrs.size() == 0)
        {
            return std::string();
        }
        addr_base = strtoull(addrs[0].c_str(), NULL, 16);
    }

    uint64 addr_offset_absolute = strtoull(offset_absolute.c_str(), NULL, 16);
    uint64 addr_offset = strtoull(offset.c_str(), NULL, 16);
    uint64 addr = addr_base + addr_offset;
    if(addr == 0)
    {
        if(addr_offset_absolute == 0)
        {
            return std::string();
        }
        addr = addr_offset_absolute;
    }

    result = com_run_shell_with_output("%s -e %s 0x%llx 2>/dev/null",
                                       FILE_ADDR2LINE, file.c_str(), addr);
    if(result[0] == '?')
    {
        return std::string();
    }
    com_string_replace(result, "\n", "");

    return result;
}

static std::string search_file_addr2line()
{
    std::string path ;

    path = com_string_format("./%s", FILE_ADDR2LINE);
    if(com_file_type(path.c_str()) == FILE_TYPE_FILE)
    {
        return path;
    }

    path = com_string_format("%s/%s", com_get_bin_path().c_str(), FILE_ADDR2LINE);
    if(com_file_type(path.c_str()) == FILE_TYPE_FILE)
    {
        return path;
    }

    path = com_string_format("/usr/bin/%s", FILE_ADDR2LINE);
    if(com_file_type(path.c_str()) == FILE_TYPE_FILE)
    {
        return path;
    }
    return std::string();
}

static std::string search_file_objdump()
{
    std::string path ;

    path = com_string_format("./%s", FILE_OBJDUMP);
    if(com_file_type(path.c_str()) == FILE_TYPE_FILE)
    {
        return path;
    }

    path = com_string_format("%s/%s", com_get_bin_path().c_str(), FILE_OBJDUMP);
    if(com_file_type(path.c_str()) == FILE_TYPE_FILE)
    {
        return path;
    }

    path = com_string_format("/usr/bin/%s", FILE_OBJDUMP);
    if(com_file_type(path.c_str()) == FILE_TYPE_FILE)
    {
        return path;
    }
    return std::string();
}

#ifdef __GLIBC__
static void stack_signal_function(int sig, siginfo_t* info, void* context)
{
    if(signal_cb_func != NULL)
    {
        if((*signal_cb_func)(sig, signal_cb_user_data))
        {
            return;
        }
    }

    char pid_name[128];
    com_stack_name_of_pid(info->si_pid, pid_name, sizeof(pid_name));
    LOG_W("%s:%d called\nsig=%d:%s, from=%s[%u], my pid=%llu",
          __FUNC__, __LINE__, sig, strsignal(sig), pid_name, info->si_pid, com_thread_get_pid());

    com_stack_print();

    //默认动作
    switch(sig)
    {
        case SIGINT:
            LOG_W("SIGINT system will exist");
            exit(sig);
            break;
        case SIGABRT:
            LOG_E("SIGABRT system will exist");
            exit(sig);
            break;
        case SIGSEGV:
            LOG_E("SIGSEGV system will exist");
            exit(sig);
            break;
        case SIGTERM:
            LOG_I("SIGTERM system will exist");
            exit(sig);
            break;
        case SIGTTIN:
            LOG_I("SIGTTIN system will exist");
            exit(sig);
            break;
        case SIGUSR1:
            LOG_I("SIGUSR1");
            break;
        case SIGUSR2:
            LOG_I("SIGUSR2");
            break;
        default:
            LOG_W("UNKNOWN signal:%d", sig);
            break;
    }
}
#endif

void com_stack_init()
{
#if __linux__ == 1
    signal_cb_func = NULL;
    signal_cb_user_data = NULL;

    std::vector<int> signal_value_ignore;
    std::vector<int> signal_value_none_reset;
    std::vector<int> signal_value_reset;

    signal_value_ignore.push_back(SIGPIPE);
    signal_value_reset.push_back(SIGINT);
    signal_value_reset.push_back(SIGABRT);
    signal_value_reset.push_back(SIGSEGV);
    signal_value_reset.push_back(SIGBUS);
    signal_value_reset.push_back(SIGSYS);
    signal_value_none_reset.push_back(SIGTERM);
    signal_value_none_reset.push_back(SIGUSR1);
    signal_value_none_reset.push_back(SIGUSR2);

    for(size_t i = 0; i < signal_value_ignore.size(); i++)
    {
        signal(signal_value_ignore[i], SIG_IGN);
    }

    struct sigaction sig_action;
    sigset_t sig_pocess_mask;
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_sigaction = stack_signal_function;
    sigemptyset(&sig_action.sa_mask);
    sigemptyset(&sig_pocess_mask);
    sig_action.sa_flags = SA_NODEFER | SA_SIGINFO;
    for(size_t i = 0; i < signal_value_none_reset.size(); i++)
    {
        sigaction(signal_value_none_reset[i], &sig_action, NULL);
        sigaddset(&sig_pocess_mask, signal_value_none_reset[i]);
    }

    sig_action.sa_flags |= SA_RESETHAND;   // 信号产生后重置该信号处理函数为系统默认处理
    for(size_t i = 0; i < signal_value_reset.size(); i++)
    {
        sigaction(signal_value_reset[i], &sig_action, NULL);
        sigaddset(&sig_pocess_mask, signal_value_reset[i]);
    }
    sigprocmask(SIG_SETMASK, &sig_pocess_mask, NULL);
    sigprocmask(SIG_UNBLOCK, &sig_pocess_mask, NULL);
#endif
}

void com_stack_uninit()
{
#if __linux__ == 1
    for(int i = 0; i <= SIGRTMAX; i++)
    {
        signal(i, SIG_DFL);
    }
#endif
    signal_cb_func = NULL;
}

void com_stack_print()
{
#ifdef __GLIBC__
    void* stack_addr[32];
    int nstack;

    memset(stack_addr, 0, sizeof(stack_addr));
    nstack = backtrace(stack_addr, 32);
    std::string file_addr2line = search_file_addr2line();
    std::string file_objdump = search_file_objdump();
    std::string buf;
    buf.append("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    buf.append(com_string_format("  Print Stack For Thread: %s\n", com_thread_get_name().c_str()));
    char** callstack = backtrace_symbols(stack_addr, nstack);
    if(callstack != NULL)
    {
        for(int j = 0; j < nstack; j++)
        {
            buf.append(com_string_format("  ThreadID:%llu IDX:%d : %s", com_thread_get_tid(), j, callstack[j]));
            if(file_addr2line.empty() == false && file_objdump.empty() == false)
            {
                std::string address_info = stack_addr2line(callstack[j], file_addr2line.c_str(), file_objdump.c_str());
                address_info = com_path_name(address_info.c_str());
                if(address_info.empty() == false)
                {
                    buf.append(" <");
                    buf.append(address_info);
                    buf.append(">");
                }
            }
            buf.append("\n");
        }
        free(callstack);
    }
    LOG_W("%s", buf.c_str());
#endif
}

void com_stack_set_hook(signal_cb cb, void* user_data)
{
    signal_cb_func = cb;
    signal_cb_user_data = user_data;
}

void com_system_send_signal(int sig)
{
#if __linux__ == 1
    raise(sig);
#endif
}

void com_system_send_signal(uint64 pid, int sig)
{
#if __linux__ == 1
    kill(pid, sig);
#endif
}

void com_system_exit(bool force)
{
#if __linux__ == 1
    com_system_send_signal(SIGTERM);
#else
    exit(0);
#endif
    if(force)
    {
        //信号会被程序截获，程序会执行清退工作
        com_sleep_s(5);//等待5秒尽可能保证程序完成清退工作
        exit(0);//强制退出
    }
}

