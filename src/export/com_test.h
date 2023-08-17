#ifndef __COM_TEST_H__
#define __COM_TEST_H__

#include <setjmp.h>
#include <float.h>
#include "cmocka.h"

#define ASSERT_TRUE(c)                   assert_true((c))
#define ASSERT_FALSE(c)                  assert_false((c))
#define ASSERT_INT_EQUAL(a,b)            assert_int_equal((a),(b))
#define ASSERT_INT_NOT_EQUAL(a,b)        assert_int_not_equal((a),(b))
#define ASSERT_FLOAT_EQUAL(a,b)          assert_float_equal((a),(b),DBL_EPSILON)
#define ASSERT_FLOAT_NOT_EQUAL(a,b)      assert_float_not_equal((a),(b),DBL_EPSILON)
#define ASSERT_PTR_EQUAL(a,b)            assert_ptr_equal((a),(b))
#define ASSERT_PTR_NOT_EQUAL(a,b)        assert_ptr_not_equal((a),(b))
#define ASSERT_NULL(p)                   assert_null((p))
#define ASSERT_NOT_NULL(p)               assert_non_null((p))
#define ASSERT_MEM_EQUAL(a,b,s)          assert_memory_equal((a),(b),(s))
#define ASSERT_MEM_NOT_EQUAL(a,b,s)      assert_memory_not_equal((a),(b),(s))
#define ASSERT_IN_RANGE(a,min,max)       assert_in_range((a),(min),(max))
#define ASSERT_NOT_IN_RANGE(a,min,max)   assert_not_in_range((a),(min),(max))
#define ASSERT_IN_SET(a,ptr,count)       assert_in_set((a),(ptr),(count))
#define ASSERT_NOT_IN_SET(a,ptr,count)   assert_not_in_set((a),(ptr),(count))
#define ASSERT_STR_EQUAL(a,b)            assert_string_equal((a),(b))
#define ASSERT_STR_NOT_EQUAL(a,b)        assert_string_not_equal((a),(b))
#define ASSERT_SHOW_MESSAGE(fmt,...)     print_message(fmt "\n",##__VA_ARGS__)

#define TEST_PARAM_PUSH(fc,param)        will_return(fc,param)
#define TEST_PARAM_POP(type)             mock_type(type)
#define TEST_PARAM_POP_PTR(type)         mock_ptr_type(type)

#define TEST_EXPECT_SET(fc,arg,val)      expect_value(fc,arg,val)
#define TEST_EXPECT_CHECK(arg)           check_expected(arg)

#define TEST_FCALL_SET(fc)               expect_function_call(fc)
#define TEST_FCALL_SET_X(fc,count)       expect_function_calls(fc,count);
#define TEST_FCALL_MARK()                function_called()

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    const char* name;
    int case_count;
    struct CMUnitTest* cases;
} TEST_GROUP_DES;
#pragma pack(pop)

#define UNIT_TEST_LIB(test_cases) \
int main(void) \
{\
    com_log_init();\
    TEST_GROUP_DES cmocka_group = \
    { \
        "unit_test_",\
        sizeof(test_cases) / sizeof(test_cases[0]),\
        test_cases\
    };\
    int ret = _cmocka_run_group_tests(cmocka_group.name,\
                                      cmocka_group.cases,\
                                      cmocka_group.case_count,\
                                      NULL, NULL);\
    exit(ret);\
}

//为兼容Win，不采用GNU独有的constructor方式,因此单元测试不能涉及全局变量
#define UNIT_TEST_APP(test_cases) \
class __COM_LOAD_UT__ final\
{\
public:\
    __COM_LOAD_UT__()\
    {\
        com_log_init();\
        TEST_GROUP_DES cmocka_group = \
        {\
            "unit_test_", \
            sizeof(test_cases) / sizeof(test_cases[0]), \
            test_cases\
        };\
        int ret = _cmocka_run_group_tests(cmocka_group.name, \
                                          cmocka_group.cases, \
                                          cmocka_group.case_count, \
                                          NULL, NULL);\
        exit(ret);\
    }\
};\
static __COM_LOAD_UT__ __com_load_ut__;

#endif /* __COM_TEST_H__ */