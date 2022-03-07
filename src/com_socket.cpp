#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define SOCK_CLOEXEC 0
#define MSG_DONTWAIT 0
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netinet/tcp_fsm.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/un.h>
#include <stddef.h>
#include <fnmatch.h>
#elif __linux__ == 1
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/un.h>
#endif

#include "com_socket.h"
#include "com_serializer.h"
#include "com_thread.h"
#include "com_file.h"
#include "com_log.h"

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

int com_socket_udp_open(const char* interface_name, uint16 local_port, bool broadcast)
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

    NicInfo nic;

#if __linux__ == 1
    if(com_string_len(interface_name) > 0)  //绑定到指定网卡，忽略路由
    {
        if(com_net_get_nic(interface_name, nic) == false)
        {
            com_socket_close(socketfd);
            LOG_E("failed to get %s info", interface_name);
            return -1;
        }
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
        local_port = 0;
        socklen_t option_len = sizeof(option);
        setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &option, option_len);
    }

    /*填写出口地址*/
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_port = htons(local_port);
    local_addr.sin_family = AF_INET;
    if(nic.ip.empty())
    {
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        local_addr.sin_addr.s_addr = inet_addr(nic.ip.c_str());
    }
    if(bind(socketfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    {
        com_socket_close(socketfd);
        return -1;
    }

    return socketfd;
}

int com_unix_domain_tcp_open(const char* my_name, const char* server_name)
{
#if __linux__ == 1
    if(my_name == NULL || server_name == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, server_name);

    com_file_remove(my_name);
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sun_family = AF_UNIX;
    strcpy(client_addr.sun_path, my_name);
    int socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(socketfd < 0)
    {
        LOG_E("failed to create unix domain socket");
        return -1;
    }
    //long len = offsetof(struct sockaddr_un, sun_path) + strlen(client_addr.sun_path);
    int ret = bind(socketfd, (struct sockaddr*)(&client_addr), sizeof(client_addr));
    if(ret < 0)
    {
        com_socket_close(socketfd);
        LOG_E("bind failed");
        return -1;
    }
    //len = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);
    ret = connect(socketfd, (struct sockaddr*)(&server_addr), sizeof(server_addr));
    if(ret != 0)
    {
        com_socket_close(socketfd);
        LOG_E("connect failed");
        return -1;
    }
    return socketfd;
#else
    return -1;
#endif
}

int com_socket_tcp_open(const char* remote_host, uint16 remote_port, uint32 timeout_ms, const char* interface_name)
{
    if(remote_host == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    addrinfo hints;
    addrinfo* res = NULL;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    if(getaddrinfo(remote_host, NULL, &hints, &res) != 0)
    {
        LOG_E("failed to get host ip");
        return -1;
    }
    uint32 remote_ip = ((sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(res);

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


    NicInfo nic;
#if __linux__ == 1
    if(com_string_len(interface_name) > 0)  //绑定到指定网卡，忽略路由
    {
        if(com_net_get_nic(interface_name, nic) == false)
        {
            com_socket_close(socketfd);
            LOG_E("failed to get %s info", interface_name);
            return -1;
        }
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, interface_name, strlen(ifr.ifr_name));
        setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }
#endif

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    if(nic.ip.empty())
    {
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        local_addr.sin_addr.s_addr = inet_addr(nic.ip.c_str());
    }

    if(bind(socketfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    {
        LOG_E("bind fail,errno=%d:%s,host=%s:%d!\n",
              errno, strerror(errno), remote_host, remote_port);
        com_socket_close(socketfd);
        return -1;
    }

    com_socket_set_recv_timeout(socketfd, timeout_ms);
    com_socket_set_send_timeout(socketfd, 10 * 1000);

    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(struct sockaddr_in));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = remote_ip;
    remote_addr.sin_port = htons(remote_port);
    if(-1 == connect(socketfd, (struct sockaddr*)(&remote_addr), sizeof(struct sockaddr)))
    {
        LOG_E("connect fail,errno=%d:%s,host=%s:%d!\n",
              errno, strerror(errno), remote_host, remote_port);
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
    addrinfo hints;
    addrinfo* res = NULL;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    if(getaddrinfo(remote_host, NULL, &hints, &res) != 0)
    {
        LOG_E("failed to get host ip");
        return -1;
    }
    uint32 remote_ip = ((sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(res);

    //设置目的地地址
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = remote_ip;
    target_addr.sin_port = htons(remote_port);
#if 0
    if(-1 == connect(socketfd, (struct sockaddr*)(&target_addr), sizeof(struct sockaddr)))
    {
        LOG_E("connect fail,errno=%d:%s,host=%s:%d!\n",
              errno, strerror(errno), remote_host, remote_port);
        return -1;
    }
#endif
    return sendto(socketfd, (char*)data, data_size, 0,
                  (struct sockaddr*)&target_addr,
                  sizeof(struct sockaddr));
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
        LOG_E("socket is incorrect");
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
            com_socket_close(socketfd);
            return -1;
        }
        if(select_rtn == 0)
        {
            LOG_D("timeout");
            com_socket_close(socketfd);
            return -2;
        }
        if(FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("no data");
            ((char*)data)[0] = '\0';
            com_socket_close(socketfd);
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
            LOG_W("failed,ret=%d,errno=%d:%s", select_rtn, errno, strerror(errno));
            return -1;
        }
        if(select_rtn == 0)
        {
            //LOG_D("timeout");
            return -2;
        }
        if(FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("no data");
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
    CPPBytes bytes = com_file_readall(com_string_format("/proc/sys/net/ipv4/conf/%s/rp_filter", interface_name).c_str());
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
        if(com_string_len(p_ifr->ifr_name) > 0)
        {
            list.push_back(p_ifr->ifr_name);
            LOG_D("int=%s", p_ifr->ifr_name);
        }
    }

    close(sockfd);
#endif
    return list;
}

bool com_net_get_nic(const char* interface_name, NicInfo& nic)
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

NicInfo::NicInfo()
{
    flags = 0;
    addr_family = 0;
    memset(mac, 0, sizeof(mac));
}

NicInfo::~NicInfo()
{
}

Socket::Socket()
{
    port = 0;
    socketfd = 0;
}

Socket::~Socket()
{
}

void Socket::setHost(const char* host)
{
    AutoMutex a(mutxe);
    if(host != NULL)
    {
        this->host = host;
    }
}

void Socket::setPort(uint16 port)
{
    this->port = port;
}

void Socket::setSocketfd(int fd)
{
    this->socketfd = fd;
}

void Socket::setInterface(const char* interface_name)
{
    AutoMutex a(mutxe);
    if(interface_name != NULL)
    {
        this->interface_name = interface_name;
    }
}

std::string Socket::getHost()
{
    AutoMutex a(mutxe);
    return host.c_str();
}

uint16 Socket::getPort()
{
    return port;
}

int Socket::getSocketfd()
{
    return socketfd;
}

std::string Socket::getInterface()
{
    AutoMutex a(mutxe);
    return interface_name;
}

SocketTcpClient::SocketTcpClient()
{
    reconnect_at_once = false;
    running = false;
    connected = false;
}

SocketTcpClient::SocketTcpClient(const char* host, uint16 port)
{
    reconnect_at_once = false;
    running = false;
    connected = false;
    setHost(host);
    setPort(port);
}

SocketTcpClient::~SocketTcpClient()
{
    stopClient();
}

void SocketTcpClient::ThreadSocketClientRunner(SocketTcpClient* socket_client)
{
    if(socket_client == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
    uint8 buf[1024];
    while(socket_client->running)
    {
        if(socket_client->reconnect_at_once || com_socket_get_tcp_connection_status(socket_client->socketfd) == 0)
        {
            com_socket_close(socket_client->socketfd);
            socket_client->socketfd = -1;
            LOG_I("connection to %s:%u lost, will reconnect later",
                  socket_client->getHost().c_str(), socket_client->getPort());
            socket_client->connected = false;
            socket_client->onConnectionChanged(socket_client->connected);
            if(socket_client->reconnect_at_once)
            {
                socket_client->reconnect_at_once = false;
            }
            else
            {
                com_sleep_ms(socket_client->reconnect_interval_ms);
            }
        }
        if(socket_client->socketfd <= 0)
        {
            socket_client->socketfd = com_socket_tcp_open(socket_client->getHost().c_str(),
                                      socket_client->getPort(), 10 * 1000,
                                      socket_client->getInterface().c_str());
            if(socket_client->socketfd <= 0)
            {
                LOG_E("connection to %s:%u failed, will retry 1 sec later",
                      socket_client->getHost().c_str(), socket_client->getPort());
                com_sleep_ms(socket_client->reconnect_interval_ms);
                continue;
            }
            socket_client->connected = true;
            socket_client->onConnectionChanged(socket_client->connected);
        }
        memset(buf, 0, sizeof(buf));
        int ret = com_socket_tcp_read(socket_client->socketfd, buf, sizeof(buf), 1000);
        if(ret == -1)
        {
            socket_client->reconnect_at_once = true;
        }
        else if(ret > 0)
        {
            socket_client->onRecv(buf, ret);
        }
    }
    return;
}

bool SocketTcpClient::startClient()
{
    if(getHost().empty() || getPort() == 0)
    {
        LOG_E("host or port not set");
        return false;
    }
    running = true;
    thread_runner = std::thread(ThreadSocketClientRunner, this);
    return true;
}

void SocketTcpClient::stopClient()
{
    running = false;
    if(thread_runner.joinable())
    {
        thread_runner.join();
    }
    if(socketfd > 0)
    {
        com_socket_close(socketfd);
        socketfd = -1;
        LOG_I("connection to %s:%u quit", getHost().c_str(), getPort());
        connected = false;
        onConnectionChanged(connected);
    }
}

void SocketTcpClient::reconnect()
{
    this->reconnect_at_once = true;
}

void SocketTcpClient::setReconnectInterval(int reconnect_interval_ms)
{
    this->reconnect_interval_ms = reconnect_interval_ms;
}

int SocketTcpClient::send(const void* data, int data_size)
{
    if(connected == false || data == NULL || data_size <= 0)
    {
        return 0;
    }
    int ret = com_socket_tcp_send(socketfd, data, data_size);
    if(ret == -1)
    {
        this->reconnect_at_once = true;
    }
    return ret;
}

bool SocketTcpClient::isConnected()
{
    return connected;
}

void SocketTcpClient::onConnectionChanged(bool connected)
{
}

void SocketTcpClient::onRecv(uint8* data, int data_size)
{
}

UnixDomainTcpClient::UnixDomainTcpClient(const char* server_file_name, const char* file_name)
{
    need_reconnect = false;
    socketfd = -1;
    receiver_running = false;
    if(file_name != NULL)
    {
        this->file_name = file_name;
    }
    if(server_file_name != NULL)
    {
        this->server_file_name = server_file_name;
    }
}

UnixDomainTcpClient::~UnixDomainTcpClient()
{
    stopClient();
    com_file_remove(getFileName().c_str());
}

void UnixDomainTcpClient::ThreadUnixDomainClientReceiver(UnixDomainTcpClient* client)
{
    if(client == NULL)
    {
        return;
    }
    uint8 buf[1024];
    while(client->receiver_running)
    {
        if(client->need_reconnect)
        {
            com_socket_close(client->socketfd);
            client->socketfd = -1;
            LOG_I("connection to %s lost, will reconnect later",
                  client->server_file_name.c_str());
            client->onConnectionChanged(false);
            client->need_reconnect = false;
        }
        if(client->socketfd <= 0)
        {
            client->socketfd = com_unix_domain_tcp_open(client->file_name.c_str(),
                               client->server_file_name.c_str());
            if(client->socketfd <= 0)
            {
                LOG_E("connection to %s failed, will retry 1 sec later",
                      client->server_file_name.c_str());
                com_sleep_ms(1000);
                continue;
            }
            client->onConnectionChanged(true);
        }
        memset(buf, 0, sizeof(buf));
        int ret = com_socket_tcp_read(client->socketfd, buf, sizeof(buf), 1000);
        if(ret == -1)
        {
            client->need_reconnect = true;
        }
        else if(ret > 0)
        {
            client->onRecv(buf, ret);
        }
    }
    return;
}

int UnixDomainTcpClient::startClient()
{
    socketfd = com_unix_domain_tcp_open(file_name.c_str(), server_file_name.c_str());
    if(socketfd <= 0)
    {
        LOG_W("connection to %s failed, will auto reconnect later", server_file_name.c_str());
    }
    receiver_running = true;
    onConnectionChanged(true);
    thread_receiver = std::thread(ThreadUnixDomainClientReceiver, this);
    return 0;
}

void UnixDomainTcpClient::stopClient()
{
    receiver_running = false;
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

void UnixDomainTcpClient::reconnect()
{
    this->need_reconnect = true;
}

int UnixDomainTcpClient::send(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return 0;
    }
    return com_socket_tcp_send(socketfd, data, data_size);
}

std::string& UnixDomainTcpClient::getFileName()
{
    return file_name;
}

std::string& UnixDomainTcpClient::getServerFileName()
{
    return server_file_name;
}

int UnixDomainTcpClient::getSocketfd()
{
    return socketfd;
}

void UnixDomainTcpClient::onConnectionChanged(bool connected)
{
}

void UnixDomainTcpClient::onRecv(uint8* data, int data_size)
{
}
