#include "com_dns.h"
#include "com_socket.h"
#include "com_log.h"
#include "com_test.h"

#if __linux__ == 1
void com_dns_unit_test_suit(void** state)
{
    std::string ip;
    std::vector<std::string> interfaces = com_net_get_interface_all();
    for (size_t i = 0; i < interfaces.size(); i++)
    {
        if (com_string_equal(interfaces[i].c_str(), "lo")
                || com_string_equal(interfaces[i].c_str(), "br0"))
        {
            continue;
        }
        ip = com_dns_query("www.163.com", interfaces[i].c_str(), "114.114.114.114");
        ASSERT_FALSE(ip.empty());
    }

    ip = com_dns_query("www.baidu.com");
    ASSERT_FALSE(ip.empty());

    ip = com_dns_query("www.google.com");
    ASSERT_FALSE(ip.empty());

    ASSERT_TRUE(com_string_is_ip("127.23.247.6"));
    ASSERT_FALSE(com_string_is_ip("127.0247.4467.6"));
}
#endif

