#include <iostream>
#include "com_base.h"
#include "com_log.h"
#include "com_test.h"
#include "com_stack.h"
#include "com_file.h"
#include "com_thread.h"
#include "CJsonObject.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace std;

static std::mutex mutex_test;

static int mock_test(int* val_a, int* val_b, int val_c)
{
    TEST_FCALL_MARK();
    TEST_EXPECT_CHECK(val_c);
    *val_a = TEST_PARAM_POP(int);
    *val_b = TEST_PARAM_POP(int);
    int ret = TEST_PARAM_POP(int);
    return ret;
}

static ComBytes bytearray_test(uint8* data, int dataSize)
{
    ComBytes bytes;
    if(data == NULL || dataSize <= 0 || dataSize % 8 != 0)
    {
        return bytes;
    }

    ComBytes bytesSrc(data, dataSize);
    uint8 buf[8];
    for(int i = 0; i < bytesSrc.getDataSize() / 8; i++)
    {
        memcpy(buf, bytesSrc.getData() + 8 * i, 8);
        bytes.append(buf, sizeof(buf));
    }

    return bytes;
}

static void thread_condition_test(void* arg)
{
    if(arg == NULL)
    {
        return;
    }
    com_sleep_ms(1000);
    ComCondition* condition = (ComCondition*)arg;
    LOG_D("condition start notify");
    condition->notifyAll();
    LOG_D("condition notify done");
    return;
}

void com_base_string_split_unit_test_suit(void** state)
{
    std::vector<std::string> ret_split = com_string_split("123,456,7", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 3);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "123");
    ASSERT_STR_EQUAL(ret_split[1].c_str(), "456");
    ASSERT_STR_EQUAL(ret_split[2].c_str(), "7");
    ret_split = com_string_split("123,456,7,", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 4);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "123");
    ASSERT_STR_EQUAL(ret_split[1].c_str(), "456");
    ASSERT_STR_EQUAL(ret_split[2].c_str(), "7");
    ASSERT_STR_EQUAL(ret_split[3].c_str(), "");
    ret_split = com_string_split(",123,456,7", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 4);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "");
    ASSERT_STR_EQUAL(ret_split[1].c_str(), "123");
    ASSERT_STR_EQUAL(ret_split[2].c_str(), "456");
    ASSERT_STR_EQUAL(ret_split[3].c_str(), "7");
    ret_split = com_string_split(",123,456,7,", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 5);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "");
    ASSERT_STR_EQUAL(ret_split[1].c_str(), "123");
    ASSERT_STR_EQUAL(ret_split[2].c_str(), "456");
    ASSERT_STR_EQUAL(ret_split[3].c_str(), "7");
    ASSERT_STR_EQUAL(ret_split[4].c_str(), "");
    ret_split = com_string_split(",,456,7,", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 5);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "");
    ASSERT_STR_EQUAL(ret_split[1].c_str(), "");
    ASSERT_STR_EQUAL(ret_split[2].c_str(), "456");
    ASSERT_STR_EQUAL(ret_split[3].c_str(), "7");
    ASSERT_STR_EQUAL(ret_split[4].c_str(), "");
    ret_split = com_string_split(",123,,7,", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 5);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "");
    ASSERT_STR_EQUAL(ret_split[1].c_str(), "123");
    ASSERT_STR_EQUAL(ret_split[2].c_str(), "");
    ASSERT_STR_EQUAL(ret_split[3].c_str(), "7");
    ASSERT_STR_EQUAL(ret_split[4].c_str(), "");
    ret_split = com_string_split(",123,,7,", ",");
    ASSERT_INT_EQUAL(ret_split.size(), 5);
    ret_split = com_string_split(NULL, ",");
    ASSERT_INT_EQUAL(ret_split.size(), 0);
    ret_split = com_string_split(NULL, ",");
    ASSERT_INT_EQUAL(ret_split.size(), 0);
    ret_split = com_string_split("123", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 1);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "123");
    ret_split = com_string_split(" ", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 1);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), " ");
    ret_split = com_string_split("", ",");
    ASSERT_TRUE(ret_split.size() > 0);
    ASSERT_INT_EQUAL(ret_split.size(), 1);
    ASSERT_STR_EQUAL(ret_split[0].c_str(), "");
}

void com_base_string_unit_test_suit(void** state)
{
    char buf_a[8] = {0};
    char buf_b[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_MEM_EQUAL(buf_a, buf_b, 8);
    com_strncpy(buf_a, "123", sizeof(buf_a));
    ASSERT_STR_EQUAL(buf_a, "123");
    com_strncpy(buf_a, "12345678", sizeof(buf_a));
    ASSERT_STR_EQUAL(buf_a, "1234567");
    ASSERT_STR_NOT_EQUAL(buf_a, "12345678");
    com_strncpy(buf_a, "123", sizeof(buf_a));
    ASSERT_STR_EQUAL(buf_a, "123");
    ASSERT_NULL(com_strncpy(buf_a, NULL, sizeof(buf_a)));
    ASSERT_NULL(com_strncpy(buf_a, "123", 0));
    ASSERT_NULL(com_strncpy(buf_a, "123", -1));
    ASSERT_NULL(com_strncpy(NULL, "123", 0));
    ASSERT_TRUE(com_strncmp("abc", "abc", 3));
    ASSERT_TRUE(com_strncmp("abc", "abcd", 3));
    ASSERT_TRUE(com_strncmp("abcd", "abc", 3));
    ASSERT_TRUE(com_strncmp("aBc", "aBc", 3));
    ASSERT_FALSE(com_strncmp("aBc", "abc", 3));
    ASSERT_FALSE(com_strncmp("abC", "abc", 3));
    ASSERT_FALSE(com_strncmp("Abc", "abc", 3));
    ASSERT_FALSE(com_strncmp(NULL, "abc", 3));
    ASSERT_FALSE(com_strncmp("abc", NULL, 3));
    ASSERT_FALSE(com_strncmp("abc", "abc", 0));
    ASSERT_FALSE(com_strncmp("abc", "abc", -1));
    ASSERT_TRUE(com_strncmp_ignore_case("abc", "abc", 3));
    ASSERT_TRUE(com_strncmp_ignore_case("abc", "abcd", 3));
    ASSERT_TRUE(com_strncmp_ignore_case("abcd", "abc", 3));
    ASSERT_TRUE(com_strncmp_ignore_case("aBc", "aBc", 3));
    ASSERT_TRUE(com_strncmp_ignore_case("aBc", "abc", 3));
    ASSERT_TRUE(com_strncmp_ignore_case("abC", "abc", 3));
    ASSERT_TRUE(com_strncmp_ignore_case("Abc", "abc", 3));
    ASSERT_FALSE(com_strncmp_ignore_case(NULL, "abc", 3));
    ASSERT_FALSE(com_strncmp_ignore_case("abc", NULL, 3));
    ASSERT_FALSE(com_strncmp_ignore_case("abc", "abc", 0));
    ASSERT_FALSE(com_strncmp_ignore_case("abc", "abc", -1));
    ASSERT_TRUE(com_string_equal("abc", "abc"));
    ASSERT_FALSE(com_string_equal("abc", "Abc"));
    ASSERT_TRUE(com_string_equal("", ""));
    ASSERT_FALSE(com_string_equal("abc", "123"));
    ASSERT_FALSE(com_string_equal(NULL, "123"));
    ASSERT_FALSE(com_string_equal("123", NULL));
    ASSERT_TRUE(com_string_equal_ignore_case("abc", "abc"));
    ASSERT_TRUE(com_string_equal_ignore_case("abc", "Abc"));
    ASSERT_TRUE(com_string_equal_ignore_case("", ""));
    ASSERT_FALSE(com_string_equal_ignore_case("abc", "123"));
    ASSERT_FALSE(com_string_equal_ignore_case(NULL, "123"));
    ASSERT_FALSE(com_string_equal_ignore_case("123", NULL));
    char buf[512];
    com_strncpy(buf,  " 123 ", sizeof(buf));
    com_string_trim_left(buf);
    ASSERT_STR_EQUAL(buf, "123 ");
    com_string_trim_right(buf);
    ASSERT_STR_EQUAL(buf, "123");
    com_strncpy(buf, " 123 ", sizeof(buf));
    com_string_trim(buf);
    ASSERT_STR_EQUAL(buf, "123");
    com_string_trim_left(NULL);
    com_string_trim_right(NULL);
    com_string_trim(NULL);

    com_strncpy(buf, " 123\t ", sizeof(buf));
    com_string_trim(buf);
    ASSERT_STR_EQUAL(buf, "123");

    com_strncpy(buf, " \v123\t ", sizeof(buf));
    com_string_trim(buf);
    ASSERT_STR_EQUAL(buf, "123");

    com_strncpy(buf, " \t\v123\t\f\n\r ", sizeof(buf));
    com_string_trim(buf);
    ASSERT_STR_EQUAL(buf, "123");

    com_strncpy(buf, "abcfglldf fdljgoiwjrg", sizeof(buf));
    ASSERT_TRUE(com_string_start_with(buf, "abc"));
    ASSERT_TRUE(com_string_end_with(buf, "jrg"));
    ASSERT_TRUE(com_string_contain(buf, "fdl"));
    ASSERT_TRUE(com_string_start_with(buf, "a"));
    ASSERT_TRUE(com_string_end_with(buf, "g"));
    ASSERT_TRUE(com_string_contain(buf, "lj"));
    ASSERT_FALSE(com_string_start_with(buf, NULL));
    ASSERT_FALSE(com_string_start_with(NULL, "123"));
    ASSERT_FALSE(com_string_start_with(NULL, NULL));
    ASSERT_FALSE(com_string_end_with(buf, NULL));
    ASSERT_FALSE(com_string_end_with(NULL, "123"));
    ASSERT_FALSE(com_string_end_with(NULL, NULL));
    ASSERT_FALSE(com_string_contain(buf, NULL));
    ASSERT_FALSE(com_string_contain(NULL, "123"));
    ASSERT_FALSE(com_string_contain(NULL, NULL));
    std::string str_x("abCDef");
    str_x = com_string_to_upper(str_x);
    ASSERT_STR_EQUAL(str_x.c_str(), "ABCDEF");
    str_x = com_string_to_lower(str_x);
    ASSERT_STR_EQUAL(str_x.c_str(), "abcdef");
    com_strncpy(buf, "abcABC", sizeof(buf));
    com_string_to_lower(buf);
    ASSERT_STR_EQUAL(buf, "abcabc");
    com_string_to_upper(buf);
    ASSERT_STR_EQUAL(buf, "ABCABC");
    ASSERT_NULL(com_string_to_lower(NULL));
    ASSERT_NULL(com_string_to_upper(NULL));
    com_strncpy(buf, "abcABC", sizeof(buf));
    com_string_replace(buf, 'a', 'X');
    ASSERT_STR_EQUAL(buf, "XbcABC");
    com_strncpy(buf, "abcABC", sizeof(buf));
    com_string_replace(buf, 'C', 'X');
    ASSERT_STR_EQUAL(buf, "abcABX");
    com_strncpy(buf, "abcABC", sizeof(buf));
    com_string_replace(buf, 'x', 'X');
    ASSERT_STR_EQUAL(buf, "abcABC");
    com_strncpy(buf, "line1\nline2\nline3", sizeof(buf));

    std::string fmt_str = com_string_format("str=%s", "abc");
    ASSERT_STR_EQUAL(fmt_str.c_str(), "str=abc");
    fmt_str = com_string_format(NULL);
    ASSERT_STR_EQUAL(fmt_str.c_str(), "");

    std::wstring fmt_wstr = com_wstring_format(L"str=%s", L"abc");

    LOG_D("uuid=%s", com_uuid_generator().c_str());
    LOG_D("uuid=%s", com_uuid_generator().c_str());
    LOG_D("uuid=%s", com_uuid_generator().c_str());
    LOG_D("uuid=%s", com_uuid_generator().c_str());
    LOG_D("uuid=%s", com_uuid_generator().c_str());
    LOG_D("uuid=%s", com_uuid_generator().c_str());
    LOG_D("uuid=%s", com_uuid_generator().c_str());

    std::string x = "hh   ggggg   ";
    com_string_trim(x, " h");
    ASSERT_STR_EQUAL(x.c_str(), "ggggg");

    x = "If you already have files you can push them using command line instructions below";
    com_string_replace(x, "you", "U");
    ASSERT_STR_EQUAL(x.c_str(), "If U already have files U can push them using command line instructions below");

    com_string_replace(x, "If", "kk");
    ASSERT_STR_EQUAL(x.c_str(), "kk U already have files U can push them using command line instructions below");

    com_string_replace(x, "kk", "IF");
    com_string_replace(x, "below", "");
    ASSERT_STR_EQUAL(x.c_str(), "IF U already have files U can push them using command line instructions ");

    std::string str_utf8 = com_wstring_to_utf8(L"测试").toString();
    ASSERT_STR_EQUAL(com_string_utf8_to_local(str_utf8.c_str()).c_str(), "测试");
    ComBytes utf16 = com_string_utf8_to_utf16(ComBytes("中文测试"));
    ComBytes utf32 = com_string_utf16_to_utf32(utf16);
    ComBytes utf8 = com_string_utf32_to_utf8(utf32);
    ASSERT_STR_EQUAL(utf8.toString().c_str(), "中文测试");

    ComBytes utf16_2 = com_string_utf32_to_utf16(utf32);
    ASSERT_TRUE(utf16 == utf16_2);
    ComBytes utf8_2 = com_string_utf16_to_utf8(utf16);
    ASSERT_TRUE(utf8 == utf8_2);
    std::wstring wstr = com_wstring_from_utf8(utf8_2);
    ComBytes utf8_3 = com_wstring_to_utf8(wstr);
    ASSERT_TRUE(utf8_2 == utf8_3);

    ASSERT_TRUE(com_string_match("123456", "123?56"));
    ASSERT_TRUE(com_string_match("123456", "123*56"));
    ASSERT_TRUE(com_string_match("123456", "*"));
    ASSERT_TRUE(com_string_match("123456", "?*"));
    ASSERT_TRUE(com_string_match("123456", "*?"));
    ASSERT_TRUE(com_string_match("123456", "**"));
    ASSERT_TRUE(com_string_match("123456", "?*?"));
    ASSERT_TRUE(com_string_match("123456", "*?*"));
    ASSERT_FALSE(com_string_match("123456", "?"));
    ASSERT_FALSE(com_string_match("123456", "??"));
    ASSERT_FALSE(com_string_match("123456", "???"));
    ASSERT_FALSE(com_string_match("123456", "????"));
    ASSERT_FALSE(com_string_match("123456", "?????"));
    ASSERT_TRUE(com_string_match("123456", "??????"));

    ASSERT_TRUE(com_string_match("123456", "123[3-4]56"));
    ASSERT_TRUE(com_string_match("123456", "123[34]56"));
    ASSERT_FALSE(com_string_match("123456", "123[23]56"));
    ASSERT_TRUE(com_string_match("123456", "123[!1-3]56"));
    ASSERT_FALSE(com_string_match("123456", "123[!1-4]56"));
    ASSERT_FALSE(com_string_match("123456", "123[!64]56"));
    ASSERT_FALSE(com_string_match("123456", "123[2-3]56"));

    ASSERT_TRUE(com_string_match("1/2/3/4/5/6/a.txt", "1/?/3/4/5/6/a.txt"));
    ASSERT_TRUE(com_string_match("1/2/3/4/5/6/a.txt", "1/?/3/4/5/6/a.*"));
    ASSERT_TRUE(com_string_match("1/2/3/4/5/6/a.txt", "1/?/3/4/5/6/*.txt"));
    ASSERT_FALSE(com_string_match(PATH_TO_LOCAL("/1/2/3/4.txt").c_str(), PATH_TO_LOCAL("/1/2/*.txt").c_str(), true));
    ASSERT_TRUE(com_string_match(PATH_TO_LOCAL("/1/2/3/4.txt").c_str(), PATH_TO_LOCAL("/1/2/*.txt").c_str(), false));

    ASSERT_TRUE(com_string_is_utf8("abcd"));
    ASSERT_TRUE(com_string_is_utf8("UTF8中文"));
    ASSERT_FALSE(com_string_is_utf8(NULL));
}

void com_base_time_unit_test_suit(void** state)
{
    uint32 val_32 = com_time_rtc_s();
    //printf("time_s=%u\n", val_32);
    //uint64 val_64 = com_time_rtc_ms();
    //printf("time_s=%llu\n", val_64);
    int32 timezone_s = com_timezone_get_s();
    //printf("timezone_s=%d, timezone_h=%d\n", timezone_s, timezone_s / 3600);
    ASSERT_INT_EQUAL(timezone_s, 8 * 3600);
    //val_64 = com_time_cpu_ms();
    //printf("tick_ms=%llu\n", val_64);
    struct tm tm_val;
    val_32 = com_time_rtc_s();
    com_time_to_tm(val_32, &tm_val);
#if 0
    printf("tm=%d-%d-%d %d:%d:%d week=%d\n",
           tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
           tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec, tm_val.tm_wday);
#endif
#if 0
    printf("tm=%d-%d-%d %d:%d:%d week=%d\n",
           tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
           tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec, tm_val.tm_wday);
#endif
    ASSERT_TRUE(com_time_to_string(com_time_rtc_s(), "%Y-%m-%d %H:%M:%S %z").length() > 0);
    //printf("date=%s\n", date_str);
    ASSERT_TRUE(com_time_from_string("2018-07-27 13:59:01") > 0);
    ASSERT_TRUE(com_time_to_string(com_time_from_tm(&tm_val), "%Y-%m-%d %H:%M:%S %z").length() > 0);
    //printf("date_check=%s      %s\n", "2018-07-27 13:59:01", date_str);
    uint32 time_cur_s = com_time_rtc_s();
    com_time_to_tm(time_cur_s, &tm_val);
}

void com_base_gps_unit_test_suit(void** state)
{
    //printf("lon=%f,lat=%f\n", 104.111696, 30.489359);
    //GPS gps = com_gps_wgs84_to_gcj02(104.111696, 30.489359);
    //printf("lon=%f,lat=%f\n", gps.longitude, gps.latitude);
    double x1 = 104.0571437219;
    double y1 = 30.5742331250;
    double x2 = 104.0652295420;
    double y2 = 30.5743978939;
    double distance_m = com_gps_distance_m(x1, y1, x2, y2);
    //printf("distance_m=%f\n", distance_m);
    ASSERT_IN_RANGE(distance_m, 750, 850);
}


class XT
{
    int x = 0;
    int y = 0;
};

XT x;
XT* p_x = &x;

XT& getXT()
{
    return *p_x;
}

void com_base_unit_test_suit(void** state)
{
    com_stack_init();
    TIME_COST();

    std::vector<int> array_a;
    array_a.push_back(1);
    array_a.push_back(2);
    array_a.push_back(3);

    CJsonObject json_a;
    json_a.Add("a", "123");
    json_a.Add("array", array_a);
    LOG_I("%s", json_a.ToString().c_str());

    std::map<std::string, int> array_m;
    array_m["a"] = 1;
    array_m["b"] = 2;
    array_m["c"] = 3;

    CJsonObject json_m;
    json_m.Add("a", "123");
    json_m.Add("map", array_m);
    LOG_I("%s", json_m.ToString().c_str());

    ComCondition condition;
    std::thread thread_test(thread_condition_test, &condition);
    LOG_D("condition start wait");
    condition.wait();
    LOG_D("condition wait done");
    if(thread_test.joinable())
    {
        thread_test.join();
    }

    int val_a = 0;
    int val_b = 0;
    TEST_EXPECT_SET(mock_test, val_c, 30);
    TEST_PARAM_PUSH(mock_test, 11);
    TEST_PARAM_PUSH(mock_test, 22);
    TEST_PARAM_PUSH(mock_test, 33);
    TEST_FCALL_SET_X(mock_test, 1);
    int ret = mock_test(&val_a, &val_b, 30);
    LOG_D("val_a=%d,val_b=%d,ret=%d", val_a, val_b, ret);
    ASSERT_INT_EQUAL(val_a, 11);
    ASSERT_INT_EQUAL(val_b, 22);
    ASSERT_INT_EQUAL(ret, 33);

    void* p1 = (void*)0x123456;
    uint64 val =  com_ptr_to_number(p1);
    ASSERT_INT_EQUAL(val, 0x123456);
    void* p2 = com_number_to_ptr(val);
    ASSERT_PTR_EQUAL(p2, (void*)0x123456);

    std::vector<std::string> vals = com_string_split("a,b,c", ",");
    ASSERT_INT_EQUAL(vals.size(), 3);
    ASSERT_STR_EQUAL(vals[0].c_str(), "a");
    ASSERT_STR_EQUAL(vals[1].c_str(), "b");
    ASSERT_STR_EQUAL(vals[2].c_str(), "c");

    vals = com_string_split(",a,b,c", ",");
    ASSERT_INT_EQUAL(vals.size(), 4);
    ASSERT_STR_EQUAL(vals[0].c_str(), "");
    ASSERT_STR_EQUAL(vals[1].c_str(), "a");
    ASSERT_STR_EQUAL(vals[2].c_str(), "b");
    ASSERT_STR_EQUAL(vals[3].c_str(), "c");

    vals = com_string_split(",a,b,c,", ",");
    ASSERT_INT_EQUAL(vals.size(), 5);
    ASSERT_STR_EQUAL(vals[0].c_str(), "");
    ASSERT_STR_EQUAL(vals[1].c_str(), "a");
    ASSERT_STR_EQUAL(vals[2].c_str(), "b");
    ASSERT_STR_EQUAL(vals[3].c_str(), "c");
    ASSERT_STR_EQUAL(vals[4].c_str(), "");

    vals = com_string_split("MARKaMARKbMARKcMARK", "MARK");
    ASSERT_INT_EQUAL(vals.size(), 5);
    ASSERT_STR_EQUAL(vals[0].c_str(), "");
    ASSERT_STR_EQUAL(vals[1].c_str(), "a");
    ASSERT_STR_EQUAL(vals[2].c_str(), "b");
    ASSERT_STR_EQUAL(vals[3].c_str(), "c");
    ASSERT_STR_EQUAL(vals[4].c_str(), "");

    int gcd = com_gcd(140, 21, 14, 28);
    ASSERT_INT_EQUAL(gcd, 7);
    try
    {
        throw(ComException("test exception", 0));
    }
    catch(ComException& e)
    {
        ASSERT_STR_EQUAL("test exception", e.what());
    }

#if defined(_WIN32) || defined(_WIN64)
    ASSERT_STR_EQUAL(com_get_bin_name().c_str(), "com.exe");
#else
    ASSERT_STR_EQUAL(com_get_bin_name().c_str(), "com");
#endif

    TIME_COST_SHOW();
    Message msg;
    ComBytes b1(10 * 1014 * 1024);
    for(int i = 0; i < 1024 * 1024; i++)
    {
        b1.append((uint8*)"0123456789", 10);
        msg.set(std::to_string(i).c_str(), "0123456789");
    }

    TIME_COST_SHOW();
    ComBytes b2 = b1;
    TIME_COST_SHOW();
    ComBytes b3 = std::move(b1);
    TIME_COST_SHOW();
    ComBytes b4(std::move(b2));
    TIME_COST_SHOW();

    Message msg2 = msg;
    TIME_COST_SHOW();
    Message msg3(msg);
    TIME_COST_SHOW();
    Message msg4 = std::move(msg);
    TIME_COST_SHOW();
    Message msg5(std::move(msg2));
    TIME_COST_SHOW();

    SingleInstanceProcess s("sss");
#if defined(_WIN32) || defined(_WIN64)
    com_run_shell("D:\\1.bat");
    std::string result = com_run_shell_with_output("dir");
    LOG_I("reault=%s", result.c_str());
#endif
    LOG_I("process name=%s", com_get_bin_name().c_str());
    LOG_I("process path=%s", com_get_bin_path().c_str());
}

void com_base_bytearray_unit_test_suit(void** state)
{
    ComBytes bytes((uint8*)"123456", 6);

    bytes.removeAt(1);
    ASSERT_STR_EQUAL(bytes.toString().c_str(), "13456");
    LOG_D("bytes=%s\n", bytes.toString().c_str());

    bytes.removeTail();
    ASSERT_STR_EQUAL(bytes.toString().c_str(), "1345");
    LOG_D("bytes=%s\n", bytes.toString().c_str());

    bytes.removeHead();
    ASSERT_STR_EQUAL(bytes.toString().c_str(), "345");
    LOG_D("bytes=%s\n", bytes.toString().c_str());

    ComBytes bytes1((uint8*)"123456", 6);
    bytes1 = bytearray_test(bytes1.getData(), bytes1.getDataSize());
}

void com_base_message_unit_test_suit(void** state)
{
    Message msg(1);
    msg.set("key0", true);
    msg.set("key0.1", false);
    msg.set("key1", 1);
    msg.set("key2", -1.1);
    msg.set("key3", "kkkks");
    //msg.set("key4", (void*)0x123456);
    msg.set("key5", (const uint8*)"123456", 6);

    ASSERT_TRUE(msg.getBool("key0"));
    ASSERT_FALSE(msg.getBool("key0.1"));
    ASSERT_STR_EQUAL(msg.getString("key3").c_str(), "kkkks");
    ASSERT_FLOAT_EQUAL(msg.getDouble("key2"), -1.1);
    //ASSERT_PTR_EQUAL(msg.getPtr("key4"), (void*)0x123456);

    LOG_D("msg=\n%s", msg.toJSON().c_str());
    msg = Message::FromJSON(msg.toJSON());

    msg.reset();
    ComBytes bytes;
    msg.set("bytes", bytes);
}

void com_base_json_unit_test_suit(void** state)
{
    std::map<std::string, std::string> m;
    m["a"] = "value_a";
    m["b"] = "value_b";
    m["c"] = "value_c";
    CJsonObject json;
    json.Add("isUpgrade", true ? "true" : "false");
    json.Add("isProtected", true ? "true" : "false");
    json.AddEmptySubObject("plugin");
    for(auto it = m.begin(); it != m.end(); it++)
    {
        json["plugin"].Add(it->first, it->second);
    }
    std::string str = json.ToFormattedString();
    //printf("str=%s\n", str.c_str());

    CJsonObject oJson("{\"refresh_interval\":60,"
                      "\"timeout\":12.5,"
                      "\"dynamic_loading\":["
                      "{"
                      "\"so_path\":\"plugins/User.so\", \"load\":false, \"version\":1,"
                      "\"cmd\":["
                      "{\"cmd\":2001, \"class\":\"neb::CmdUserLogin\"},"
                      "{\"cmd\":2003, \"class\":\"neb::CmdUserLogout\"}"
                      "],"
                      "\"module\":["
                      "{\"path\":\"im/user/login\", \"class\":\"neb::ModuleLogin\"},"
                      "{\"path\":\"im/user/logout\", \"class\":\"neb::ModuleLogout\"}"
                      "]"
                      "},"
                      "{"
                      "\"so_path\":\"plugins/ChatMsg.so\", \"load\":false, \"version\":1,"
                      "\"cmd\":["
                      "{\"cmd\":2001, \"class\":\"neb::CmdChat\"}"
                      "],"
                      "\"module\":[]"
                      "}"
                      "]"
                      "}");
    //printf("ojson=%s\n", oJson.ToString().c_str());
    float timeout = 0;
    oJson.Get("timeout", timeout);
    ASSERT_FLOAT_EQUAL(timeout, 12.5f);
    int version = 0;
    oJson["dynamic_loading"][0].Get("version", version);
    ASSERT_INT_EQUAL(version, 1);
    std::string so_path;
    oJson["dynamic_loading"][0].Get("so_path", so_path);
    ASSERT_STR_EQUAL(so_path.c_str(), "plugins/User.so");
    oJson["dynamic_loading"].Delete(0);
    //printf("ojson=%s\n", oJson.ToString().c_str());
    CJsonObject xjson;
    xjson.Add("name", "myname");
    xjson.Add("age", 18);
    xjson.AddEmptySubArray("data");
    CJsonObject params;
    ASSERT_TRUE(params.Add("addr", "address1"));
    ASSERT_TRUE(xjson["data"].Add(params));
    ASSERT_TRUE(params.Replace("addr", "address2"));
    ASSERT_TRUE(xjson["data"].Add(params));
    ASSERT_TRUE(params.Replace("addr", "address3"));
    ASSERT_TRUE(xjson["data"].Add(params));
    std::string address1;
    std::string address2;
    std::string address3;
    ASSERT_TRUE(xjson["data"][0].Get("addr", address1));
    ASSERT_TRUE(xjson["data"][1].Get("addr", address2));
    ASSERT_TRUE(xjson["data"][2].Get("addr", address3));
    ASSERT_STR_EQUAL(address1.c_str(), "address1");
    ASSERT_STR_EQUAL(address2.c_str(), "address2");
    ASSERT_STR_EQUAL(address3.c_str(), "address3");
}

#include <shared_mutex>
#if __linux__==1
#include <sys/resource.h>
#endif
template<class T>
void lock_test(T& m, int thread_count, int loop_count, int writer_ratio)
{
    uint64 x = 0;
    uint64 y = 0;
    std::list<std::thread> t;
    uint64 time_begin = com_time_cpu_ms();
#if __linux__==1
    struct rusage r_begin;
    getrusage(RUSAGE_SELF, &r_begin);
#endif
    for(int i = 0; i < thread_count; i++)
    {
        t.push_back(std::thread([&]()->void
        {
            for(int j = 0; j < loop_count; j++)
            {
                if((j % writer_ratio) == 0)
                {
                    m.lock();
                    x++;
                    y++;
                    m.unlock();
                }
                else
                {
                    m.lock_shared();
                    if(x != y)
                    {
                        LOG_E("x=%llu,y=%llu", x, y);
                        throw std::runtime_error("test failed");
                    }
                    m.unlock_shared();
                }
            }
        }));
    }

    for(auto& it : t)
    {
        it.join();
    }
    t.clear();
#if __linux__==1
    struct rusage r_end;
    getrusage(RUSAGE_SELF, &r_end);
    double ru = r_end.ru_utime.tv_sec + (double)r_end.ru_utime.tv_usec / 100000 - r_begin.ru_utime.tv_sec - (double)r_begin.ru_utime.tv_usec / 1000000;
    double rs = r_end.ru_stime.tv_sec + (double)r_end.ru_stime.tv_usec / 100000 - r_begin.ru_stime.tv_sec - (double)r_begin.ru_stime.tv_usec / 1000000;
#else
    double ru = -1;
    double rs = -1;
#endif
    printf("%-20s  %8d  %.4fs u:%.4f s:%.4f %10lld\n", typeid(m).name(), writer_ratio, (double)(com_time_cpu_ms() - time_begin) / 1000, ru, rs, x);
}

void com_base_lock_unit_test_suit(void** state)
{
    int thread_count = 32;
    int loop_count = 1 * 1024 * 1024;

    ComMutex m1;
    //std::shared_mutex m2;
    for(int i = 1; i < loop_count; i = i * 2)
    {
        lock_test(m1, thread_count, loop_count, i);
        //lock_test(m2, thread_count, loop_count, i);
        printf("\n");
    }
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
