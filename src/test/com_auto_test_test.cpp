#include "com_auto_test.h"
#include "com_test.h"

using namespace httplib;

void com_auto_test_unit_test_suit(void** state)
{
    GetAutoTestService().setLatestInfo("system", "protect", true);
    GetAutoTestService().setLatestInfo("gui", "ver", "1.0.1");
    com_sleep_s(5);
}

