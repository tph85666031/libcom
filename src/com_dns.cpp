#if __linux__ == 1
#include <netdb.h>
#include "com_dns.h"
#include "com_socket.h"
#include "com_file.h"
#include "com_log.h"
#include "com_serializer.h"

#define DNS_PORT 53
static std::atomic<uint16> dns_id = {0};

std::vector<std::string> dns_get_server_list(const char* file_dns)
{
    if(file_dns == NULL)
    {
        file_dns = "/etc/resolv.conf";
    }

    std::vector<std::string> list;
    FILE* file = com_file_open(file_dns, "r");
    if(file != NULL)
    {
        char line[1024];
        while(com_file_readline(file, line, sizeof(line)))
        {
            if(!com_string_start_with(line, "nameserver"))
            {
                continue;
            }
            std::vector<std::string> vals = com_string_split(line, " ");
            if(vals.size() != 2)
            {
                continue;
            }
            if(com_string_is_ipv4(vals[1].c_str()))
            {
                list.push_back(vals[1]);
            }
        }
        com_file_close(file);
    }
    return list;
}

ComBytes dns_query_encode(const char* domain_name, uint16 id, bool is_tcp)
{
    //构造DNS数据
    Serializer s;
    s.append((uint16)id);
    s.append((uint16)0x0100);
    s.append((uint16)1);
    s.append((uint16)0);
    s.append((uint16)0);
    s.append((uint16)0);
    std::vector<std::string> name_parts = com_string_split(domain_name, ".");
    for(size_t i = 0; i < name_parts.size(); i++)
    {
        s.append((uint8)name_parts[i].size());
        s.append(name_parts[i]);
    }
    s.append((uint8)0);
    s.append((uint8)0x00);
    s.append((uint8)0x01);
    s.append((uint8)0x00);
    s.append((uint8)0x01);
    if(is_tcp == false)
    {
        return s.toBytes();
    }
    Serializer s2;
    s2.append((uint16)s.getDataSize());
    s2.append(s.getData(), s.getDataSize());
    return s2.toBytes();
}

std::string dns_query_decode(uint8* buf, int buf_size, uint16 id_expect, bool is_tcp)
{
    if(buf == NULL || buf_size <= 0)
    {
        return std::string();
    }
    //解析DNS数据
    Serializer s = Serializer(buf, buf_size);
    uint16 id = 0 ;
    uint16 flag = 0;
    uint16 question_count = 0;
    uint16 answer_count = 0;
    uint16 auth_count = 0;
    uint16 extra_count = 0;
    if(is_tcp)
    {
        uint16 size_rev = 0;
        s.detach(size_rev);//跳过长度字段
    }
    s.detach(id);
    s.detach(flag);
    s.detach(question_count);
    s.detach(answer_count);
    s.detach(auth_count);
    s.detach(extra_count);

    if(id_expect != id)
    {
        return std::string();
    }
    if(answer_count == 0)
    {
        return std::string();
    }

    for(int i = 0; i < question_count; i++)
    {
        uint8 tmp;
        while(true)
        {
            if(s.detach(tmp) < 0)
            {
                return std::string();
            }
            if(tmp == 0)
            {
                s.detach(tmp);
                s.detach(tmp);
                s.detach(tmp);
                s.detach(tmp);
                break;
            }
        }
    }

    for(int i = 0; i < answer_count; i++)
    {
        uint8 tmp;
        s.detach(tmp);
        if(tmp & 0xC0)  //pointer
        {
            uint8 offset = 0;
            s.detach(offset);
        }
        else
        {
            while(true)
            {
                if(s.detach(tmp) < 0)
                {
                    return std::string();
                }
                if(tmp == 0)
                {
                    break;
                }
            }
        }

        uint16 answer_type = 0;
        uint16 answer_class = 0;
        uint32 answer_ttl = 0;
        uint16 answer_size = 0;
        s.detach(answer_type);
        s.detach(answer_class);
        s.detach(answer_ttl);
        s.detach(answer_size);

        if(answer_size > s.getDetachRemainSize())
        {
            return std::string();
        }

        uint8 bytes_tmp[answer_size];
        s.detach(bytes_tmp, sizeof(bytes_tmp));

        if(answer_type == 1 && answer_size == 4)  //ip address
        {
            return com_string_format("%u.%u.%u.%u", bytes_tmp[0], bytes_tmp[1], bytes_tmp[2], bytes_tmp[3]);
        }
    }

    return std::string();
}

std::string dns_via_udp(const char* domain_name, const char* interface, const char* dns_server_ip)
{
    int socket_fd = com_socket_udp_open(interface, 10053, false);
    if(socket_fd <= 0)
    {
        LOG_E("udp open failed");
        return std::string();
    }

    uint16 id = dns_id++;
    ComBytes bytes = dns_query_encode(domain_name, id, false);
    int ret = com_socket_udp_send(socket_fd, dns_server_ip, DNS_PORT, bytes.getData(), bytes.getDataSize());
    if(ret <= 0)
    {
        com_socket_close(socket_fd);
        return std::string();
    }

    uint8 buf[1024];
    ret = com_socket_udp_read(socket_fd, buf, sizeof(buf), 5 * 1000, NULL, 0);
    if(ret <= 0)
    {
        com_socket_close(socket_fd);
        return std::string();
    }
    com_socket_close(socket_fd);

    return dns_query_decode(buf, ret, id, false);
}

std::string dns_via_tcp(const char* domain_name, const char* interface, const char* dns_server_ip)
{
    LOG_E("called");
    int socket_fd = com_socket_tcp_open(dns_server_ip, DNS_PORT, 5 * 1000, interface);
    if(socket_fd <= 0)
    {
        LOG_E("tcp open failed");
        return std::string();
    }

    uint16 id = dns_id++;
    ComBytes bytes = dns_query_encode(domain_name, id, true);
    int ret = com_socket_tcp_send(socket_fd, bytes.getData(), bytes.getDataSize());
    if(ret <= 0)
    {
        com_socket_close(socket_fd);
        return std::string();
    }

    uint8 buf[1024];
    ret = com_socket_tcp_read(socket_fd, buf, sizeof(buf), 5 * 1000, false);
    if(ret <= 0)
    {
        com_socket_close(socket_fd);
        return std::string();
    }
    com_socket_close(socket_fd);

    return dns_query_decode(buf, ret, id, true);
}

std::string com_dns_query(const char* domain_name, const char* interface_name, const char* dns_server_ip)
{
    if(domain_name == NULL)
    {
        LOG_E("domain_name is NULL");
        return std::string();
    }

    if(com_string_len(interface_name) <= 0)
    {
        addrinfo hints;
        addrinfo* res = NULL;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = AF_INET;
        if(getaddrinfo(domain_name, NULL, &hints, &res) != 0)
        {
            return std::string();
        }
        uint32 ip = ntohl(((sockaddr_in*)(res->ai_addr))->sin_addr.s_addr);
        freeaddrinfo(res);
        return com_string_format("%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
                                 (ip >> 8) & 0xFF, ip & 0xFF);
    }

    std::vector<std::string> dns_server_list;
    if(dns_server_ip == NULL)
    {
        dns_server_list = dns_get_server_list(NULL);
    }
    else if(com_string_is_ipv4(dns_server_ip))
    {
        dns_server_list.push_back(dns_server_ip);
    }
    else
    {
        dns_server_list.push_back("114.114.114.114");
        dns_server_list.push_back("223.5.5.5");
        dns_server_list.push_back("8.8.8.8");
    }

    ComNicInfo nic;
    if(com_net_get_nic(interface_name, nic) == false)
    {
        LOG_E("failed to get %s info", interface_name);
        return std::string();
    }
    LOG_I("ip=%s for %s", nic.ip.c_str(), nic.name.c_str());

    bool rp_set = false;
    uint8 rpfilter_flag = com_net_get_rpfilter(interface_name);
    if(rpfilter_flag != 2)
    {
        com_net_set_rpfilter(interface_name, 2);
        rp_set = true;
    }
    std::string ip;
    for(size_t i = 0; i < dns_server_list.size() && ip.empty(); i++)
    {
        ip = dns_via_udp(domain_name, interface_name, dns_server_list[i].c_str());
    }

    if(ip.empty())
    {
        for(size_t i = 0; i < dns_server_list.size() && ip.empty(); i++)
        {
            ip = dns_via_tcp(domain_name, interface_name, dns_server_list[i].c_str());
        }
    }
    if(rp_set)
    {
        com_net_set_rpfilter(interface_name, rpfilter_flag);
    }

    LOG_I("ip_pri=%s, ip_pub=%s for %s", nic.ip.c_str(), ip.c_str(), nic.name.c_str());
    return ip;
}
#endif