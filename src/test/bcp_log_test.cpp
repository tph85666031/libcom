#include "bcp.h"

static void log_hook_test(int level, const char* buf)
{
    printf("level=%d,buf=%s\n", level, buf);
}

void bcp_log_unit_test_suit(void** state)
{
    bcp_thread_set_name("log unit test thread");
    LOG_I("log test 1");
    LOG_I("log test 11");
    LOG_I("log test 111");
    LOG_F("log test critical");
    LOG_D("log debug test 2");
    uint64 val = 1234567890;
    LOG_D("xxxx=%llu", val);

    bcp_log_set_hook(log_hook_test);
    LOG_D("log hook test");
}