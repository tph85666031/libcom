#include "bcp_dns.h"
#include "bcp_socket.h"
#include "bcp_log.h"
#include "bcp_test.h"

void bcp_dns_unit_test_suit(void** state)
{
    std::string ip;
    std::vector<std::string> interfaces = bcp_net_get_interface_all();
    for (int i = 0; i < interfaces.size(); i++)
    {
        if (bcp_string_equal(interfaces[i].c_str(), "lo")
                || bcp_string_equal(interfaces[i].c_str(), "br0"))
        {
            continue;
        }
        ip = bcp_dns_query("www.163.com", interfaces[i].c_str(), "114.114.114.114");
        ASSERT_FALSE(ip.empty());
    }

    ip = bcp_dns_query("www.baidu.com");
    ASSERT_FALSE(ip.empty());

    ip = bcp_dns_query("www.google.com");
    ASSERT_FALSE(ip.empty());

    ASSERT_TRUE(bcp_string_is_ip("127.23.247.6"));
    ASSERT_FALSE(bcp_string_is_ip("127.0247.4467.6"));
}

