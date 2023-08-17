#include "com_log.h"
#include "com_file.h"
#include "com_thread.h"
#include "com_test.h"

void com_log_unit_test_suit(void** state)
{
    com_thread_set_name("log unit test thread");
    LOG_I("log test 1");
    LOG_I("log test 11");
    LOG_I("log test 111");
    LOG_F("log test critical");
    LOG_D("log debug test 2");
    uint64 val = 1234567890;
    LOG_D("xxxx=%llu", val);

    TIME_COST();
    if(com_file_exist("/data/0.txt") == false)
    {
        com_file_copy("/data/0.txt", "/data/1.txt");
    }
    TIME_COST_SHOW();

    com_logger_create("l1", "./1.txt");
    com_logger_log("l1", LOG_LEVEL_ERROR, "this logger error");
    com_logger_destroy("l1");
}

