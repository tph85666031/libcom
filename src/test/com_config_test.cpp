#include "com.h"

void com_config_unit_test_suit(void** state)
{
    com_file_remove("1.ini");
    com_file_remove("/tmp/.1.ini");
    com_file_remove("/tmp/.1.ini.bak");
    FILE* file =  com_file_open("1.ini", "w+");
    ASSERT_NOT_NULL(file);
    com_file_writef(file, "id=%d\n", 0);
    com_file_writef(file, "name=%s\n", "000");
    com_file_writef(file, "float=%.6f\n", 0.632473f);
    com_file_writef(file, "double=%.15lf\n", 0.632193878912649f);
    com_file_writef(file, "int=%d\n", -100);
    com_file_writef(file, "uint=%u\n", 100);
    com_file_writef(file, "bool=tRue\n");
    com_file_writef(file, "[GROUP_%d]\n", 1);
    com_file_writef(file, "id=%d\n", 1);
    com_file_writef(file, "name=%s\n", "abc");
    com_file_writef(file, "float=%.6f\n", 1.632473f);
    com_file_writef(file, "double=%.15lf\n", 1.632193878912649f);
    com_file_writef(file, "int=%d\n", -101);
    com_file_writef(file, "uint=%u\n", 101);
    com_file_writef(file, "bool=trUe\n");
    com_file_writef(file, "[GROUP_%d]\n", 2);
    com_file_writef(file, "id=%d\n", 2);
    com_file_writef(file, "name=%s\n", "def");
    com_file_writef(file, "float=%.6f\n", 2.23f);
    com_file_writef(file, "double=%.15lf\n", 2.632193878912649f);
    com_file_writef(file, "int=%d\n", -102);
    com_file_writef(file, "uint=%u\n", 102);
    com_file_writef(file, "bool=FaLSe\n");
    com_file_writef(file, "[GROUP_%d]\n", 3);
    com_file_writef(file, "id=%d\n", 3);
    com_file_writef(file, "name=%s;comment\n", "ghi");
    com_file_writef(file, "float=%.6f\n", 3.23f);
    com_file_writef(file, "double=%.15lf\n", 3.632193878912649f);
    com_file_writef(file, "int=%d\n", -103);
    com_file_writef(file, "uint=%u\n", 103);
    com_file_writef(file, "bool=falSe\n");
    com_file_writef(file, "empty=\n");
    com_file_flush(file);
    com_file_close(file);
    CPPConfig config("1.ini", false, true, "/tmp/");
    ASSERT_FALSE(config.isGroupExist(NULL));
    ASSERT_TRUE(config.isGroupExist("GROUP_1"));
    ASSERT_TRUE(config.isGroupExist("GROUP_2"));
    ASSERT_TRUE(config.isGroupExist("GROUP_3"));
    ASSERT_INT_EQUAL(config.getGroupKeyCount(NULL), -1);
    ASSERT_INT_EQUAL(config.getInt32(NULL, "id"), 0);
    ASSERT_STR_EQUAL(config.getString(NULL, "name").c_str(), "000");
    ASSERT_INT_EQUAL(config.getInt32(NULL, "int"), -100);
    ASSERT_INT_EQUAL(config.getUInt32(NULL, "uint"), 100);
    ASSERT_TRUE(config.getBool(NULL, "bool", false));
    ASSERT_INT_EQUAL(config.getGroupKeyCount("GROUP_1"), 7);
    ASSERT_INT_EQUAL(config.getInt32("GROUP_1", "id"), 1);
    ASSERT_STR_EQUAL(config.getString("GROUP_1", "name").c_str(), "abc");
    ASSERT_INT_EQUAL(config.getInt32("GROUP_1", "int"), -101);
    ASSERT_INT_EQUAL(config.getUInt32("GROUP_1", "uint"), 101);
    ASSERT_TRUE(config.getBool("GROUP_1", "bool", false));
    ASSERT_INT_EQUAL(config.getGroupKeyCount("GROUP_2"), 7);
    ASSERT_INT_EQUAL(config.getInt32("GROUP_2", "id"), 2);
    ASSERT_STR_EQUAL(config.getString("GROUP_2", "name").c_str(), "def");
    ASSERT_INT_EQUAL(config.getInt32("GROUP_2", "int"), -102);
    ASSERT_INT_EQUAL(config.getUInt32("GROUP_2", "uint"), 102);
    ASSERT_FALSE(config.getBool("GROUP_2", "bool", false));
    ASSERT_INT_EQUAL(config.getGroupKeyCount("GROUP_3"), 8);
    ASSERT_INT_EQUAL(config.getInt32("GROUP_3", "id"), 3);
    ASSERT_STR_EQUAL(config.getString("GROUP_3", "name").c_str(), "ghi");
    ASSERT_INT_EQUAL(config.getInt32("GROUP_3", "int"), -103);
    ASSERT_INT_EQUAL(config.getUInt32("GROUP_3", "uint"), 103);
    ASSERT_FALSE(config.getBool("GROUP_3", "bool", false));
    ASSERT_STR_EQUAL(config.getString("GROUP_3", "empty").c_str(), "");
    com_file_remove("1.ini");
    LOG_D("bin name=%s", com_get_bin_name());
    LOG_D("bin name=%s", com_get_bin_name());
    LOG_D("bin path=%s", com_get_bin_path());
    LOG_D("bin path=%s", com_get_bin_path());

    config.removeGroup("group_3");
    config.removeItem("group_2", "bool");
    config.addGroup("group_4");
    config.set("group_4", "bool", 0);
    config.set("group_4", "int", std::to_string(6));
    config.set("group_4", "char*", "char*");
    config.set("group_4", "string", std::string("string"));
    config.set("group_4", "double", 123.5735);

    Message msg = config.toMessage();
    std::string val = config.toString();

    ASSERT_STR_EQUAL(val.c_str(), ";md5=3987241a82a2884619d0bd96ee1cdd7a\n[group_1]\nid=1\nname=abc\nfloat=1.632473\ndouble=1.632193922996521\nint=-101\nuint=101\nbool=trUe\n[group_2]\nid=2\nname=def\nfloat=2.230000\ndouble=2.632193803787231\nint=-102\nuint=102\n[group_4]\nbool=0\nint=6\nchar*=char*\nstring=string\ndouble=123.573500\n");

    ASSERT_TRUE(config.saveAs("1.ini.bak"));
    ASSERT_TRUE(CPPConfig::CheckSignature("1.ini.bak"));
    com_file_remove("1.ini.bak");

    ASSERT_TRUE(config.save());
    ASSERT_TRUE(CPPConfig::CheckSignature("/tmp/.1.ini"));
    ASSERT_TRUE(CPPConfig::CheckSignature("/tmp/.1.ini.bak"));
    com_file_remove("1.ini");
    com_file_remove("/tmp/.1.ini");
    com_file_remove("/tmp/.1.ini.bak");

    ASSERT_STR_EQUAL("/1/2/3/", com_path_dir("/1/2/3/4.txt").c_str());

    CPPConfig config2("2.ini");
    config2.set("hhh", "abc", 3);
    config2.set("hhh", "abcd", 4);

    ASSERT_INT_EQUAL(config2.getUInt32("hhh", "abc"), 3);
    ASSERT_INT_EQUAL(config2.getUInt32("hhh", "abcd"), 4);
    ASSERT_TRUE(config2.save());

    ASSERT_INT_EQUAL(com_file_type(".2.ini"), FILE_TYPE_NOT_EXIST);
    ASSERT_INT_EQUAL(com_file_type(".2.ini.bak"), FILE_TYPE_NOT_EXIST);

    ASSERT_TRUE(config.load("2.ini"));
    ASSERT_INT_EQUAL(config.getUInt32("hhh", "abc"), 3);
    ASSERT_INT_EQUAL(config.getUInt32("hhh", "abcd"), 4);

    com_file_remove(".2.ini");
    com_file_remove(".2.ini.bak");
}

