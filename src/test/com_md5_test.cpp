#include "com.h"

void com_md5_unit_test_suit(void** state)
{
    CPPMD5 md5;
    md5.append((uint8*)"1234567890", sizeof("1234567890") - 1);
    md5.append((uint8*)"1234567890", sizeof("1234567890") - 1);
    std::string result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL("fd85e62d9beb45428771ec688418b271", result.c_str());

    md5.appendFile("./build");
    result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL("417dd98a69c29ea315fed56e1cf73146", result.c_str());

    result = md5.finish().toHexString();
    ASSERT_STR_EQUAL("d41d8cd98f00b204e9800998ecf8427e", result.c_str());
}
