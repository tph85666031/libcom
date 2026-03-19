#ifndef __COM_STACK_H__
#define __COM_STACK_H__

#include <signal.h>
#include "com_base.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef SIGPIPE
#define SIGPIPE -1
#endif
#ifndef SIGUSR1
#define SIGUSR1 191
#endif
#ifndef SIGUSR2
#define SIGUSR2 192
#endif
#endif

COM_EXPORT void com_stack_enable_coredump();
COM_EXPORT void com_stack_disable_coredump();

COM_EXPORT void com_system_signal_reset(int sig);
COM_EXPORT void com_system_signal_ignore(int sig);
COM_EXPORT void com_system_signal_send(int sig);
COM_EXPORT void com_system_signal_send(uint64 pid, int sig);
COM_EXPORT void com_system_exit(bool force = false);
COM_EXPORT void com_system_pause();

#endif /* __COM_STACK_H__ */
