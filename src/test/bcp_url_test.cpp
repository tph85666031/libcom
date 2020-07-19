#include "bcp.h"

void bcp_url_unit_test_suit(void** state)
{
    std::string result = URL::Encode("https://www.baidu.com?arg1=123+45&arg2=是不是56");
    ASSERT_STR_EQUAL("https%3A%2F%2Fwww.baidu.com%3Farg1%3D123%2B45%26arg2%3D%E6%98%AF%E4%B8%8D%E6%98%AF56", result.c_str());
}