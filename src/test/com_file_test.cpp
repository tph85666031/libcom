#include <stdio.h>
#include "com_file.h"
#include "com_log.h"
#include "com_test.h"

void com_file_unit_test_suit(void** state)
{
    CPPBytes bytes = com_file_readall(PATH_TO_LOCAL(com_get_bin_path() + com_get_bin_name()).c_str());
    ASSERT_FALSE(bytes.empty());

    com_dir_create(PATH_TO_LOCAL("./1").c_str());
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1").c_str()), FILE_TYPE_DIR);
    com_dir_remove(PATH_TO_LOCAL("./1").c_str());
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1").c_str()), FILE_TYPE_NOT_EXIST);

#if !defined(_WIN32) && !defined(_WIN64)
    if(com_dir_create(PATH_TO_LOCAL("/1").c_str()) == true)
    {
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1").c_str()), FILE_TYPE_DIR);
        com_dir_remove(PATH_TO_LOCAL("/1").c_str());
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1").c_str()), FILE_TYPE_NOT_EXIST);
    }
#endif
    com_dir_create(PATH_TO_LOCAL("./1/2/3/4/5/6").c_str());
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1").c_str()), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1/2").c_str()), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1/2/3").c_str()), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1/2/3/4").c_str()), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1/2/3/4/5").c_str()), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1/2/3/4/5/6").c_str()), FILE_TYPE_DIR);
    FILE* file = com_file_open(PATH_TO_LOCAL("./1/2/3/3.txt").c_str(), "w+");
    ASSERT_NOT_NULL(file);
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1/2/3/3.txt").c_str()), FILE_TYPE_FILE);
    com_file_close(file);
    com_dir_remove(PATH_TO_LOCAL("./1").c_str());
    ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("./1").c_str()), FILE_TYPE_NOT_EXIST);

#if !defined(_WIN32) && !defined(_WIN64)
    if(com_dir_create(PATH_TO_LOCAL("/1/2/3/4/5/6").c_str()) == true)
    {
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1").c_str()), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1/2").c_str()), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1/2/3").c_str()), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1/2/3/4").c_str()), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1/2/3/4/5").c_str()), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1/2/3/4/5/6").c_str()), FILE_TYPE_DIR);
        file = com_file_open(PATH_TO_LOCAL("/1/2/3/3.txt").c_str(), "w+");
        ASSERT_NOT_NULL(file);
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1/2/3/3.txt").c_str()), FILE_TYPE_FILE);
        com_file_close(file);
        com_dir_remove(PATH_TO_LOCAL("/1").c_str());
        ASSERT_INT_EQUAL(com_file_type(PATH_TO_LOCAL("/1").c_str()), FILE_TYPE_NOT_EXIST);
    }
#endif

    FilePath path(PATH_TO_LOCAL("1/2/3/4/5/1.txt").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), PATH_TO_LOCAL("1.txt").c_str());
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL("1/2/3/4/5").c_str());
    ASSERT_STR_EQUAL(path.getSuffix().c_str(), "txt");

#if !defined(_WIN32) && !defined(_WIN64)
    path = FilePath(PATH_TO_LOCAL("/1/2/3/4/5/1.txt").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), "1.txt");
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL("/1/2/3/4/5").c_str());

    path = FilePath(PATH_TO_LOCAL("/1/2/3/4/5/6/").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), "6");
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL("/1/2/3/4/5").c_str());

    path = FilePath(PATH_TO_LOCAL("/1").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), "1");
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL("/").c_str());

    path = FilePath(PATH_TO_LOCAL("/1/").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), "1");
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL("/").c_str());
    ASSERT_STR_EQUAL(path.getSuffix().c_str(), "");
#endif

    path = FilePath(PATH_TO_LOCAL("./1").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), "1");
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL(".").c_str());

    path = FilePath(PATH_TO_LOCAL("./1/").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), "1");
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL(".").c_str());

    path = FilePath("");
    ASSERT_STR_EQUAL(path.getName().c_str(), "");
    ASSERT_STR_EQUAL(path.getDir().c_str(), "");

#if !defined(_WIN32) && !defined(_WIN64)
    path = FilePath(PATH_TO_LOCAL("/").c_str());
    ASSERT_STR_EQUAL(path.getName().c_str(), PATH_TO_LOCAL("/").c_str());
    ASSERT_STR_EQUAL(path.getDir().c_str(), PATH_TO_LOCAL("/").c_str());
#endif

    path = FilePath(".");
    ASSERT_STR_EQUAL(path.getName().c_str(), ".");
    ASSERT_STR_EQUAL(path.getDir().c_str(), ".");

    path = FilePath("..");
    ASSERT_STR_EQUAL(path.getName().c_str(), "..");
    ASSERT_STR_EQUAL(path.getDir().c_str(), "..");

    ASSERT_STR_EQUAL(com_path_dir("1.txt").c_str(), ".");

    file = com_file_open(PATH_TO_LOCAL("./test.dat").c_str(), "w+");
    char* buf = new char[100 * 1024 * 1024]();
    com_file_write(file, buf, 100 * 1024 * 1024);
    com_file_close(file);
    com_file_remove(PATH_TO_LOCAL("./test.dat").c_str());

    file = com_file_open(PATH_TO_LOCAL("./test.dat").c_str(), "w+");
    com_file_write(file, "12345\n67890\nabcd\nefgh\n", sizeof("12345\n67890\nabcd\nefgh\n"));
    com_file_flush(file);
    com_file_close(file);

    file = com_file_open(PATH_TO_LOCAL("./test.dat").c_str(), "r");
    com_file_readline(file, buf, 1024);
    ASSERT_STR_NOT_EQUAL(buf, "12345\n");
    ASSERT_STR_EQUAL(buf, "12345");

    com_file_readline(file, buf, 1024);
    ASSERT_STR_NOT_EQUAL(buf, "67890\n");
    ASSERT_STR_EQUAL(buf, "67890");
    com_file_close(file);
    com_file_remove(PATH_TO_LOCAL("./test.dat").c_str());

    com_file_appendf(PATH_TO_LOCAL("./test.dat").c_str(), "value=%d", 123);
    com_file_copy(PATH_TO_LOCAL("./test_copy.dat").c_str(), PATH_TO_LOCAL("./test.dat").c_str());

    file = com_file_open(PATH_TO_LOCAL("./test_copy.dat").c_str(), "r");
    std::string line;
    com_file_readline(file, line);
    ASSERT_STR_EQUAL(line.c_str(), "value=123");
    com_file_close(file);

    com_file_remove(PATH_TO_LOCAL("./test.dat").c_str());
    com_file_remove(PATH_TO_LOCAL("./test_copy.dat").c_str());

    delete[] buf;

    com_dir_create(PATH_TO_LOCAL("./1/2/3").c_str());
    com_file_create(PATH_TO_LOCAL("./1/2/3/c3.txt").c_str());
    com_file_create(PATH_TO_LOCAL("./1/2/b2.txt").c_str());
    com_file_create(PATH_TO_LOCAL("./1/a1.txt").c_str());

    std::map<std::string, int> list;
    com_dir_list(PATH_TO_LOCAL("./1").c_str(), list, false);
    ASSERT_INT_EQUAL(list.size(), 2);

    list.clear();
    com_dir_list(PATH_TO_LOCAL("./1").c_str(), list, true);
    ASSERT_INT_EQUAL(list.size(), 5);

    list.clear();
    com_dir_list(PATH_TO_LOCAL("./1/*.txt").c_str(), list, true);
    ASSERT_INT_EQUAL(list.size(), 3);

    list.clear();
    com_dir_list(PATH_TO_LOCAL("./1/2/*.txt").c_str(), list, true);
    ASSERT_INT_EQUAL(list.size(), 2);

    list.clear();
    com_dir_list(PATH_TO_LOCAL("./1/2/3/*.txt").c_str(), list, true);
    ASSERT_INT_EQUAL(list.size(), 1);

    list.clear();
    com_dir_list(PATH_TO_LOCAL("./1/*.txt").c_str(), list, false);
    ASSERT_INT_EQUAL(list.size(), 1);

    list.clear();
    com_dir_list(PATH_TO_LOCAL("./1/2/*.txt").c_str(), list, false);
    ASSERT_INT_EQUAL(list.size(), 1);

    list.clear();
    com_dir_list(PATH_TO_LOCAL("./1/2/3/*.txt").c_str(), list, false);
    ASSERT_INT_EQUAL(list.size(), 1);

    com_file_create(PATH_TO_LOCAL("./file_test_time.txt").c_str());
    FileDetail detail(PATH_TO_LOCAL("./file_test_time.txt").c_str());
    ASSERT_STR_EQUAL(detail.getName().c_str(), "file_test_time.txt");
    ASSERT_STR_EQUAL(detail.getSuffix().c_str(), "txt");
    detail.setAccessTime();
    ASSERT_INT_EQUAL(detail.getChangeTimeS(), detail.getAccessTimeS());
    detail.setModifyTime();
    ASSERT_INT_EQUAL(detail.getChangeTimeS(), detail.getAccessTimeS());
    ASSERT_INT_EQUAL(detail.getModifyTimeS(), detail.getAccessTimeS());
    com_file_remove(PATH_TO_LOCAL("./file_test_time.txt").c_str());

    com_file_writef(PATH_TO_LOCAL("./line.txt").c_str(), "1234567890");
    com_file_truncate(PATH_TO_LOCAL("./line.txt").c_str(), 2);
    ASSERT_INT_EQUAL(com_file_size(PATH_TO_LOCAL("./line.txt").c_str()), 8);
    com_file_clean(PATH_TO_LOCAL("./line.txt").c_str());
    ASSERT_INT_EQUAL(com_file_size(PATH_TO_LOCAL("./line.txt").c_str()), 0);

    com_file_remove(PATH_TO_LOCAL("./line.txt").c_str());

    int64 pos = com_file_rfind("./1.rtf", "{\\rtf1");
    LOG_I("pos=%lld", pos);

    pos = com_file_rfind("./1.rtf", "\\rtf1");
    LOG_I("pos=%lld", pos);

    pos = com_file_rfind("./1.rtf", "\\lisb0");
    LOG_I("pos=%lld", pos);

    pos = com_file_rfind("./1.rtf", "\\par");
    LOG_I("pos=%lld", pos);

    pos = com_file_rfind("./1.rtf", "}");
    LOG_I("pos=%lld", pos);

    file = com_file_open("./1.rtf", "rb");
    CPPBytes val = com_file_read_until(file, [](uint8 val)
    {
        return val == '9';
    });
    LOG_I("val=%s", val.toString().c_str());
    val = com_file_read_until(file, [](uint8 val)
    {
        return val == '2';
    });
    LOG_I("val=%s", val.toString().c_str());

    val = com_file_read_until(file, "fonttbl");
    LOG_I("val=%s", val.toString().c_str());
    val = com_file_read_until(file, "fcharset0");
    LOG_I("val=%s", val.toString().c_str());

    int count = 0;
    val = com_file_read_until(file, [&count](uint8 val)
    {
        if(val == '(')
        {
            count++;
        }
        else if(val == ')')
        {
            if(count-- == 0)
            {
                return true;
            }
        }
        return false;
    });
    //LOG_I("val=%s", val.toString().c_str());

    com_file_close(file);
    com_file_remove(PATH_TO_LOCAL("./1.rtf").c_str());
}

