#include <mutex>
#include "com.h"

LIB_HEADER_EXPORT;

extern void InitTimerManager();
extern void InitMemDataSyncManager();

ComInitializer::ComInitializer()
{
    com_thread_set_name("T-Main");
    com_log_init();
    com_stack_init();
    InitTimerManager();
    InitMemDataSyncManager();
    LOG_D("done");
}

ComInitializer::~ComInitializer()
{
    LOG_D("called");
    com_stack_uninit();
    com_log_uninit();
}

void ComInitializer::ManualInit()
{
#ifndef LIB_TYPE_SHARED
    static ComInitializer comInitializer;
#endif
}

#ifndef LIB_TYPE_STATIC
ComInitializer comInitializer;
#endif
