#include <mutex>
#include "bcp.h"

LIB_HEADER_EXPORT;

extern void InitTimerManager();
extern void InitMemDataSyncManager();

BcpInitializer::BcpInitializer()
{
    bcp_thread_set_name("T-Main");
    bcp_log_init();
    bcp_stack_init();
    InitTimerManager();
    InitMemDataSyncManager();
    LOG_D("done");
}

BcpInitializer::~BcpInitializer()
{
    LOG_D("called");
    bcp_stack_uninit();
    bcp_log_uninit();
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
