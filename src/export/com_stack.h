#ifndef __COM_STACK_H__
#define __COM_STACK_H__

#include "com_base.h"

typedef void (*signal_cb)(int signal, void* user_data);

void com_stack_init();
void com_stack_uninit();
void com_stack_print();
void com_stack_set_hook(signal_cb cb, void* user_data = NULL);

void com_system_send_signal(int sig);
void com_system_send_signal(uint64 pid, int sig);
void com_system_exit(bool force = false);

#endif /* __COM_STACK_H__ */
