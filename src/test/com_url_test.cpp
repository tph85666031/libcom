#include "com_log.h"
#include "com_url.h"
#include "com_test.h"

void com_url_unit_test_suit(void** state)
{
    std::string result = URL::Encode("https://www.baidu.com?arg1=123+45&arg2=是不是56");
    LOG_I("encode string=%s", result.c_str());
}
