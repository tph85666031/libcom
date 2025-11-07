#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define SOCK_CLOEXEC 0
#define MSG_DONTWAIT 0
#elif defined(__APPLE__)
#include <netinet/tcp_fsm.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/un.h>
#elif __linux__ == 1
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/un.h>
#endif

#include "com_socket.h"
#include "com_dns.h"

void com_socket_global_init()
{
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(0x0002, &wsaData);
#endif
}

void com_socket_global_uninit()
{
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

//对connect同样适用
void com_socket_set_recv_timeout(int sock, int timeout_ms)
{
    int ret;
#if defined(_WIN32) || defined(_WIN64)
    int timeout = timeout_ms;
#else
    struct timeval timeout = {timeout_ms / 1000, timeout_ms % 1000};
#endif
    ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                     (const char*)&timeout, sizeof(timeout));
    if(ret < 0)
    {
        LOG_E("socket set recv timeout failed");
    }
}

void com_socket_set_send_timeout(int sock, int timeout_ms)
{
    int ret;
#if defined(_WIN32) || defined(_WIN64)
    int timeout = timeout_ms;
#else
    struct timeval timeout = {timeout_ms / 1000, timeout_ms % 1000};
#endif
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
                     (const char*)&timeout, sizeof(timeout));
    if(ret < 0)
    {
        LOG_E("socket set send timeout failed");
    }
}

/*
    enum {
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT = 2,
    TCP_SYN_RECV = 3,
    TCP_FIN_WAIT1 = 4,
    TCP_FIN_WAIT2 = 5,
    TCP_TIME_WAIT = 6,
    TCP_CLOSE = 7,
    TCP_CLOSE_WAIT = 8,
    TCP_LAST_ACK = 9,
    TCP_LISTEN = 10,
    TCP_CLOSING = 11,    //Now a valid state
    TCP_NEW_SYN_RECV = 12,
    TCP_MAX_STATES  //Leave at the end!
    };
*/
int com_socket_get_tcp_connection_status(int sock)
{
    if(sock <= 0)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    int optval = -1;
    int optlen = sizeof(int);
    int ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*) &optval, &optlen);
    if(ret != 0)
    {
        LOG_D("failed to get tcp socket status, ret=%d", ret);
        return -1;
    }
    if(optval != 0)
    {
        return 0;
    }
#elif defined(TCP_INFO)
    struct tcp_info info;
    int len = sizeof(info);
    int ret = getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len);
    if(ret != 0)
    {
        LOG_D("failed to get tcp socket status, ret=%d,errno=%d:%s", ret, errno, strerror(errno));
        return -1;
    }
    if(info.tcpi_state != TCP_ESTABLISHED)
    {
        LOG_D("tcp socket status=%d", info.tcpi_state);
        return 0;
    }
#elif defined(TCP_CONNECTION_INFO)
    struct tcp_connection_info info;
    int len = sizeof(info);
    int ret = getsockopt(sock, IPPROTO_TCP, TCP_CONNECTION_INFO, (void*)&info, (socklen_t*)&len);
    if(ret != 0)
    {
        LOG_D("failed to get tcp socket status, ret=%d,errno=%d:%s", ret, errno, strerror(errno));
        return -1;
    }
    if(info.tcpi_state != TCPS_ESTABLISHED)
    {
        LOG_D("tcp socket status=%d", info.tcpi_state);
        return 0;
    }
#endif
    return 1; // tcp established
}

int com_socket_udp_open(const char* interface_name, uint32_be bind_ipv4, uint8* bind_ipv6, uint16 bind_port, bool broadcast)
{
    int socketfd = -1;
#if defined(SOCK_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_IP);
#elif defined(O_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(socketfd > 0)
    {
        fcntl(socketfd, O_CLOEXEC);
    }
#endif
    if(socketfd < 0)
    {
        LOG_E("failed to open socket");
        return -1;
    }
    com_socket_set_send_timeout(socketfd, 10 * 1000);
    com_socket_set_recv_timeout(socketfd, 10 * 1000);

    ComNicInfo nic;

#if __linux__ == 1
    if(com_string_length(interface_name) > 0)  //绑定到指定网卡，忽略路由
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, interface_name, sizeof(ifr.ifr_name) - 1);
        setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }
#endif

    if(broadcast)
    {
#if defined(_WIN32) || defined(_WIN64)
        char option = 0x01;
#else
        int option = 0x01;
#endif
        bind_port = 0;
        socklen_t option_len = sizeof(option);
        setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &option, option_len);
    }

    if(bind_ipv4 != 0 || bind_ipv6 != NULL)
    {
        struct sockaddr_storage bind_addr;
        memset(&bind_addr, 0, sizeof(struct sockaddr_storage));
        if(bind_ipv4 != 0)
        {
            struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)&bind_addr;
            addr_ipv4->sin_family = AF_INET;
            addr_ipv4->sin_port = htons(bind_port);
            addr_ipv4->sin_addr.s_addr = bind_ipv4;
        }
        else
        {
            struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)&bind_addr;
            addr_ipv6->sin6_family = AF_INET6;
            addr_ipv6->sin6_port = htons(bind_port);
            memcpy(addr_ipv6->sin6_addr.s6_addr, bind_ipv6, sizeof(addr_ipv6->sin6_addr.s6_addr));
        }

        if(bind(socketfd, (struct sockaddr*)&bind_addr, bind_addr.ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)) < 0)
        {
            LOG_E("bind failed,errno=%d:%s", errno, strerror(errno));
            com_socket_close(socketfd);
            return false;
        }
    }

    return socketfd;
}

int com_socket_unix_domain_open(const char* my_name, const char* server_name)
{
#if defined(_WIN32) || defined(_WIN64)
    return -1;
#else
    if(server_name == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    int socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(socketfd < 0)
    {
        LOG_E("failed to create unix domain socket");
        return -1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, server_name);

    if(NULL != my_name && strlen(my_name) > 0)
    {
        com_file_remove(my_name);
        struct sockaddr_un client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sun_family = AF_UNIX;
        strcpy(client_addr.sun_path, my_name);
        if(bind(socketfd, (struct sockaddr*)(&client_addr), sizeof(client_addr)) < 0)
        {
            LOG_E("bind failed, %s", strerror(errno));
            com_socket_close(socketfd);
            return -1;
        }
    }
    if(connect(socketfd, (struct sockaddr*)(&server_addr), sizeof(server_addr)) != 0)
    {
        LOG_E("connect failed, error: %s", strerror(errno));
        com_socket_close(socketfd);
        return -1;
    }
    return socketfd;
#endif
}

int com_socket_tcp_open(const char* remote_host, uint16 remote_port, uint32 timeout_ms,
                        uint32_be bind_ipv4, uint8* bind_ipv6,
                        uint16 bind_port, const char* bind_interface)
{
    if(remote_host == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    ComSocketAddr addr = com_dns_resolve(remote_host);

    int socketfd = -1;
#if defined(SOCK_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_IP);
#elif defined(O_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(socketfd > 0)
    {
        fcntl(socketfd, O_CLOEXEC);
    }
#endif
    if(socketfd < 0)
    {
        LOG_E("socketfd fail");
        return -1;
    }

#if __linux__ == 1
    if(com_string_length(bind_interface) > 0)  //绑定到指定网卡，忽略路由
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, bind_interface, sizeof(ifr.ifr_name) - 1);
        setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }
#endif

    if(bind_ipv4 != 0 || bind_ipv6 != NULL)
    {
        struct sockaddr_storage bind_addr;
        memset(&bind_addr, 0, sizeof(struct sockaddr_storage));
        if(bind_ipv4 != 0)
        {
            struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)&bind_addr;
            addr_ipv4->sin_family = AF_INET;
            addr_ipv4->sin_port = htons(bind_port);
            addr_ipv4->sin_addr.s_addr = bind_ipv4;
        }
        else
        {
            struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)&bind_addr;
            addr_ipv6->sin6_port = htons(bind_port);
            addr_ipv6->sin6_family = AF_INET6;
            memcpy(addr_ipv6->sin6_addr.s6_addr, bind_ipv6, sizeof(addr_ipv6->sin6_addr.s6_addr));
        }

        if(bind(socketfd, (struct sockaddr*)&bind_addr, bind_addr.ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)) < 0)
        {
            LOG_E("bind failed,errno=%d:%s,host=%s:%d", errno, strerror(errno), remote_host, remote_port);
            com_socket_close(socketfd);
            return false;
        }
    }

    com_socket_set_recv_timeout(socketfd, timeout_ms);
    com_socket_set_send_timeout(socketfd, 10 * 1000);

    if(-1 == connect(socketfd, (struct sockaddr*)(addr.toSockaddrStorage()), addr.is_ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)))
    {
        LOG_E("connect fail,errno=%d:%s,host=%s:%d", errno, strerror(errno), remote_host, remote_port);
        com_socket_close(socketfd);
        return -1;
    }
    com_socket_set_recv_timeout(socketfd, 0);
    return socketfd;
}

int com_socket_udp_send(int socketfd, const char* remote_host, int remote_port,
                        const void* data, int data_size)
{
    if(remote_host == NULL || data == NULL || data_size <= 0)
    {
        return -1;
    }
    ComSocketAddr addr_to = com_dns_resolve(remote_host);
    addr_to.port = remote_port;
    return sendto(socketfd, (char*)data, data_size, 0,
                  (struct sockaddr*)addr_to.toSockaddrStorage(),
                  addr_to.is_ipv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in));
}

int com_socket_tcp_send(int socketfd, const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return -2;
    }
    if(socketfd <= 0)
    {
        LOG_E("socket is incorrect,socketfd=%d", socketfd);
        return -3;
    }
    return send(socketfd, (char*)data, data_size, 0);
}

int com_socket_udp_read(int socketfd, uint8* data, int data_size,
                        uint32 timeout_ms, char* remote_ip, int sender_ip_size)
{
    if(socketfd <= 0 || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    if(timeout_ms != 0)
    {
        struct timeval tv;
        fd_set readfds;
        int select_rtn;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        FD_ZERO(&readfds);
        FD_SET(socketfd, &readfds);
        select_rtn = select(socketfd + 1, &readfds, NULL, NULL, &tv);
        if(select_rtn == -1)
        {
            LOG_E("failed");
            return -1;
        }
        if(select_rtn == 0)
        {
            LOG_D("timeout");
            return -2;
        }
        if(FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("no data");
            ((char*)data)[0] = '\0';
            return 0;
        }
    }
    int addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    int len = recvfrom(socketfd, (char*)data, data_size, 0,
                       (struct sockaddr*)&addr, (socklen_t*)&addr_len);
    if(remote_ip && sender_ip_size > 0)
    {
        uint32 ip_addr = ntohl(addr.sin_addr.s_addr);
        com_snprintf(remote_ip, sender_ip_size, "%d.%d.%d.%d",
                     (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
                     (ip_addr >> 8) & 0xFF, ip_addr & 0xFF);
    }
    return len;
}

//如果此函数返回值<=0则需要主动检查下连接是否已断开了(-2代表超时,可以不用检查连接)
int com_socket_tcp_read(int socketfd, uint8* data, int data_size,
                        uint32 timeout_ms, bool block_mode)
{
    if(socketfd <= 0 || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return -3;
    }
    if(timeout_ms > 0)
    {
        block_mode = false;
        struct timeval tv;
        fd_set readfds;
        int select_rtn;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        FD_ZERO(&readfds);
        FD_SET(socketfd, &readfds);
        select_rtn = select(socketfd + 1, &readfds, NULL, NULL, &tv);
        //如果对端关闭了连接,select的返回值可能为1,且errno为2
        if(select_rtn < 0)
        {
            LOG_W("select() failed,ret=%d,errno=%d:%s", select_rtn, errno, strerror(errno));
            return -1;
        }
        if(select_rtn == 0)
        {
            LOG_T("select() timeout");
            return -2;
        }
        if(FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("select() no data");
            ((char*)data)[0] = '\0';
            return 0;
        }
    }
    //如果对端关闭了连接,recv会返回0
    return recv(socketfd, (char*)data, data_size, block_mode ? 0 : MSG_DONTWAIT);
}

void com_socket_close(int socketfd)
{
    if(socketfd > 0)
    {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(socketfd);
#else
        close(socketfd);
#endif
    }
}

uint8 com_net_get_rpfilter(const char* interface_name)
{
#if __linux__ == 1
    if(interface_name == NULL)
    {
        return 0xFF;
    }
    ComBytes bytes = com_file_readall(com_string_format("/proc/sys/net/ipv4/conf/%s/rp_filter", interface_name).c_str());
    uint8 flag = (uint8)strtoul(bytes.toString().c_str(), NULL, 10);
    if(flag != 0 && flag != 1 && flag != 2)
    {
        return 0xFF;
    }
    return flag;
#else
    return 0xFF;
#endif
}

//反向原地址可达性校验，0=不校验，1=严格校验，2=宽松校验
void com_net_set_rpfilter(const char* interface_name, uint8 flag)
{
#if __linux__ == 1
    if(interface_name != NULL && (flag == 0 || flag == 1 || flag == 2))
    {
        com_run_shell("echo %d > /proc/sys/net/ipv4/conf/%s/rp_filter", flag, interface_name);
    }
#endif
}

bool com_net_is_interface_exist(const char* interface_name)
{
#if __linux__ == 1
    if(interface_name == NULL)
    {
        return false;
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        return false;
    }

    char buf[8192] = {0};
    struct ifconf ifc;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
    {
        close(sockfd);
        return false;
    }
    struct ifreq* p_ifr = ifc.ifc_req;
    for(int i = ifc.ifc_len / sizeof(struct ifreq); i >= 0; p_ifr++, i--)
    {
        if(com_string_equal(interface_name, p_ifr->ifr_name))
        {
            close(sockfd);
            return true;
        }
    }

    close(sockfd);
    return false;
#else
    return false;
#endif
}

std::vector<std::string> com_net_get_interface_all()
{
    std::vector<std::string> list;
#if __linux__ == 1
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        return list;
    }
    char buff[8192] = {0};
    struct ifconf ifc;
    ifc.ifc_len = sizeof(buff);
    ifc.ifc_buf = buff;
    if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) //网卡没有分配IP则获取不到
    {
        close(sockfd);
        return list;
    }
    struct ifreq* p_ifr = ifc.ifc_req;
    for(int i = ifc.ifc_len / sizeof(struct ifreq); i >= 0; p_ifr++, i--)
    {
        if(com_string_length(p_ifr->ifr_name) > 0)
        {
            list.push_back(p_ifr->ifr_name);
            LOG_D("int=%s", p_ifr->ifr_name);
        }
    }

    close(sockfd);
#endif
    return list;
}

bool com_net_get_nic(const char* interface_name, ComNicInfo& nic)
{
#if __linux__ == 1
    if(com_net_is_interface_exist(interface_name) == false)
    {
        return false;
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        return false;
    }

    nic.name = interface_name;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if(ioctl(sockfd, SIOCGIFADDR, &ifr) == 0)
    {
        struct sockaddr_in sin;
        memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
        nic.ip = inet_ntoa(sin.sin_addr);
    }
    if(ioctl(sockfd, SIOCGIFNETMASK, &ifr) == 0)
    {
        struct sockaddr_in sin;
        memcpy(&sin, &ifr.ifr_netmask, sizeof(sin));
        nic.ip_mask = inet_ntoa(sin.sin_addr);
    }
    if(ioctl(sockfd, SIOCGIFBRDADDR, &ifr) == 0)
    {
        struct sockaddr_in sin;
        memcpy(&sin, &ifr.ifr_broadaddr, sizeof(sin));
        nic.ip_broadcast = inet_ntoa(sin.sin_addr);
    }
    if(ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0)
    {
        nic.addr_family = ifr.ifr_addr.sa_family;
        memcpy(nic.mac, ifr.ifr_hwaddr.sa_data, sizeof(nic.mac));
    }
    if(ioctl(sockfd, SIOCGIFFLAGS, &ifr) == 0)
    {
        if(ifr.ifr_flags & IFF_UP)
        {
            nic.flags |= 0x00000002;
        }

        if(ifr.ifr_flags & IFF_RUNNING)
        {
            nic.flags |= 0x00000004;
        }

        if(ifr.ifr_flags & IFF_LOOPBACK)
        {
            nic.flags |= 0x00000001;
        }
    }
    close(sockfd);
    return true;
#else
    return false;
#endif
}

bool com_net_get_mac(const char* interface_name, uint8* mac)
{
#if __linux__ == 1
    if(interface_name == NULL || mac == NULL)
    {
        LOG_E("arg incorrect, interface_name=%s,mac=%p", interface_name, mac);
        return false;
    }
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof(struct ifreq));
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        LOG_E("socket failed");
        return false;
    }
    strncpy(ifreq.ifr_name, interface_name, sizeof(ifreq.ifr_name) - 1);  //Currently, only get eth0

    int ret = ioctl(sockfd, SIOCGIFHWADDR, &ifreq);
    if(ret < 0)
    {
        close(sockfd);
        LOG_E("ioctl failed for %s, ret=%d, errno=%d", interface_name, ret, errno);
        return false;
    }
    memcpy(mac, ifreq.ifr_hwaddr.sa_data, LENGTH_MAC);
    close(sockfd);
    return true;
#else
    return false;
#endif
}

ComNicInfo::ComNicInfo()
{
    flags = 0;
    addr_family = 0;
    memset(mac, 0, sizeof(mac));
}

ComNicInfo::~ComNicInfo()
{
}

void* ComSocketAddr::toSockaddrStorage()
{
    if(!valid())
    {
        return NULL;
    }
    if(is_ipv6)
    {
        struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)buf;
        addr_ipv6->sin6_family = AF_INET6;
        addr_ipv6->sin6_port = htons(port);
        memcpy(addr_ipv6->sin6_addr.s6_addr, ipv6, sizeof(ipv6));
    }
    else
    {
        struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)buf;
        addr_ipv4->sin_family = AF_INET;
        addr_ipv4->sin_port = htons(port);
        addr_ipv4->sin_addr.s_addr = ipv4;
    }

    return buf;
}

bool ComSocketAddr::valid()
{
    if(is_ipv6)
    {
        uint8 buf_empty[16];
        memset(buf_empty, 0, sizeof(buf_empty));
        return (memcpy(ipv6, buf_empty, sizeof(ipv6)) == 0);
    }
    else
    {
        return ipv4 != 0;
    }
}

ComSocket::ComSocket()
{
    port = 0;
    socketfd = 0;
    com_socket_global_init();
}

ComSocket::~ComSocket()
{
    closeSocket();
}

void ComSocket::setHost(const char* host)
{
    if(host != NULL)
    {
        this->host = host;
    }
}

void ComSocket::setPort(uint16 port)
{
    this->port = port;
}

void ComSocket::setInterface(const char* interface_name)
{
    if(interface_name != NULL)
    {
        this->interface_name = interface_name;
    }
}

void ComSocket::setBindIPv4(uint32_be ipv4)
{
    this->bind_ipv4 = ipv4;
}

void ComSocket::setBindIPv6(uint8* ipv6)
{
    if(ipv6 != NULL)
    {
        memcpy(this->bind_ipv6, ipv6, sizeof(this->bind_ipv6));
    }
}

void ComSocket::setBindPort(uint16 port)
{
    this->bind_port = port;
}

void ComSocket::closeSocket()
{
    if(socketfd > 0)
    {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(socketfd);
#else
        close(socketfd);
#endif
    }
    socketfd = -1;
}

ComSocketTcp::ComSocketTcp()
{
}

ComSocketTcp::~ComSocketTcp()
{
}

bool ComSocketTcp::openSocket()
{
    ComSocketAddr addr = com_dns_resolve(host.c_str());
    addr.port = port;
    if(addr.valid() == false)
    {
        LOG_E("failed to resolve host to ip,host=%s", host.c_str());
        return false;
    }

#if defined(SOCK_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_IP);
#elif defined(O_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(socketfd > 0)
    {
        fcntl(socketfd, O_CLOEXEC);
    }
#endif
    if(socketfd < 0)
    {
        LOG_E("failed to create socket,host=%s:%u", host.c_str(), port);
        return false;
    }

#if __linux__ == 1
    if(!interface_name.empty()) //绑定到指定网卡，忽略路由
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, interface_name.c_str(), sizeof(ifr.ifr_name) - 1);
        setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }
#endif
    uint8 buf_empty[16];
    memset(buf_empty, 0, sizeof(buf_empty));
    if(bind_ipv4 != 0 || memcmp(bind_ipv6, buf_empty, sizeof(bind_ipv6)) != 0)
    {
        struct sockaddr_storage bind_addr;
        memset(&bind_addr, 0, sizeof(struct sockaddr_storage));
        if(bind_ipv4 != 0)
        {
            struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)&bind_addr;
            addr_ipv4->sin_family = AF_INET;
            addr_ipv4->sin_port = htons(bind_port);
            addr_ipv4->sin_addr.s_addr = bind_ipv4;
        }
        else
        {
            struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)&bind_addr;
            addr_ipv6->sin6_port = htons(bind_port);
            addr_ipv6->sin6_family = AF_INET6;
            memcpy(addr_ipv6->sin6_addr.s6_addr, bind_ipv6, sizeof(bind_ipv6));
        }

        if(bind(socketfd, (struct sockaddr*)&bind_addr, bind_addr.ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)) < 0)
        {
            LOG_E("bind failed,errno=%d:%s,host=%s:%d", errno, strerror(errno), host.c_str(), port);
            com_socket_close(socketfd);
            return false;
        }
    }

    com_socket_set_recv_timeout(socketfd, timeout_recv_ms);
    com_socket_set_send_timeout(socketfd, timeout_send_ms);

    if(-1 == connect(socketfd, (struct sockaddr*)(addr.toSockaddrStorage()), addr.is_ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)))
    {
        LOG_E("connect failed,errno=%d:%s,host=%s:%d", errno, strerror(errno), host.c_str(), port);
        com_socket_close(socketfd);
        return false;
    }
    com_socket_set_recv_timeout(socketfd, 0);
    return true;
}

int ComSocketTcp::readData(uint8* buf, int buf_size, uint32 timeout_ms)
{
    if(socketfd <= 0 || buf == NULL || buf_size <= 0)
    {
        LOG_E("arg incorrect");
        return -3;
    }
    if(timeout_ms > 0)
    {
        struct timeval tv;
        fd_set readfds;
        int select_rtn;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        FD_ZERO(&readfds);
        FD_SET(socketfd, &readfds);
        select_rtn = select(socketfd + 1, &readfds, NULL, NULL, &tv);
        //如果对端关闭了连接,select的返回值可能为1,且errno为2
        if(select_rtn < 0)
        {
            LOG_W("select() failed,ret=%d,errno=%d:%s", select_rtn, errno, strerror(errno));
            return -1;
        }
        if(select_rtn == 0)
        {
            LOG_T("select() timeout");
            return -2;
        }
        if(FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("select() no data");
            ((char*)buf)[0] = '\0';
            return 0;
        }
    }
    //如果对端关闭了连接,recv会返回0
    int ret = recv(socketfd, (char*)buf, buf_size, timeout_ms < 0 ? 0 : MSG_DONTWAIT);
    if(ret <= 0)
    {
        LOG_E("recv failed,ret=%d", ret);
        return -4;
    }
    return ret;
}

int ComSocketTcp::writeData(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return -2;
    }
    if(socketfd <= 0)
    {
        LOG_E("socket is incorrect,socketfd=%d", socketfd);
        return -3;
    }
    return send(socketfd, (char*)data, data_size, 0);
}

ComSocketUdp::ComSocketUdp()
{
}

ComSocketUdp::~ComSocketUdp()
{
}

bool ComSocketUdp::openSocket(bool broadcast)
{
#if defined(SOCK_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_IP);
#elif defined(O_CLOEXEC)
    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(socketfd > 0)
    {
        fcntl(socketfd, O_CLOEXEC);
    }
#endif
    if(socketfd < 0)
    {
        LOG_E("failed to create socket");
        return false;
    }
    com_socket_set_send_timeout(socketfd, timeout_recv_ms);
    com_socket_set_recv_timeout(socketfd, timeout_send_ms);

#if __linux__ == 1
    if(!interface_name.empty()) //绑定到指定网卡，忽略路由
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, interface_name.c_str(), sizeof(ifr.ifr_name) - 1);
        setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }
#endif

    if(broadcast)
    {
#if defined(_WIN32) || defined(_WIN64)
        char option = 0x01;
#else
        int option = 0x01;
#endif
        bind_port = 0;
        socklen_t option_len = sizeof(option);
        setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &option, option_len);
    }

    uint8 buf_empty[16];
    memset(buf_empty, 0, sizeof(buf_empty));
    if(bind_ipv4 != 0 || memcmp(bind_ipv6, buf_empty, sizeof(bind_ipv6)) != 0)
    {
        struct sockaddr_storage bind_addr;
        memset(&bind_addr, 0, sizeof(struct sockaddr_storage));
        if(bind_ipv4 != 0)
        {
            struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)&bind_addr;
            addr_ipv4->sin_family = AF_INET;
            addr_ipv4->sin_port = htons(bind_port);
            addr_ipv4->sin_addr.s_addr = bind_ipv4;
        }
        else
        {
            struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)&bind_addr;
            addr_ipv6->sin6_family = AF_INET6;
            addr_ipv6->sin6_port = htons(bind_port);
            memcpy(addr_ipv6->sin6_addr.s6_addr, bind_ipv6, sizeof(bind_ipv6));
        }

        if(bind(socketfd, (struct sockaddr*)&bind_addr, bind_addr.ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)) < 0)
        {
            LOG_E("bind failed,errno=%d:%s,host=%s:%d", errno, strerror(errno), host.c_str(), port);
            com_socket_close(socketfd);
            return false;
        }
    }

    addr_to = com_dns_resolve(host.c_str());
    addr_to.port = port;
    return addr_to.valid();
}

int ComSocketUdp::readData(uint8* buf, int buf_size, ComSocketAddr* addr_from, uint32 timeout_ms)
{
    if(socketfd <= 0 || buf == NULL || buf_size <= 0)
    {
        LOG_E("arg incorrect");
        return -3;
    }
    if(timeout_ms > 0)
    {
        struct timeval tv;
        fd_set readfds;
        int select_rtn;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        FD_ZERO(&readfds);
        FD_SET(socketfd, &readfds);
        select_rtn = select(socketfd + 1, &readfds, NULL, NULL, &tv);
        //如果对端关闭了连接,select的返回值可能为1,且errno为2
        if(select_rtn < 0)
        {
            LOG_W("select() failed,ret=%d,errno=%d:%s", select_rtn, errno, strerror(errno));
            return -1;
        }
        if(select_rtn == 0)
        {
            LOG_T("select() timeout");
            return -2;
        }
        if(FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("select() no data");
            ((char*)buf)[0] = '\0';
            return 0;
        }
    }
    int addr_len = sizeof(struct sockaddr_storage);
    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    int ret = recvfrom(socketfd, (char*)buf, buf_size, 0, (struct sockaddr*)&addr, (socklen_t*)&addr_len);
    if(ret < 0)
    {
        LOG_E("recv failed,ret=%d", ret);
        return -4;
    }
    if(addr_from != NULL)
    {
        if(addr_len == sizeof(struct sockaddr_in))
        {
            struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)&addr;
            addr_from->port = ntohs(addr_ipv4->sin_port);
            addr_from->ipv4 = addr_ipv4->sin_addr.s_addr;
            addr_from->is_ipv6 = false;
        }
        else if(addr_len == sizeof(struct sockaddr_in6))
        {
            struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)&addr;
            addr_from->port = ntohs(addr_ipv6->sin6_port);
            memcpy(addr_from->ipv6, addr_ipv6->sin6_addr.s6_addr, sizeof(addr_from->ipv6));
            addr_from->is_ipv6 = true;
        }
    }
    return ret;
}

int ComSocketUdp::writeData(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0 || addr_to.valid() == false)
    {
        return -1;
    }
    return sendto(socketfd, (char*)data, data_size, 0,
                  (struct sockaddr*)addr_to.toSockaddrStorage(),
                  addr_to.is_ipv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in));
}

ComTcpClient::ComTcpClient()
{
    reconnect_now = false;
    thread_receiver_running = false;
    connected = false;
}

ComTcpClient::ComTcpClient(const char* host, uint16 port)
{
    reconnect_now = false;
    thread_receiver_running = false;
    connected = false;
    setHost(host);
    setPort(port);
}

ComTcpClient::~ComTcpClient()
{
    stopClient();
    com_socket_global_uninit();
}

void ComTcpClient::ThreadRX(ComTcpClient* ctx)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }

    uint8 buf[4096];
    while(ctx->thread_receiver_running)
    {
        if(ctx->reconnect_now || com_socket_get_tcp_connection_status(ctx->socketfd) == 0)
        {
            com_socket_close(ctx->socketfd);
            ctx->socketfd = -1;
            LOG_I("connection to %s:%u lost, will reconnect later", ctx->host.c_str(), ctx->port);
            ctx->connected = false;
            ctx->onConnectionChanged(ctx->connected);
            if(ctx->reconnect_now)
            {
                ctx->reconnect_now = false;
            }
            else
            {
                com_sleep_ms(ctx->reconnect_interval_ms);
            }
        }
        if(ctx->socketfd <= 0)
        {
            if(ctx->openSocket() == false)
            {
                LOG_E("connection to %s:%u failed, will retry 3 sec later", ctx->host.c_str(), ctx->port);
                com_sleep_ms(ctx->reconnect_interval_ms);
                continue;
            }
            LOG_I("connect to %s:%u success,fd=%d", ctx->host.c_str(), ctx->port, ctx->socketfd);
            ctx->connected = true;
            ctx->condition_connection.notifyAll();
            ctx->onConnectionChanged(ctx->connected);
        }

        do
        {
            int ret = ctx->readData(buf, sizeof(buf), 1000);
            if(ret > 0)
            {
                ctx->onRecv(buf, ret);
            }
            else if(ret == -2)
            {
                //select() timeout
            }
            else
            {
                ctx->reconnect_now = true;
                break;
            }
        }
        while(ctx->thread_receiver_running && ctx->reconnect_now == false);
    }
    return;
}

bool ComTcpClient::startClient()
{
    thread_receiver_running = true;
    thread_receiver = std::thread(ThreadRX, this);
    return true;
}

void ComTcpClient::stopClient()
{
    thread_receiver_running = false;
    if(thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    closeSocket();
    LOG_I("connection to %s:%u quit", host.c_str(), port);
    connected = false;
    onConnectionChanged(connected);
}

void ComTcpClient::reconnect()
{
    this->reconnect_now = true;
}

void ComTcpClient::setReconnectInterval(int reconnect_interval_ms)
{
    this->reconnect_interval_ms = reconnect_interval_ms;
}

int ComTcpClient::send(const void* data, int data_size)
{
    if(connected == false || data == NULL || data_size <= 0)
    {
        return 0;
    }
    int ret = writeData(data, data_size);
    if(ret == -1)
    {
        this->reconnect_now = true;
    }
    return ret;
}

bool ComTcpClient::waitForConnected(int timeout_ms)
{
    if(isConnected())
    {
        return true;
    }
    condition_connection.wait(timeout_ms);
    return isConnected();
}

void ComTcpClient::onConnectionChanged(bool connected)
{
}

void ComTcpClient::onRecv(uint8* data, int data_size)
{
}

ComUnixDomainClient::ComUnixDomainClient(const char* server_file_name, const char* file_name)
{
    need_reconnect = false;
    socketfd = -1;
    thread_receiver_running = false;
    if(file_name != NULL)
    {
        this->file_name = file_name;
    }
    if(server_file_name != NULL)
    {
        this->server_file_name = server_file_name;
    }
}

ComUnixDomainClient::~ComUnixDomainClient()
{
    stopClient();
    com_file_remove(getFileName().c_str());
}

void ComUnixDomainClient::setServerFileName(const char* server_file_name)
{
    if(server_file_name != NULL)
    {
        this->server_file_name = server_file_name;
    }
}

void ComUnixDomainClient::setFileName(const char* file_name)
{
    if(file_name != NULL)
    {
        this->file_name = file_name;
    }
}

void ComUnixDomainClient::ThreadReceiver(ComUnixDomainClient* client)
{
    if(client == NULL)
    {
        return;
    }
    uint8 buf[4096 * 4];
    while(client->thread_receiver_running)
    {
        if(client->need_reconnect)
        {
            com_socket_close(client->socketfd);
            client->socketfd = -1;
            LOG_I("connection to %s lost, will reconnect later", client->server_file_name.c_str());
            client->onConnectionChanged(false);
            client->need_reconnect = false;
        }
        if(client->socketfd <= 0)
        {
            client->socketfd = com_socket_unix_domain_open(client->file_name.c_str(),
                               client->server_file_name.c_str());
            if(client->socketfd <= 0)
            {
                LOG_E("connection to %s failed, will retry 3 sec later", client->server_file_name.c_str());
                com_sleep_ms(3000);
                continue;
            }
            LOG_I("connect to %s success", client->server_file_name.c_str());
            client->onConnectionChanged(true);
        }
        memset(buf, 0, sizeof(buf));
        int ret = com_socket_tcp_read(client->socketfd, buf, sizeof(buf), 1000);
        if(ret > 0)
        {
            client->onRecv(buf, ret);
        }
        else if(0 == ret)
        {
            LOG_W("read data empty! will reconnect after 3s");
            client->need_reconnect = true;
            com_sleep_s(3);
        }
        else
        {
            if(-1 == ret)
            {
                LOG_E("read data failed! will reconnect after 3s");
                client->need_reconnect = true;
                com_sleep_s(3);
            }
            else if(-2 == ret)
            {
                // select() timeout
                // com_sleep_s(1);
            }
            else if(-3 == ret)
            {
                // arg incorrect
            }
            else
            {
                LOG_F("recv() returns error: %s, ret: %d", strerror(errno), ret);
                com_sleep_s(1); // 防止CPU占用率过高
            }
        }
    }
    return;
}

int ComUnixDomainClient::startClient()
{
    socketfd = com_socket_unix_domain_open(file_name.c_str(), server_file_name.c_str());
    if(socketfd <= 0)
    {
        LOG_W("connection to %s failed, will auto reconnect later", server_file_name.c_str());
    }
    LOG_I("socketfd: %d, success open server file: %s", socketfd.load(), server_file_name.c_str());
    thread_receiver_running = true;
    onConnectionChanged(true);
    thread_receiver = std::thread(ThreadReceiver, this);
    LOG_I("start unix domain tcp client");
    return 0;
}

void ComUnixDomainClient::stopClient()
{
    thread_receiver_running = false;
    if(thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    if(socketfd > 0)
    {
        com_socket_close(socketfd);
        socketfd = -1;
        LOG_I("connection to %s quit", server_file_name.c_str());
        onConnectionChanged(false);
    }
}

void ComUnixDomainClient::reconnect()
{
    this->need_reconnect = true;
}

int ComUnixDomainClient::send(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return 0;
    }
    return com_socket_tcp_send(socketfd, data, data_size);
}

std::string& ComUnixDomainClient::getFileName()
{
    return file_name;
}

std::string& ComUnixDomainClient::getServerFileName()
{
    return server_file_name;
}

int ComUnixDomainClient::getSocketfd()
{
    return socketfd;
}

void ComUnixDomainClient::onConnectionChanged(bool connected)
{
}

void ComUnixDomainClient::onRecv(uint8* data, int data_size)
{
}

ComMulticastNode::ComMulticastNode()
{
    com_socket_global_init();
    memset(buf, 0, sizeof(buf));
}

ComMulticastNode::~ComMulticastNode()
{
    stopNode();
    com_socket_global_uninit();
}

bool ComMulticastNode::startNode()
{
    LOG_I("called");
    stopNode();
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketfd < 0)
    {
        return false;
    }

#if defined(_WIN32) || defined(_WIN64)
    char option = 1;
#else
    int option = 1;
#endif
    int ret = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if(ret < 0)
    {
        LOG_E("set socket reuse failed");
        return false;
    }

    struct ip_mreq mreq; // 多播地址结构体
    mreq.imr_multiaddr.s_addr = inet_addr(host.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    ret = setsockopt(socketfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    if(ret < 0)
    {
        LOG_E("add multicast group failed");
        return false;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port);

    ret = bind(socketfd, (struct sockaddr*)&local_addr, sizeof(local_addr));
    if(ret < 0)
    {
        perror("bind failed");
        return false;
    }

    thread_rx_running = true;
    thread_rx = std::thread(ThreadRX, this);
    return true;
}

void ComMulticastNode::stopNode()
{
    LOG_I("called");
    thread_rx_running = false;
    if(thread_rx.joinable())
    {
        thread_rx.join();
    }
    com_socket_close(socketfd);
    socketfd = -1;
}

int ComMulticastNode::send(const void* data, int data_size)
{
    return com_socket_udp_send(socketfd, host.c_str(), port, data, data_size);
}

void ComMulticastNode::ThreadRX(ComMulticastNode* client)
{
    if(client == NULL)
    {
        return;
    }
    int ret = 0;
    while(client->thread_rx_running)
    {
        ret = com_socket_udp_read(client->socketfd, client->buf, sizeof(client->buf), 1000);
        if(ret > 0)
        {
            client->onRecv(client->buf, ret);
        }
        else if(ret == -2)
        {
        }
        else if(ret < 0)
        {
            break;
        }
    }
    LOG_I("quit,ret=%d", ret);
    return;
}

void ComMulticastNode::onRecv(uint8* data, int data_size)
{
    ComBytes bytes(data, data_size);
    LOG_I("received,hex=%s,size=%d", bytes.toHexString().c_str(), data_size);
}


ComMulticastNodeString::ComMulticastNodeString()
{
}

ComMulticastNodeString::~ComMulticastNodeString()
{
}

int ComMulticastNodeString::send(const char* str)
{
    if(str == NULL)
    {
        return 0;
    }
    int size = (int)strlen(str) + 1;
    return send(str, size);
}

int ComMulticastNodeString::send(const std::string& str)
{
    return send(str.c_str(), (int)str.length() + 1);
}

int ComMulticastNodeString::getCount()
{
    std::lock_guard<std::mutex> lck(mutex_queue);
    return (int)queue.size();
}

bool ComMulticastNodeString::fetchString(std::string& item)
{
    std::lock_guard<std::mutex> lck(mutex_queue);
    if(queue.empty())
    {
        return false;
    }
    item = queue.front();
    queue.pop();
    return true;
}

bool ComMulticastNodeString::waitString(int timeout_ms)
{
    return condition_queue.wait(timeout_ms);
}

void ComMulticastNodeString::onRecv(uint8* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return;
    }
    for(int i = 0; i < data_size; i++)
    {
        if(data[i] != '\0')
        {
            item.push_back((char)data[i]);
            continue;
        }
        if(item.empty())
        {
            continue;
        }
        mutex_queue.lock();
        queue.push(item);
        mutex_queue.unlock();
        condition_queue.notifyAll();
        item.clear();
    }
}

ComStringIpcClient::ComStringIpcClient()
{
}

ComStringIpcClient::~ComStringIpcClient()
{
    stopIPC();
}

bool ComStringIpcClient::startIPC(const char* name, const char* host, uint16 port)
{
    if(name == NULL || host == NULL)
    {
        return false;
    }
    this->name = name;
    setHost(host);
    setPort(port);

    thread_receiver_running = true;
    thread_receiver = std::thread(ThreadReceiver, this);
    return startClient();
}

void ComStringIpcClient::stopIPC()
{
    thread_receiver_running = false;
    if(thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    stopClient();
}

bool ComStringIpcClient::sendString(const char* name_to, const std::string& message)
{
    return sendString(name_to, message.c_str());
}

bool ComStringIpcClient::sendString(const char* name_to, const char* message)
{
    if(name_to == NULL || message == NULL)
    {
        return false;
    }

    std::string data = this->name;
    data.append("\n");
    data.append(name_to);
    data.append("\n");
    data.append(message);
    if(send(data.c_str(), data.length() + 1) != (int)data.length() + 1)
    {
        return false;
    }
    return true;
}

std::string ComStringIpcClient::getName()
{
    return name;
}

void ComStringIpcClient::onMessage(const std::string& name, const std::string& message)
{
    LOG_I("name=%s,message=%s", name.c_str(), message.c_str());
}

void ComStringIpcClient::onConnectionChanged(bool connected)
{
}

void ComStringIpcClient::onRecv(uint8* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return;
    }
    for(int i = 0; i < data_size; i++)
    {
        if(data[i] != '\0')
        {
            val.push_back((char)data[i]);
            continue;
        }

        if(val.empty())
        {
            continue;
        }

        size_t pos_from = val.find_first_of("\n");
        if(pos_from == std::string::npos)
        {
            val.clear();
            continue;
        }

        size_t pos_to = val.find("\n", pos_from + 1);
        if(pos_to == std::string::npos)
        {
            val.clear();
            continue;
        }

        std::string name_from = val.substr(0, pos_from);
        std::string name_to = val.substr(pos_from + 1, pos_to - pos_from - 1);

        if(name_to != this->name)
        {
            val.clear();
            continue;
        }

        mutex_queue.lock();
        queue.push(val);
        mutex_queue.unlock();
        condition_queue.notifyAll();
        val.clear();
    }
}

void ComStringIpcClient::ThreadReceiver(ComStringIpcClient* ctx)
{
    if(ctx == NULL)
    {
        return;
    }
    while(ctx->thread_receiver_running)
    {
        ctx->condition_queue.wait(1000);
        while(ctx->thread_receiver_running)
        {
            ctx->mutex_queue.lock();
            if(ctx->queue.empty())
            {
                ctx->mutex_queue.unlock();
                break;
            }
            std::string val = ctx->queue.front();
            ctx->queue.pop();
            ctx->mutex_queue.unlock();
            if(val.empty())
            {
                continue;
            }

            size_t pos_from = val.find_first_of("\n");
            if(pos_from == std::string::npos)
            {
                continue;
            }

            size_t pos_to = val.find("\n", pos_from + 1);
            if(pos_to == std::string::npos)
            {
                continue;
            }

            std::string name_from = val.substr(0, pos_from);
            val.erase(0, pos_to + 1);
            ctx->onMessage(name_from, val);
        }
    }
}

ComStringIpcServer::ComStringIpcServer()
{
}

ComStringIpcServer::~ComStringIpcServer()
{
    stopIPC();
}

bool ComStringIpcServer::startIPC(const char* name, uint16 port)
{
    if(name == NULL)
    {
        return false;
    }
    this->name = name;
    thread_receiver_running = true;
    thread_receiver = std::thread(ThreadReceiver, this);
    setPort(port);
    return (startServer() == 0);
}

void ComStringIpcServer::stopIPC()
{
    thread_receiver_running = false;
    if(thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    stopServer();
}

bool ComStringIpcServer::sendString(const char* name_to, const std::string& message)
{
    return sendString(name_to, message.c_str());
}

bool ComStringIpcServer::sendString(const char* name_to, const char* message)
{
    if(name_to == NULL || message == NULL)
    {
        return false;
    }
    int fd = getClientFD(name_to);
    if(fd < 0)
    {
        return false;
    }

    std::string data = this->name;
    data.append("\n");
    data.append(name_to);
    data.append("\n");
    data.append(message);
    if(send(fd, data.c_str(), data.length() + 1) != (int)data.length() + 1)
    {
        return false;
    }
    return true;
}

void ComStringIpcServer::setClientFD(const char* name, int fd)
{
    if(name != NULL)
    {
        LOG_D("String IPC, Get Connection:%s fd:%d", name, fd);
        mutex_clients.lock();
        clients[name] = fd;
        mutex_clients.unlock();
    }
}

int ComStringIpcServer::getClientFD(const char* name)
{
    if(name == NULL)
    {
        return -1;
    }
    std::lock_guard<std::mutex> lck(mutex_clients);
    if(clients.count(name) == 0)
    {
        return -1;
    }
    return clients[name];
}

std::string ComStringIpcServer::getName()
{
    return name;
}

void ComStringIpcServer::removeClientFD(int fd)
{
    mutex_clients.lock();
    for(auto it = clients.begin(); it != clients.end(); it++)
    {
        if(it->second == fd)
        {
            clients.erase(it);
            break;
        }
    }
    mutex_clients.unlock();
}

void ComStringIpcServer::onMessage(const std::string& name, const std::string& message)
{
    LOG_I("name=%s,message=%s", name.c_str(), message.c_str());
}

void ComStringIpcServer::onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected)
{
    LOG_D("String IPC Connection Changed, fd:%d connected:%d", socketfd, connected);
    if(connected == false)
    {
        removeClientFD(socketfd);
    }
}

void ComStringIpcServer::removeClientFD(const char* name)
{
    if(name == NULL)
    {
        return;
    }
    mutex_clients.lock();
    auto it = clients.find(name);
    if(it != clients.end())
    {
        clients.erase(it);
    }
    mutex_clients.unlock();
}

void ComStringIpcServer::onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size)
{

    if(data == NULL || data_size <= 0)
    {
        return;
    }
    LOG_D("String IPC Recv Data, fd:%d data size: %d data:%s", socketfd, data_size, (char*)data);

    if(vals.count(socketfd) == 0)
    {
        vals[socketfd] = std::string();
    }
    std::string& val = vals[socketfd];
    for(int i = 0; i < data_size; i++)
    {
        if(data[i] != '\0')
        {
            val.push_back((char)data[i]);
            continue;
        }

        if(val.empty())
        {
            continue;
        }

        size_t pos_from = val.find_first_of("\n");
        if(pos_from == std::string::npos)
        {
            val.clear();
            continue;
        }
        size_t pos_to = val.find("\n", pos_from + 1);
        if(pos_to == std::string::npos)
        {
            val.clear();
            continue;
        }

        std::string name_from = val.substr(0, pos_from);
        std::string name_to = val.substr(pos_from + 1, pos_to - pos_from - 1);
        setClientFD(name_from.c_str(), socketfd);
        if(name_to == this->name)//发给server
        {
            mutex_queue.lock();
            queue.push(val);
            mutex_queue.unlock();
            condition_queue.notifyAll();
        }
        else//转发
        {
            send(getClientFD(name_to.c_str()), val.c_str(), val.length() + 1);
        }

        val.clear();
    }
}

void ComStringIpcServer::ThreadReceiver(ComStringIpcServer* ctx)
{
    if(ctx == NULL)
    {
        return;
    }
    while(ctx->thread_receiver_running)
    {
        ctx->condition_queue.wait(1000);
        while(ctx->thread_receiver_running)
        {
            ctx->mutex_queue.lock();
            if(ctx->queue.empty())
            {
                ctx->mutex_queue.unlock();
                break;
            }
            std::string val = ctx->queue.front();
            ctx->queue.pop();
            ctx->mutex_queue.unlock();

            if(val.empty())
            {
                continue;
            }

            size_t pos_from = val.find_first_of("\n");
            if(pos_from == std::string::npos)
            {
                continue;
            }

            size_t pos_to = val.find("\n", pos_from + 1);
            if(pos_to == std::string::npos)
            {
                continue;
            }

            std::string name_from = val.substr(0, pos_from);
            val.erase(0, pos_to + 1);
            ctx->onMessage(name_from, val);
        }
    }
}

