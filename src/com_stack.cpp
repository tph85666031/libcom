#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#if __linux__ == 1
#include <execinfo.h>
#include <sys/signalfd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#endif
#include "com_base.h"
#include "com_stack.h"

void com_stack_enable_coredump()
{
#if __linux__ == 1
    struct rlimit rlim;
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rlim);
#endif
}

void com_stack_disable_coredump()
{
#if __linux__ == 1
    struct rlimit rlim;
    rlim.rlim_cur = 0;
    rlim.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rlim);
#endif
}

void com_system_signal_reset(int sig)
{
    signal(sig, SIG_DFL);
}

void com_system_signal_ignore(int sig)
{
    signal(sig, SIG_IGN);
}

void com_system_signal_send(int sig)
{
    raise(sig);
}

void com_system_signal_send(uint64 pid, int sig)
{
#if __linux__ == 1
    kill(pid, sig);
#endif
}

void com_system_exit(bool force)
{
    com_system_signal_send(SIGTERM);
    if(force)
    {
        //信号会被程序截获，程序会执行清退工作
        com_sleep_s(5);//等待5秒尽可能保证程序完成清退工作
        exit(0);//强制退出
    }
}

void com_system_pause()
{
#if defined(_WIN32) || defined(_WIN64)
    system("pause");
#else
    pause();
#endif
}

