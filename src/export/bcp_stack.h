#ifndef __BCP_STACK_H__
#define __BCP_STACK_H__

#include "bcp_com.h"

typedef void (*signal_cb)(int signal, void* user_data);

void bcp_stack_init();
void bcp_stack_uninit();
void bcp_stack_print();
void bcp_stack_set_hook(signal_cb cb, void* user_data = NULL);

void bcp_system_send_signal(int sig);
void bcp_system_send_signal(uint64 pid, int sig);
void bcp_system_exit(bool force = false);

#endif /* __BCP_STACK_H__ */
