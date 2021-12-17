#include "com_auto_test.h"
#include "com.h"

void com_auto_test_unit_test_suit(void** state)
{
    GetAutoTestService().setLatestInfo("system", "gui", true);
    com_sleep_s(15);
}

