#ifndef __COM_STACK_H__
#define __COM_STACK_H__

#include <signal.h>
#include "com_base.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef SIGUSR1
#define SIGUSR1 191
#endif
#ifndef SIGUSR2
#define SIGUSR2 192
#endif
#endif

typedef bool (*signal_cb)(int sig, void* ctx);

COM_EXPORT void com_stack_init();
COM_EXPORT void com_stack_uninit();
COM_EXPORT std::string com_stack_get();
COM_EXPORT void com_stack_print();
COM_EXPORT void com_stack_set_hook(signal_cb cb, void* user_data = NULL);
COM_EXPORT void com_stack_enable_coredump();
COM_EXPORT void com_stack_disable_coredump();

COM_EXPORT void com_system_send_signal(int sig);
COM_EXPORT void com_system_send_signal(uint64 pid, int sig);
COM_EXPORT void com_system_exit(bool force = false);

#endif /* __COM_STACK_H__ */
