#include "com.h"
#include "com_auto_test.h"

void com_auto_test_unit_test_suit(void** state)
{
    InitAutoTestService();
    GetAutoTestService().setLatestInfo("system", "gui", true);
    com_sleep_s(120);
    UninitAutoTestService();
}

