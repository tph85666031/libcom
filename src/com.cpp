#include <mutex>
#include "bcp.h"

LIB_HEADER_EXPORT;

extern void InitTimerManager();
extern void InitMemDataSyncManager();

BcpInitializer::BcpInitializer()
{
    com_thread_set_name("T-Main");
    com_log_init();
    com_stack_init();
    InitTimerManager();
    InitMemDataSyncManager();
    LOG_D("done");
}

BcpInitializer::~BcpInitializer()
{
    LOG_D("called");
    com_stack_uninit();
    com_log_uninit();
}

void BcpInitializer::ManualInit()
{
#ifndef LIB_TYPE_SHARED
    static BcpInitializer bcpInitializer;
#endif
}

#ifndef LIB_TYPE_STATIC
BcpInitializer bcpInitializer;
#endif
