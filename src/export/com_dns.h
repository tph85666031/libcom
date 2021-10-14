#ifndef __COM_DNS_H__
#define __COM_DNS_H__
#include "com_base.h"

#if __linux__ == 1
std::string com_dns_query(const char* domain_name, const char* interface_name = NULL, const char* dns_server_ip = NULL);
#endif
#endif
