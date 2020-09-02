#include "com.h"

extern void com_base_unit_test_suit(void** state);
extern void com_base_string_split_unit_test_suit(void** state);
extern void com_base_string_unit_test_suit(void** state);
extern void com_base_time_unit_test_suit(void** state);
extern void com_base_gps_unit_test_suit(void** state);
extern void com_base_bytearray_unit_test_suit(void** state);
extern void com_base_message_unit_test_suit(void** state);
extern void com_cache_unit_test_suit(void** state);
extern void com_thread_unit_test_suit(void** state);

extern void com_socket_unit_test_suit(void** state);
extern void com_sqlite3_unit_test_suit(void** state);
extern void com_config_unit_test_suit(void** state);
extern void com_log_unit_test_suit(void** state);
extern void com_serializer_unit_test_suit(void** state);
extern void com_base64_unit_test_suit(void** state);
extern void com_url_unit_test_suit(void** state);
extern void com_md5_unit_test_suit(void** state);
extern void com_task_unit_test_suit(void** state);
extern void com_file_unit_test_suit(void** state);

extern void com_unix_domain_unit_test_suit(void** state);
extern void com_ipc_ud_unit_test_suit(void** state);
extern void com_kvs_unit_test_suit(void** state);
extern void com_kv_unit_test_suit(void** state);
extern void com_crc_unit_test_suit(void** state);
extern void com_ipc_tcp_unit_test_suit(void** state);
extern void com_timer_unit_test_suit(void** state);

extern void com_nmea_unit_test_suit(void** state);
extern void com_mem_unit_test_suit(void** state);

extern void com_dns_unit_test_suit(void** state);

extern void com_base_xstring_unit_test(void** state);

CMUnitTest test_cases_com_lib[] =
{
#if 1
    cmocka_unit_test(com_base_unit_test_suit),
    cmocka_unit_test(com_base_string_split_unit_test_suit),
    cmocka_unit_test(com_base_string_unit_test_suit),
    cmocka_unit_test(com_base_time_unit_test_suit),
    cmocka_unit_test(com_base_gps_unit_test_suit),
    cmocka_unit_test(com_base_bytearray_unit_test_suit),
    cmocka_unit_test(com_base_message_unit_test_suit),
    cmocka_unit_test(com_base_xstring_unit_test),
    cmocka_unit_test(com_config_unit_test_suit),
    cmocka_unit_test(com_cache_unit_test_suit),
    cmocka_unit_test(com_thread_unit_test_suit),
    cmocka_unit_test(com_socket_unit_test_suit),
    cmocka_unit_test(com_unix_domain_unit_test_suit),
    cmocka_unit_test(com_sqlite3_unit_test_suit),
    cmocka_unit_test(com_serializer_unit_test_suit),
    cmocka_unit_test(com_base64_unit_test_suit),
    cmocka_unit_test(com_url_unit_test_suit),
    cmocka_unit_test(com_md5_unit_test_suit),
    cmocka_unit_test(com_task_unit_test_suit),
    cmocka_unit_test(com_file_unit_test_suit),
    cmocka_unit_test(com_ipc_ud_unit_test_suit),
    cmocka_unit_test(com_ipc_tcp_unit_test_suit),
    cmocka_unit_test(com_kv_unit_test_suit),
    cmocka_unit_test(com_kvs_unit_test_suit),
    cmocka_unit_test(com_crc_unit_test_suit),
    cmocka_unit_test(com_nmea_unit_test_suit),
    cmocka_unit_test(com_mem_unit_test_suit),
    cmocka_unit_test(com_dns_unit_test_suit),
#else
    cmocka_unit_test(com_ipc_tcp_unit_test_suit),
#endif
};

UNIT_TEST_LOAD(test_cases_com_lib);
