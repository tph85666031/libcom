#include "com.h"

extern void com_base_unit_test_suit(void** state);
extern void com_base_string_split_unit_test_suit(void** state);
extern void com_base_string_unit_test_suit(void** state);
extern void com_base_time_unit_test_suit(void** state);
extern void com_base_gps_unit_test_suit(void** state);
extern void com_base_bytearray_unit_test_suit(void** state);
extern void com_base_message_unit_test_suit(void** state);
extern void com_base_json_unit_test_suit(void** state);
extern void com_base_lock_unit_test_suit(void** state);
extern void com_base_option_unit_test_suit(void** state);
extern void com_cache_unit_test_suit(void** state);
extern void com_thread_unit_test_suit(void** state);
extern void com_socket_unit_test_suit(void** state);
extern void com_sqlite_unit_test_suit(void** state);
extern void com_config_unit_test_suit(void** state);
extern void com_log_unit_test_suit(void** state);
extern void com_serializer_unit_test_suit(void** state);
extern void com_base64_unit_test_suit(void** state);
extern void com_url_unit_test_suit(void** state);
extern void com_md5_unit_test_suit(void** state);
extern void com_task_unit_test_suit(void** state);
extern void com_file_unit_test_suit(void** state);
extern void com_file_lock_unit_test_suit(void** state);
extern void com_unix_domain_unit_test_suit(void** state);
extern void com_kv_unit_test_suit(void** state);
extern void com_crc_unit_test_suit(void** state);
extern void com_timer_unit_test_suit(void** state);
extern void com_mem_unit_test_suit(void** state);
extern void com_dns_unit_test_suit(void** state);
extern void com_auto_test_unit_test_suit(void** state);
extern void com_http_unit_test_suit(void** state);
extern void com_socket_multicast_unit_test_suit(void** state);
extern void com_xml_unit_test_suit(void** state);
extern void com_plist_unit_test_suit(void** state);
extern void com_sync_unit_test_suit(void** state);
extern void com_itp_log_unit_test_suit(void** state);
extern void com_socket_stringipc_unit_test_suit(void** state);
extern void com_netlink_unit_test_suit(void** state);

CMUnitTest test_cases_com_lib[] =
{
#if 0
    cmocka_unit_test(com_base_unit_test_suit),
    cmocka_unit_test(com_base_string_split_unit_test_suit),
    cmocka_unit_test(com_base_string_unit_test_suit),
    cmocka_unit_test(com_base_json_unit_test_suit),
    cmocka_unit_test(com_base_time_unit_test_suit),
    cmocka_unit_test(com_base_gps_unit_test_suit),
    cmocka_unit_test(com_base_bytearray_unit_test_suit),
    cmocka_unit_test(com_base_message_unit_test_suit),
    cmocka_unit_test(com_config_unit_test_suit),
    cmocka_unit_test(com_thread_unit_test_suit),
    cmocka_unit_test(com_socket_unit_test_suit),
    cmocka_unit_test(com_unix_domain_unit_test_suit),
    cmocka_unit_test(com_sqlite_unit_test_suit),
    cmocka_unit_test(com_serializer_unit_test_suit),
    cmocka_unit_test(com_base64_unit_test_suit),
    cmocka_unit_test(com_url_unit_test_suit),
    cmocka_unit_test(com_md5_unit_test_suit),
    cmocka_unit_test(com_task_unit_test_suit),
    cmocka_unit_test(com_file_unit_test_suit),
    cmocka_unit_test(com_kv_unit_test_suit),
    cmocka_unit_test(com_crc_unit_test_suit),
    cmocka_unit_test(com_mem_unit_test_suit),
    cmocka_unit_test(com_dns_unit_test_suit),
    cmocka_unit_test(com_auto_test_unit_test_suit),
    cmocka_unit_test(com_socket_multicast_unit_test_suit),
    cmocka_unit_test(com_xml_unit_test_suit),
    cmocka_unit_test(com_plist_unit_test_suit),
    cmocka_unit_test(com_sync_unit_test_suit),
    cmocka_unit_test(com_socket_stringipc_unit_test_suit)
#else
    cmocka_unit_test(com_netlink_unit_test_suit)
#endif
};

UNIT_TEST_LIB(test_cases_com_lib);

