#include "bcp.h"

void bcp_file_unit_test_suit(void** state)
{
    ByteArray bytes = bcp_file_readall("./CMakeLists.txt");
    ASSERT_FALSE(bytes.empty());

    bcp_dir_create("./1");
    ASSERT_INT_EQUAL(bcp_file_type("./1"), FILE_TYPE_DIR);
    bcp_dir_remove("./1");
    ASSERT_INT_EQUAL(bcp_file_type("./1"), FILE_TYPE_NOT_EXIST);

    if(bcp_dir_create("/1") == true)
    {
        ASSERT_INT_EQUAL(bcp_file_type("/1"), FILE_TYPE_DIR);
        bcp_dir_remove("/1");
        ASSERT_INT_EQUAL(bcp_file_type("/1"), FILE_TYPE_NOT_EXIST);
    }

    bcp_dir_create("./1/2/3/4/5/6");
    ASSERT_INT_EQUAL(bcp_file_type("./1"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(bcp_file_type("./1/2"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(bcp_file_type("./1/2/3"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(bcp_file_type("./1/2/3/4"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(bcp_file_type("./1/2/3/4/5"), FILE_TYPE_DIR);
    ASSERT_INT_EQUAL(bcp_file_type("./1/2/3/4/5/6"), FILE_TYPE_DIR);
    FILE* file = bcp_file_open("./1/2/3/3.txt", "w+");
    ASSERT_NOT_NULL(file);
    ASSERT_INT_EQUAL(bcp_file_type("./1/2/3/3.txt"), FILE_TYPE_FILE);
    bcp_file_close(file);
    bcp_dir_remove("./1");
    ASSERT_INT_EQUAL(bcp_file_type("./1"), FILE_TYPE_NOT_EXIST);

    if(bcp_dir_create("/1/2/3/4/5/6") == true)
    {
        ASSERT_INT_EQUAL(bcp_file_type("/1"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(bcp_file_type("/1/2"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(bcp_file_type("/1/2/3"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(bcp_file_type("/1/2/3/4"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(bcp_file_type("/1/2/3/4/5"), FILE_TYPE_DIR);
        ASSERT_INT_EQUAL(bcp_file_type("/1/2/3/4/5/6"), FILE_TYPE_DIR);
        file = bcp_file_open("/1/2/3/3.txt", "w+");
        ASSERT_NOT_NULL(file);
        ASSERT_INT_EQUAL(bcp_file_type("/1/2/3/3.txt"), FILE_TYPE_FILE);
        bcp_file_close(file);
        bcp_dir_remove("/1");
        ASSERT_INT_EQUAL(bcp_file_type("/1"), FILE_TYPE_NOT_EXIST);
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


    ASSERT_STR_EQUAL(bcp_path_dir("1.txt").c_str(), ".");

    file = bcp_file_open("./test.dat", "w+");
    char* buf = new char[100 * 1024 * 1024]();
    bcp_file_write(file, buf, 100 * 1024 * 1024);
    bcp_file_close(file);
    bcp_file_remove("./test.dat");
    delete[] buf;
}
