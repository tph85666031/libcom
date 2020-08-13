#include "bcp.h"

extern void bcp_com_unit_test_suit(void** state);
extern void bcp_com_string_split_unit_test_suit(void** state);
extern void bcp_com_string_unit_test_suit(void** state);
extern void bcp_com_time_unit_test_suit(void** state);
extern void bcp_com_gps_unit_test_suit(void** state);
extern void bcp_com_bytearray_unit_test_suit(void** state);
extern void bcp_com_message_unit_test_suit(void** state);
extern void bcp_cache_unit_test_suit(void** state);
extern void bcp_thread_unit_test_suit(void** state);

extern void bcp_socket_unit_test_suit(void** state);
extern void bcp_sqlite3_unit_test_suit(void** state);
extern void bcp_config_unit_test_suit(void** state);
extern void bcp_log_unit_test_suit(void** state);
extern void bcp_serializer_unit_test_suit(void** state);
extern void bcp_base64_unit_test_suit(void** state);
extern void bcp_url_unit_test_suit(void** state);
extern void bcp_md5_unit_test_suit(void** state);
extern void bcp_task_unit_test_suit(void** state);
extern void bcp_file_unit_test_suit(void** state);

extern void bcp_unix_domain_unit_test_suit(void** state);
extern void bcp_ipc_ud_unit_test_suit(void** state);
extern void bcp_kvs_unit_test_suit(void** state);
extern void bcp_kv_unit_test_suit(void** state);
extern void bcp_crc_unit_test_suit(void** state);
extern void bcp_ipc_tcp_unit_test_suit(void** state);
extern void bcp_timer_unit_test_suit(void** state);

extern void bcp_nmea_unit_test_suit(void** state);
extern void bcp_mem_unit_test_suit(void** state);

extern void bcp_dns_unit_test_suit(void** state);

extern void bcp_com_xstring_unit_test(void** state);

CMUnitTest test_cases_bcp_lib[] =
{
#if 1
    cmocka_unit_test(bcp_com_unit_test_suit),
    cmocka_unit_test(bcp_com_string_split_unit_test_suit),
    cmocka_unit_test(bcp_com_string_unit_test_suit),
    cmocka_unit_test(bcp_com_time_unit_test_suit),
    cmocka_unit_test(bcp_com_gps_unit_test_suit),
    cmocka_unit_test(bcp_com_bytearray_unit_test_suit),
    cmocka_unit_test(bcp_com_message_unit_test_suit),
    cmocka_unit_test(bcp_com_xstring_unit_test),
    cmocka_unit_test(bcp_config_unit_test_suit),
    cmocka_unit_test(bcp_cache_unit_test_suit),
    cmocka_unit_test(bcp_thread_unit_test_suit),
    cmocka_unit_test(bcp_socket_unit_test_suit),
    cmocka_unit_test(bcp_unix_domain_unit_test_suit),
    cmocka_unit_test(bcp_sqlite3_unit_test_suit),
    cmocka_unit_test(bcp_serializer_unit_test_suit),
    cmocka_unit_test(bcp_base64_unit_test_suit),
    cmocka_unit_test(bcp_url_unit_test_suit),
    cmocka_unit_test(bcp_md5_unit_test_suit),
    cmocka_unit_test(bcp_task_unit_test_suit),
    cmocka_unit_test(bcp_file_unit_test_suit),
    cmocka_unit_test(bcp_ipc_ud_unit_test_suit),
    cmocka_unit_test(bcp_ipc_tcp_unit_test_suit),
    cmocka_unit_test(bcp_kv_unit_test_suit),
    cmocka_unit_test(bcp_kvs_unit_test_suit),
    cmocka_unit_test(bcp_crc_unit_test_suit),
    cmocka_unit_test(bcp_nmea_unit_test_suit),
    cmocka_unit_test(bcp_mem_unit_test_suit),
    cmocka_unit_test(bcp_dns_unit_test_suit),
#else
    cmocka_unit_test(bcp_ipc_tcp_unit_test_suit),
#endif
};

UNIT_TEST_LOAD(test_cases_bcp_lib);