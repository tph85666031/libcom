#include "bcp_com.h"

#if __linux__ == 1
std::string bcp_dns_query(const char* domain_name, const char* interface_name = NULL, const char* dns_server_ip = NULL);
#endif