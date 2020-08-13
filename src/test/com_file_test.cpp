#include "com.h"

void com_file_unit_test_suit(void** state)
{
    ByteArray bytes = com_file_readall("./CMakeLists.txt");
    ASSERT_FALSE(bytes.empty());

    com_dir_create("./1");
    ASSERT_INT_EQUAL(com_file_type("./1"), FILE_TYPE_DIR);
    com_dir_remove("./1");
    ASSERT_INT_EQUAL(com_file_type("./1"), FILE_TYPE_NOT_EXIST);

    if(com_dir_create("/1") == true)
    {
        ASSERT_INT_EQUAL(com_file_type("/1"), FILE_TYPE_DIR);
        com_dir_remove("/1");
        ASSERT_INT_EQUAL(com_file_type("/1"), FILE_TYPE_NOT_EXIST);
    }

    com_dir_create("./1/2/3/4/5/6");
    ASSERT_INT_EQUAL(com_file_type("./1"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type("./1/2"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type("./1/2/3"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type("./1/2/3/4"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type("./1/2/3/4/5"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(com_file_type("./1/2/3/4/5/6"), FILE_TYPE_DIR);
    FILE* file = com_file_open("./1/2/3/3.txt", "w+");
    ASSERT_NOT_NULL(file);
    ASSERT_INT_EQUAL(com_file_type("./1/2/3/3.txt"), FILE_TYPE_FILE);
    com_file_close(file);
    com_dir_remove("./1");
    ASSERT_INT_EQUAL(com_file_type("./1"), FILE_TYPE_NOT_EXIST);

    if(com_dir_create("/1/2/3/4/5/6") == true)
    {
        ASSERT_INT_EQUAL(com_file_type("/1"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type("/1/2"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type("/1/2/3"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type("/1/2/3/4"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type("/1/2/3/4/5"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(com_file_type("/1/2/3/4/5/6"), FILE_TYPE_DIR);
        file = com_file_open("/1/2/3/3.txt", "w+");
        ASSERT_NOT_NULL(file);
        ASSERT_INT_EQUAL(com_file_type("/1/2/3/3.txt"), FILE_TYPE_FILE);
        com_file_close(file);
        com_dir_remove("/1");
        ASSERT_INT_EQUAL(com_file_type("/1"), FILE_TYPE_NOT_EXIST);
    }

    FilePath path("1/2/3/4/5/1.txt");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "1.txt");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "1/2/3/4/5/");
    ASSERT_FALSE(path.isDirectory());

    path = FilePath("/1/2/3/4/5/1.txt");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "1.txt");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "/1/2/3/4/5/");
    ASSERT_FALSE(path.isDirectory());

    path = FilePath("/1/2/3/4/5/6/");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "6");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "/1/2/3/4/5/");
    ASSERT_TRUE(path.isDirectory());

    path = FilePath("/1");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "1");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "/");
    ASSERT_FALSE(path.isDirectory());

    path = FilePath("/1/");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "1");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "/");
    ASSERT_TRUE(path.isDirectory());

    path = FilePath("./1");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "1");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "./");
    ASSERT_FALSE(path.isDirectory());

    path = FilePath("./1/");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "1");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "./");
    ASSERT_TRUE(path.isDirectory());

    path = FilePath("");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "");
    ASSERT_FALSE(path.isDirectory());

    path = FilePath("/");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "/");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "/");
    ASSERT_TRUE(path.isDirectory());

    path = FilePath(".");
    ASSERT_STR_EQUAL(path.GetName().c_str(), ".");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), ".");
    ASSERT_TRUE(path.isDirectory());

    path = FilePath("..");
    ASSERT_STR_EQUAL(path.GetName().c_str(), "..");
    ASSERT_STR_EQUAL(path.GetLocationDirectory().c_str(), "..");
    ASSERT_TRUE(path.isDirectory());


    ASSERT_STR_EQUAL(com_path_dir("1.txt").c_str(), ".");

    file = com_file_open("./test.dat", "w+");
    char* buf = new char[100 * 1024 * 1024]();
    com_file_write(file, buf, 100 * 1024 * 1024);
    com_file_close(file);
    com_file_remove("./test.dat");
    delete[] buf;
}
