#include "com_md5.h"
#include "com_file.h"
#include "com_test.h"
#include "com_log.h"

void com_md5_unit_test_suit(void** state)
{
    CPPMD5 md5;
    md5.append((uint8*)"1234567890", sizeof("1234567890") - 1);
    md5.append((uint8*)"1234567890", sizeof("1234567890") - 1);
    std::string result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL("fd85e62d9beb45428771ec688418b271", result.c_str());

    com_file_writef("./md5_test.txt", "teset value");
    md5.appendFile("./md5_test.txt");
    result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL(CPPMD5::Digest("teset value", sizeof("teset value") - 1).toHexString().c_str(), result.c_str());

    md5.appendFile("./md5_test.txt", 0, 5);
    result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL(CPPMD5::Digest("teset", sizeof("teset") - 1).toHexString().c_str(), result.c_str());

    md5.appendFile("./md5_test.txt", 1, 4);
    result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL(CPPMD5::Digest("eset", sizeof("eset") - 1).toHexString().c_str(), result.c_str());

    md5.appendFile("./md5_test.txt", 6, 3);
    result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL(CPPMD5::Digest("val", sizeof("val") - 1).toHexString().c_str(), result.c_str());

    md5.appendFile("./md5_test.txt", 6, -1);
    result =  md5.finish().toHexString();
    ASSERT_STR_EQUAL(CPPMD5::Digest("value", sizeof("value") - 1).toHexString().c_str(), result.c_str());

    com_file_remove("./md5_test.txt");
    result = md5.finish().toHexString();
    ASSERT_STR_EQUAL("d41d8cd98f00b204e9800998ecf8427e", result.c_str());

    std::map<std::string, int> list;
    com_dir_list(".", list, true);

    for(auto it = list.begin(); it != list.end(); it++)
    {
        if(it->second == FILE_TYPE_UNKNOWN || it->second == FILE_TYPE_DIR)
        {
            continue;
        }
        std::string file = it->first;
        std::string md5_my = CPPMD5::Digest(file.c_str()).toHexString();
#if defined(_WIN32) || defined(_WIN64)
        std::string val = com_run_shell_with_output("md5sum.exe %s", file.c_str());
#else
        std::string val = com_run_shell_with_output("md5sum %s", file.c_str());
#endif
        //LOG_I("md5_my=%s,val=%s,file=%s", md5_my.c_str(), val.c_str(), file.c_str());
        ASSERT_TRUE(com_string_contain(val.c_str(), md5_my.c_str()));
    }
}
