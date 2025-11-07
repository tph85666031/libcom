#ifndef __COM_DNS_H__
#define __COM_DNS_H__

#include "com_base.h"
#include "com_socket.h"

COM_EXPORT std::string com_dns_query(const char* domain_name, const char* interface_name = NULL, const char* dns_server_ip = NULL);
COM_EXPORT ComSocketAddr com_dns_resolve(const char* dns, bool ipv6_prefer = false);

#endif
