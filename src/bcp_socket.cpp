#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define SOCK_CLOEXEC 0
#define MSG_DONTWAIT 0
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/un.h>
#include <stddef.h>
#include <fnmatch.h>
#endif

#include "bcp_socket.h"
#include "bcp_serializer.h"
#include "bcp_thread.h"
#include "bcp_file.h"
#include "bcp_log.h"

//对connect同样适用
void bcp_socket_set_recv_timeout(int sock, int timeout_ms)
{
    int ret;
#if defined(_WIN32) || defined(_WIN64)
    int timeout = timeout_ms;
#else
    struct timeval timeout = {timeout_ms / 1000, timeout_ms % 1000};
#endif
    ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                     (const char*)&timeout, sizeof(timeout));
    if (ret < 0)
    {
        LOG_E("socket set recv timeout failed");
    }
}

void bcp_socket_set_send_timeout(int sock, int timeout_ms)
{
    int ret;
#if defined(_WIN32) || defined(_WIN64)
    int timeout = timeout_ms;
#else
    struct timeval timeout = {timeout_ms / 1000, timeout_ms % 1000};
#endif
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
                     (const char*)&timeout, sizeof(timeout));
    if (ret < 0)
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
int bcp_socket_get_tcp_connection_status(int sock)
{
    if (sock <= 0)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    int optval = -1;
    int optlen = sizeof(int);
    int ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*) &optval, &optlen);
    if (ret != 0)
    {
        LOG_D("failed to get tcp socket status, ret=%d", ret);
        return -1;
    }
    if (optval != 0)
    {
        return 0;
    }
    return 1;
#else
    struct tcp_info info;
    int len = sizeof(info);
    int ret = getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len);
    if (ret != 0)
    {
        LOG_D("failed to get tcp socket status, ret=%d,errno=%d:%s", ret, errno, strerror(errno));
        return -1;
    }
    if (info.tcpi_state != TCP_ESTABLISHED)
    {
        LOG_D("tcp socket status=%d", info.tcpi_state);
        return 0;
    }
    return 1;
#endif
}

bool bcp_socket_is_tcp_connected(int sock)
{
    if (sock <= 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    int optval = -1;
    int optlen = sizeof(int);
    int ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*) &optval, &optlen);
    if (ret != 0)
    {
        LOG_D("failed to get tcp socket status, ret=%d", ret);
        return false;
    }
    if (optval == 0)
    {
        return true;
    }
    return false;
#else
    struct tcp_info info;
    int len = sizeof(info);
    int ret = getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len);
    if (ret != 0)
    {
        LOG_D("failed to get tcp socket status, ret=%d,errno=%d:%s", ret, errno, strerror(errno));
        return false;
    }
    if (info.tcpi_state != TCP_ESTABLISHED)
    {
        LOG_D("tcp socket status=%d", info.tcpi_state);
        return false;
    }
    return true;
#endif
}

int bcp_socket_udp_open(const char* interface_name, uint16 local_port, bool broadcast)
{
    int socketfd = -1;

    /*建立socket*/
    if ((socketfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_IP)) < 0)
    {
        LOG_E("failed to open socket");
        return -1;
    }
    bcp_socket_set_send_timeout(socketfd, 10 * 1000);
    bcp_socket_set_recv_timeout(socketfd, 10 * 1000);

    NicInfo nic;

#if __linux__ == 1
    if (bcp_string_len(interface_name) > 0) //绑定到指定网卡，忽略路由
    {
        if (bcp_net_get_nic(interface_name, nic) == false)
        {
            bcp_socket_close(socketfd);
            LOG_E("failed to get %s info", interface_name);
            return -1;
        }
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, interface_name, strlen(interface_name));
        setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }
#endif

    if (broadcast)
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
    if (nic.ip.empty())
    {
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        local_addr.sin_addr.s_addr = inet_addr(nic.ip.c_str());
    }
    if (bind(socketfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    {
        bcp_socket_close(socketfd);
        return -1;
    }

    return socketfd;
}
#if __linux__ == 1
int bcp_unix_domain_tcp_open(const char* my_name, const char* server_name)
{
    if (my_name == NULL || server_name == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, server_name);

    bcp_file_remove(my_name);
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sun_family = AF_UNIX;
    strcpy(client_addr.sun_path, my_name);
    int socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        LOG_E("failed to create unix domain socket");
        return -1;
    }
    int len = offsetof(struct sockaddr_un, sun_path) + strlen(client_addr.sun_path);
    int ret = bind(socketfd, (struct sockaddr*)(&client_addr), len);
    if (ret < 0)
    {
        bcp_socket_close(socketfd);
        LOG_E("bind failed");
        return -1;
    }
    len = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);
    ret = connect(socketfd, (struct sockaddr*)(&server_addr), len);
    if (ret != 0)
    {
        bcp_socket_close(socketfd);
        LOG_E("connect failed");
        return -1;
    }
    return socketfd;
}
#endif

int bcp_socket_tcp_open(const char* remote_host, uint16 remote_port, uint32 timeout_ms, const char* interface_name)
{
    if (remote_host == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    addrinfo hints;
    addrinfo* res = NULL;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    if (getaddrinfo(remote_host, NULL, &hints, &res) != 0)
    {
        LOG_E("failed to get host ip");
        return -1;
    }
    uint32 remote_ip = ((sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(res);

    int socketfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_IP);
    if (-1 == socketfd)
    {
        LOG_E("socketfd fail");
        return -1;
    }


    NicInfo nic;
#if __linux__ == 1
    if (bcp_string_len(interface_name) > 0) //绑定到指定网卡，忽略路由
    {
        if (bcp_net_get_nic(interface_name, nic) == false)
        {
            bcp_socket_close(socketfd);
            LOG_E("failed to get %s info", interface_name);
            return -1;
        }
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, interface_name, strlen(interface_name));
        setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }
#endif

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    if (nic.ip.empty())
    {
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        local_addr.sin_addr.s_addr = inet_addr(nic.ip.c_str());
    }

    if (bind(socketfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    {
        LOG_E("bind fail,errno=%d:%s,host=%s:%d!\n",
              errno, strerror(errno), remote_host, remote_port);
        bcp_socket_close(socketfd);
        return -1;
    }

    bcp_socket_set_recv_timeout(socketfd, timeout_ms);
    bcp_socket_set_send_timeout(socketfd, 10 * 1000);

    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(struct sockaddr_in));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = remote_ip;
    remote_addr.sin_port = htons(remote_port);
    if (-1 == connect(socketfd, (struct sockaddr*)(&remote_addr), sizeof(struct sockaddr)))
    {
        LOG_E("connect fail,errno=%d:%s,host=%s:%d!\n",
              errno, strerror(errno), remote_host, remote_port);
        bcp_socket_close(socketfd);
        return -1;
    }
    bcp_socket_set_recv_timeout(socketfd, 0);
    return socketfd;
}

int bcp_socket_udp_send(int socketfd, const char* remote_host, int remote_port,
                        void* data, int data_size)
{
    if (remote_host == NULL || data == NULL || data_size <= 0)
    {
        return -1;
    }
    addrinfo hints;
    addrinfo* res = NULL;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    if (getaddrinfo(remote_host, NULL, &hints, &res) != 0)
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
    if (-1 == connect(socketfd, (struct sockaddr*)(&target_addr), sizeof(struct sockaddr)))
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

int bcp_socket_tcp_send(int socketfd, void* data, int data_size)
{
    if (data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return -2;
    }
    if (socketfd <= 0)
    {
        LOG_E("socket is incorrect");
        return -3;
    }
    return send(socketfd, (char*)data, data_size, 0);
}

int bcp_socket_udp_read(int socketfd, uint8* data, int data_size,
                        uint32 timeout_ms, char* remote_ip, int sender_ip_size)
{
    if (socketfd <= 0 || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    if (timeout_ms != 0)
    {
        struct timeval tv;
        fd_set readfds;
        int select_rtn;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        FD_ZERO(&readfds);
        FD_SET(socketfd, &readfds);
        select_rtn = select(socketfd + 1, &readfds, NULL, NULL, &tv);
        if (select_rtn == -1)
        {
            LOG_E("failed");
            bcp_socket_close(socketfd);
            return -1;
        }
        if (select_rtn == 0)
        {
            LOG_D("timeout");
            bcp_socket_close(socketfd);
            return -2;
        }
        if (FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("no data");
            ((char*)data)[0] = '\0';
            bcp_socket_close(socketfd);
            return 0;
        }
    }
    int addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    int len = recvfrom(socketfd, (char*)data, data_size, 0,
                       (struct sockaddr*)&addr, (socklen_t*)&addr_len);
    if (remote_ip && sender_ip_size > 0)
    {
        uint32 ip_addr = ntohl(addr.sin_addr.s_addr);
        bcp_snprintf(remote_ip, sender_ip_size, "%d.%d.%d.%d",
                     (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
                     (ip_addr >> 8) & 0xFF, ip_addr & 0xFF);
    }
    return len;
}

//如果此函数返回值<=0则需要主动检查下连接是否已断开了(-2代表超时,可以不用检查连接)
int bcp_socket_tcp_read(int socketfd, uint8* data, int data_size,
                        uint32 timeout_ms, bool block_mode)
{
    if (socketfd <= 0 || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return -3;
    }
    if (timeout_ms > 0)
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
        if (select_rtn < 0)
        {
            LOG_W("failed,ret=%d,errno=%d:%s", select_rtn, errno, strerror(errno));
            return -1;
        }
        if (select_rtn == 0)
        {
            //LOG_D("timeout");
            return -2;
        }
        if (FD_ISSET(socketfd, &readfds) == 0)
        {
            LOG_D("no data");
            ((char*)data)[0] = '\0';
            return 0;
        }
    }
    //如果对端关闭了连接,recv会返回0
    return recv(socketfd, (char*)data, data_size, block_mode ? 0 : MSG_DONTWAIT);
}

void bcp_socket_close(int socketfd)
{
    if (socketfd > 0)
    {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(socketfd);
#else
        close(socketfd);
#endif
    }
}

uint8 bcp_net_get_rpfilter(const char* interface_name)
{
#if __linux__ == 1
    if (interface_name == NULL)
    {
        return 0xFF;
    }
    ByteArray bytes = bcp_file_readall(bcp_string_format("/proc/sys/net/ipv4/conf/%s/rp_filter", interface_name).c_str());
    uint8 flag = (uint8)strtoul(bytes.toString().c_str(), NULL, 10);
    if (flag != 0 && flag != 1 && flag != 2)
    {
        return 0xFF;
    }
    return flag;
#else
    return 0xFF;
#endif
}

//反向原地址可达性校验，0=不校验，1=严格校验，2=宽松校验
void bcp_net_set_rpfilter(const char* interface_name, uint8 flag)
{
#if __linux__ == 1
    if (interface_name != NULL && (flag == 0 || flag == 1 || flag == 2))
    {
        bcp_run_shell("echo %d > /proc/sys/net/ipv4/conf/%s/rp_filter", flag, interface_name);
    }
#endif
}

bool bcp_net_is_interface_exist(const char* interface_name)
{
#if __linux__ == 1
    if (interface_name == NULL)
    {
        return false;
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        return false;
    }

    char buf[8192] = {0};
    struct ifconf ifc;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
    {
        close(sockfd);
        return false;
    }
    struct ifreq* p_ifr = ifc.ifc_req;
    for (int i = ifc.ifc_len / sizeof(struct ifreq); i >= 0; p_ifr++, i--)
    {
        if (bcp_string_equal(interface_name, p_ifr->ifr_name))
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

std::vector<std::string> bcp_net_get_interface_all()
{
    std::vector<std::string> list;
#if __linux__ == 1
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        return list;
    }
    char buff[8192] = {0};
    struct ifconf ifc;
    ifc.ifc_len = sizeof(buff);
    ifc.ifc_buf = buff;
    if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)//网卡没有分配IP则获取不到
    {
        close(sockfd);
        return list;
    }
    struct ifreq* p_ifr = ifc.ifc_req;
    for (int i = ifc.ifc_len / sizeof(struct ifreq); i >= 0; p_ifr++, i--)
    {
        if (bcp_string_len(p_ifr->ifr_name) > 0)
        {
            list.push_back(p_ifr->ifr_name);
            LOG_E("int=%s", p_ifr->ifr_name);
        }
    }

    close(sockfd);
#endif
    return list;
}

bool bcp_net_get_nic(const char* interface_name, NicInfo& nic)
{
#if __linux__ == 1
    if (bcp_net_is_interface_exist(interface_name) == false)
    {
        return false;
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        return false;
    }

    nic.name = interface_name;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0)
    {
        struct sockaddr_in sin;
        memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
        nic.ip = inet_ntoa(sin.sin_addr);
    }
    if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) == 0)
    {
        struct sockaddr_in sin;
        memcpy(&sin, &ifr.ifr_netmask, sizeof(sin));
        nic.ip_mask = inet_ntoa(sin.sin_addr);
    }
    if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr) == 0)
    {
        struct sockaddr_in sin;
        memcpy(&sin, &ifr.ifr_broadaddr, sizeof(sin));
        nic.ip_broadcast = inet_ntoa(sin.sin_addr);
    }
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0)
    {
        nic.addr_family = ifr.ifr_addr.sa_family;
        memcpy(nic.mac, ifr.ifr_hwaddr.sa_data, sizeof(nic.mac));
    }
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == 0)
    {
        if (ifr.ifr_flags & IFF_UP)
        {
            nic.flags |= 0x00000002;
        }

        if (ifr.ifr_flags & IFF_RUNNING)
        {
            nic.flags |= 0x00000004;
        }

        if (ifr.ifr_flags & IFF_LOOPBACK)
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

bool bcp_net_get_mac(const char* interface_name, uint8* mac)
{
#if __linux__ == 1
    if (interface_name == NULL || mac == NULL)
    {
        return false;
    }
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof(struct ifreq));
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG_E("socket failed");
        return false;
    }
    strncpy(ifreq.ifr_name, interface_name, sizeof(ifreq.ifr_name));    //Currently, only get eth0

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifreq) < 0)
    {
        close(sockfd);
        LOG_E("ioctl failed");
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
    if (host != NULL)
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
    if (interface_name != NULL)
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
    if (socket_client == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
    uint8 buf[1024];
    while (socket_client->running)
    {
        if (socket_client->reconnect_at_once || bcp_socket_get_tcp_connection_status(socket_client->socketfd) == 0)
        {
            bcp_socket_close(socket_client->socketfd);
            socket_client->socketfd = -1;
            LOG_I("connection to %s:%u lost, will reconnect later",
                  socket_client->getHost().c_str(), socket_client->getPort());
            socket_client->connected = false;
            socket_client->onConnectionChanged(socket_client->connected);
            if (socket_client->reconnect_at_once)
            {
                socket_client->reconnect_at_once = false;
            }
            else
            {
                bcp_sleep_ms(socket_client->reconnect_interval_ms);
            }
        }
        if (socket_client->socketfd <= 0)
        {
            socket_client->socketfd = bcp_socket_tcp_open(socket_client->getHost().c_str(),
                                      socket_client->getPort(), 10 * 1000,
                                      socket_client->getInterface().c_str());
            if (socket_client->socketfd <= 0)
            {
                LOG_E("connection to %s:%u failed, will retry 1 sec later",
                      socket_client->getHost().c_str(), socket_client->getPort());
                bcp_sleep_ms(socket_client->reconnect_interval_ms);
                continue;
            }
            socket_client->connected = true;
            socket_client->onConnectionChanged(socket_client->connected);
        }
        memset(buf, 0, sizeof(buf));
        int ret = bcp_socket_tcp_read(socket_client->socketfd, buf, sizeof(buf), 1000);
        if (ret == -1)
        {
            socket_client->reconnect_at_once = true;
        }
        else if (ret > 0)
        {
            socket_client->onRecv(buf, ret);
        }
    }
    return;
}

int SocketTcpClient::startClient()
{
    if (getHost().empty() || getPort() == 0)
    {
        LOG_E("host or port not set");
        return -1;
    }
    running = true;
    thread_runner = std::thread(ThreadSocketClientRunner, this);
    return 0;
}

void SocketTcpClient::stopClient()
{
    running = false;
    if (thread_runner.joinable())
    {
        thread_runner.join();
    }
    if (socketfd > 0)
    {
        bcp_socket_close(socketfd);
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

int SocketTcpClient::send(uint8* data, int data_size)
{
    if (connected == false || data == NULL || data_size <= 0)
    {
        return 0;
    }
    int ret = bcp_socket_tcp_send(socketfd, data, data_size);
    if (ret == -1)
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

#if __linux__ == 1
SocketTcpServer::SocketTcpServer()
{
    setHost("127.0.0.1");
    epollfd = -1;
    receiver_running = false;
    listener_running = false;
    epoll_timeout_ms = 3000;
}

SocketTcpServer::SocketTcpServer(uint16 port)
{
    setHost("127.0.0.1");
    setPort(port);
    epollfd = -1;
    receiver_running = false;
    listener_running = false;
    epoll_timeout_ms = 3000;
}

SocketTcpServer::~SocketTcpServer()
{
    stopServer();
}

void SocketTcpServer::ThreadSocketServerReceiver(SocketTcpServer* socket_server)
{
    uint8 buf[4096];
    while (socket_server->receiver_running)
    {
        socket_server->semfds.wait(1000);
        socket_server->mutexfds.lock();
        if (socket_server->ready_fds.size() <= 0)
        {
            socket_server->mutexfds.unlock();
            continue;
        }
        CLIENT_DES des = socket_server->ready_fds.front();
        socket_server->ready_fds.pop();
        socket_server->mutexfds.unlock();
        while (true)
        {
            memset(buf, 0, sizeof(buf));
            int ret = recv(des.clientfd, buf, sizeof(buf), MSG_DONTWAIT);
            if (ret <= 0)
            {
                break;
            }
            socket_server->onRecv(des.host, des.port, des.clientfd, buf, ret);
        }
    }
    return;
}

void SocketTcpServer::ThreadSocketServerListener(SocketTcpServer* socket_server)
{
    if (socket_server == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
    struct epoll_event eventList[SOCKET_SERVER_MAX_CLIENTS];
    while (socket_server->listener_running)
    {
        socket_server->epoll_timeout_ms = 1000;
        //epoll_wait
        int count = epoll_wait(socket_server->epollfd, eventList,
                               SOCKET_SERVER_MAX_CLIENTS, socket_server->epoll_timeout_ms);
        if (count < 0)
        {
            LOG_E("epoll error,errno=%d:%s", errno, strerror(errno));
            break;
        }
        else if (count == 0)
        {
            //LOG_D("epoll timeout");
            continue;
        }
        if (socket_server->listener_running == false)
        {
            break;
        }
        //直接获取了事件数量,给出了活动的流,这里是和poll区别的关键
        for (int i = 0; i < count; i++)
        {
            if ((eventList[i].events & EPOLLERR) ||
                    (eventList[i].events & EPOLLHUP) ||
                    (eventList[i].events & EPOLLRDHUP) ||//远端close
                    !(eventList[i].events & EPOLLIN))
            {
                LOG_D("epoll client quit, fd=%d, event=0x%X", eventList[i].data.fd, eventList[i].events);
                socket_server->closeClient(eventList[i].data.fd);
                break;
            }
            if (eventList[i].data.fd == socket_server->socketfd)
            {
                socket_server->acceptClient();
            }
            else
            {
                socket_server->recvData(eventList[i].data.fd);
            }
        }
    }
    LOG_I("socket server quit, port=%d", socket_server->getPort());
    return;
}

int SocketTcpServer::recvData(int socketfd)//ThreadSocketServerRunner线程
{
    mutex_clients.lock();
    if (clients.count(socketfd) == 0)
    {
        mutex_clients.unlock();
        return -1;
    }
    CLIENT_DES des = clients[socketfd];
    mutex_clients.unlock();
    mutexfds.lock();
    ready_fds.push(des);
    semfds.post();
    mutexfds.unlock();
    return 0;
}

void SocketTcpServer::closeClient(int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    CLIENT_DES des;
    des.clientfd = fd;
    mutex_clients.lock();
    if (clients.count(fd) == 0)
    {
        mutex_clients.unlock();
        return;
    }
    des = clients[fd];
    mutex_clients.unlock();
    onConnectionChanged(des.host, des.port, des.clientfd, false);
    bcp_socket_close(fd);
    mutex_clients.lock();
    if (clients.count(fd) != 0)
    {
        clients.erase(fd);
    }
    mutex_clients.unlock();
}

int SocketTcpServer::acceptClient()//ThreadSocketServerRunner线程
{
    struct sockaddr_in sin;
    socklen_t len = sizeof(struct sockaddr_in);
    memset(&sin, 0, len);
    int clientfd = accept(socketfd, (struct sockaddr*)&sin, &len);
    if (clientfd < 0)
    {
        LOG_E("bad accept client, errno=%d:%s", errno, strerror(errno));
        return -1;
    }
    LOG_D("Accept Connection, fd=%d, addr=%s,port=%u",
          clientfd, bcp_ip_to_string(sin.sin_addr.s_addr).c_str(), sin.sin_port);
    CLIENT_DES des;
    des.clientfd = clientfd;
    des.host = bcp_ip_to_string(sin.sin_addr.s_addr);
    des.port = sin.sin_port;
    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到EPOLL的监听中
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = clientfd;
    event.events =  EPOLLIN | EPOLLET | EPOLLRDHUP;//边缘触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &event);
    onConnectionChanged(des.host, des.port, des.clientfd, true);
    return 0;
}

int SocketTcpServer::startServer()
{
    if (port == 0)
    {
        LOG_E("port not set");
        return -1;
    }
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd <= 0)
    {
        LOG_E("socket create failed : fd = %d", getSocketfd());
        return -1;
    }
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family  =  AF_INET;
    server_addr.sin_port = htons(getPort());
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);
    int resuse_flag = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &resuse_flag, sizeof(int));
    if (bind(getSocketfd(), (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        LOG_E("socket bind failed : fd = %d", getSocketfd());
        bcp_socket_close(socketfd);
        return -2;
    }
    if (listen(socketfd, 5) < 0)
    {
        LOG_E("socket listen failed : fd = %d", getSocketfd());
        bcp_socket_close(socketfd);
        return -3;
    }
    // epoll 初始化
    epollfd = epoll_create(SOCKET_SERVER_MAX_CLIENTS);
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    event.data.fd = getSocketfd();
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) < 0)
    {
        LOG_E("epoll add failed : fd = %d", getSocketfd());
        return -4;
    }
    listener_running = true;
    thread_listener = std::thread(ThreadSocketServerListener, this);
    receiver_running = true;
    thread_receiver = std::thread(ThreadSocketServerReceiver, this);
    return 0;
}

void SocketTcpServer::stopServer()
{
    listener_running = false;
    if (thread_listener.joinable())
    {
        thread_listener.join();
    }
    receiver_running = false;
    if (thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    bcp_socket_close(socketfd);
    socketfd = -1;
    bcp_socket_close(epollfd);
    epollfd = -1;
    mutex_clients.lock();
    for (int i = 0; i < clients.size(); i++)
    {
        bcp_socket_close(clients[i].clientfd);
    }
    mutex_clients.unlock();
    LOG_I("socket server stopped");
}

int SocketTcpServer::send(const char* host, uint16 port, uint8* data, int dataSize)
{
    if (host == NULL || data == NULL || dataSize <= 0)
    {
        return 0;
    }
    mutex_clients.lock();
    int clientfd = -1;
    std::map<int, CLIENT_DES>::iterator it;
    for (it = clients.begin(); it != clients.end(); it++)
    {
        CLIENT_DES des = it->second;
        if (bcp_string_equal(des.host.c_str(), host) && des.port == port)
        {
            clientfd = des.clientfd;
            break;
        }
    }
    mutex_clients.unlock();
    if (clientfd == -1)
    {
        return -1;
    }
    int ret = bcp_socket_tcp_send(clientfd, data, dataSize);
    if (ret == -1)
    {
        closeClient(clientfd);
    }
    return ret;
}

int SocketTcpServer::send(int clientfd, uint8* data, int dataSize)
{
    if (data == NULL || dataSize <= 0)
    {
        return 0;
    }
    int ret = bcp_socket_tcp_send(clientfd, data, dataSize);
    if (ret == -1)
    {
        closeClient(clientfd);
    }
    return ret;
}

void SocketTcpServer::onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected)
{
}

void SocketTcpServer::onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int dataSize)
{
}

UnixDomainTcpClient::UnixDomainTcpClient(const char* server_file_name, const char* file_name)
{
    need_reconnect = false;
    socketfd = -1;
    receiver_running = false;
    if (file_name != NULL)
    {
        this->file_name = file_name;
    }
    if (server_file_name != NULL)
    {
        this->server_file_name = server_file_name;
    }
}

UnixDomainTcpClient::~UnixDomainTcpClient()
{
    stopClient();
    bcp_file_remove(getFileName().c_str());
}

void UnixDomainTcpClient::ThreadUnixDomainClientReceiver(UnixDomainTcpClient* client)
{
    if (client == NULL)
    {
        return;
    }
    uint8 buf[1024];
    while (client->receiver_running)
    {
        if (client->need_reconnect)
        {
            bcp_socket_close(client->socketfd);
            client->socketfd = -1;
            LOG_I("connection to %s lost, will reconnect later",
                  client->server_file_name.c_str());
            client->onConnectionChanged(false);
            client->need_reconnect = false;
        }
        if (client->socketfd <= 0)
        {
            client->socketfd = bcp_unix_domain_tcp_open(client->file_name.c_str(),
                               client->server_file_name.c_str());
            if (client->socketfd <= 0)
            {
                LOG_E("connection to %s failed, will retry 1 sec later",
                      client->server_file_name.c_str());
                bcp_sleep_ms(1000);
                continue;
            }
            client->onConnectionChanged(true);
        }
        memset(buf, 0, sizeof(buf));
        int ret = bcp_socket_tcp_read(client->socketfd, buf, sizeof(buf), 1000);
        if (ret == -1)
        {
            client->need_reconnect = true;
        }
        else if (ret > 0)
        {
            client->onRecv(buf, ret);
        }
    }
    return;
}

int UnixDomainTcpClient::startClient()
{
    socketfd = bcp_unix_domain_tcp_open(file_name.c_str(), server_file_name.c_str());
    if (socketfd <= 0)
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
    if (thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    if (socketfd > 0)
    {
        bcp_socket_close(socketfd);
        socketfd = -1;
        LOG_I("connection to %s quit", server_file_name.c_str());
        onConnectionChanged(false);
    }
}

void UnixDomainTcpClient::reconnect()
{
    this->need_reconnect = true;
}

int UnixDomainTcpClient::send(uint8* data, int data_size)
{
    if (data == NULL || data_size <= 0)
    {
        return 0;
    }
    return bcp_socket_tcp_send(socketfd, data, data_size);
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

UnixDomainTcpServer::UnixDomainTcpServer(const char* server_file_name)
{
    if (server_file_name != NULL)
    {
        this->server_file_name = server_file_name;
    }
    socketfd = -1;
    epollfd = -1;
    listener_running = false;
    receiver_running = false;
    epoll_timeout_ms = 3000;
}

UnixDomainTcpServer::~UnixDomainTcpServer()
{
    stopServer();
    bcp_file_remove(getServerFileName().c_str());
}

void UnixDomainTcpServer::ThreadUnixDomainServerReceiver(UnixDomainTcpServer* socket_server)
{
    uint8 buf[4096];
    while (socket_server->receiver_running)
    {
        socket_server->semfds.wait(1000);
        socket_server->mutexfds.lock();
        if (socket_server->fds.size() <= 0)
        {
            socket_server->mutexfds.unlock();
            continue;
        }

        CLIENT_DES des = socket_server->fds.front();
        socket_server->fds.pop();
        socket_server->mutexfds.unlock();
        while (true)
        {
            memset(buf, 0, sizeof(buf));
            int ret = recv(des.clientfd, buf, sizeof(buf), MSG_DONTWAIT);
            if (ret <= 0)
            {
                break;
            }
            socket_server->onRecv(des.host, des.clientfd, buf, ret);
        }
    }
    return;
}

void UnixDomainTcpServer::ThreadUnixDomainServerListener(UnixDomainTcpServer* socket_server)
{
    if (socket_server == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
    struct epoll_event event_list[SOCKET_SERVER_MAX_CLIENTS];
    while (socket_server->listener_running)
    {
        socket_server->epoll_timeout_ms = 1000;
        //epoll_wait
        int count = epoll_wait(socket_server->epollfd, event_list,
                               SOCKET_SERVER_MAX_CLIENTS, socket_server->epoll_timeout_ms);
        if (count < 0)
        {
            LOG_E("epoll error");
            break;
        }
        else if (count == 0)
        {
            //LOG_D("epoll timeout");
            continue;
        }
        if (socket_server->listener_running == false)
        {
            break;
        }
        //直接获取了事件数量,给出了活动的流,这里是和poll区别的关键
        for (int i = 0; i < count; i++)
        {
            if ((event_list[i].events & EPOLLERR) ||
                    (event_list[i].events & EPOLLHUP) ||
                    (event_list[i].events & EPOLLRDHUP) ||//远端close
                    !(event_list[i].events & EPOLLIN))
            {
                LOG_D("epoll client quit, fd=%d, event=0x%X", event_list[i].data.fd, event_list[i].events);
                socket_server->closeClient(event_list[i].data.fd);
                break;
            }
            if (event_list[i].data.fd == socket_server->socketfd)
            {
                socket_server->acceptClient();
            }
            else
            {
                socket_server->recvData(event_list[i].data.fd);
            }
        }
    }
    LOG_I("socket server quit, file_path=%s", socket_server->server_file_name.c_str());
    return;
}

int UnixDomainTcpServer::recvData(int socketfd)
{
    mutex_clients.lock();
    if (clients.count(socketfd) == 0)
    {
        mutex_clients.unlock();
        return -1;
    }
    CLIENT_DES des = clients[socketfd];
    mutex_clients.unlock();
    mutexfds.lock();
    fds.push(des);
    semfds.post();
    mutexfds.unlock();
    return 0;
}

void UnixDomainTcpServer::closeClient(int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    CLIENT_DES des;
    des.clientfd = fd;
    mutex_clients.lock();
    if (clients.count(fd) == 0)
    {
        mutex_clients.unlock();
        return;
    }
    des = clients[fd];
    mutex_clients.unlock();
    onConnectionChanged(des.host, des.clientfd, false);
    bcp_socket_close(fd);
    mutex_clients.lock();
    if (clients.count(fd) != 0)
    {
        clients.erase(fd);
    }
    mutex_clients.unlock();
}

int UnixDomainTcpServer::acceptClient()
{
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_un));
    socklen_t len = sizeof(struct sockaddr_un);
    int clientfd = accept(socketfd, (struct sockaddr*)&client_addr, &len);
    if (clientfd < 0)
    {
        LOG_E("bad accept client");
        return -1;
    }
    LOG_D("Accept Connection, fd=%d, file_path=%s", clientfd, client_addr.sun_path);
    CLIENT_DES des;
    des.clientfd = clientfd;
    des.host = client_addr.sun_path;
    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到EPOLL的监听中
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = clientfd;
    event.events =  EPOLLIN | EPOLLET | EPOLLRDHUP;//边缘触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &event);
    onConnectionChanged(des.host, des.clientfd, true);
    return 0;
}

int UnixDomainTcpServer::startServer()
{
    socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketfd <= 0)
    {
        LOG_E("socket create failed : socketfd = %d", socketfd);
        return -1;
    }
    bcp_file_remove(server_file_name.c_str());
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, server_file_name.c_str());
    //int resuse_flag = 1;
    //setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &resuse_flag, sizeof(int));
    int len = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);
    if (bind(socketfd, (struct sockaddr*)(&server_addr), len) < 0)
    {
        LOG_E("socket bind failed : socketfd = %d", socketfd);
        bcp_socket_close(socketfd);
        return -2;
    }
    if (listen(socketfd, 5) < 0)
    {
        LOG_E("socket listen failed : socketfd = %d", socketfd);
        bcp_socket_close(socketfd);
        return -3;
    }
    // epoll 初始化
    epollfd = epoll_create(SOCKET_SERVER_MAX_CLIENTS);
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    event.data.fd = socketfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) < 0)
    {
        LOG_E("epoll add failed : socketfd = %d", socketfd);
        return -4;
    }
    listener_running = true;
    thread_listener = std::thread(ThreadUnixDomainServerListener, this);
    receiver_running = true;
    thread_receiver = std::thread(ThreadUnixDomainServerReceiver, this);
    return 0;
}

void UnixDomainTcpServer::stopServer()
{
    listener_running = false;
    if (thread_listener.joinable())
    {
        thread_listener.join();
    }
    receiver_running = false;
    if (thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    bcp_socket_close(socketfd);
    socketfd = -1;
    bcp_socket_close(epollfd);
    epollfd = -1;
    mutex_clients.lock();
    for (int i = 0; i < clients.size(); i++)
    {
        bcp_socket_close(clients[i].clientfd);
    }
    mutex_clients.unlock();
    LOG_I("socket server stopped");
}

int UnixDomainTcpServer::send(int clientfd, uint8* data, int data_size)
{
    if (data == NULL || data_size <= 0)
    {
        return 0;
    }
    return bcp_socket_tcp_send(clientfd, data, data_size);
}

int UnixDomainTcpServer::send(const char* client_file_name_wildcard, uint8* data, int data_size)
{
    if (client_file_name_wildcard == NULL || data == NULL || data_size <= 0)
    {
        return 0;
    }
    bool has_wildcard = false;
    for (int i = 0; i < bcp_string_len(client_file_name_wildcard); i++)
    {
        if (client_file_name_wildcard[i] == '?' || client_file_name_wildcard[i] == '*')
        {
            has_wildcard = true;
            break;
        }
    }
    std::vector<int> matched;
    mutex_clients.lock();
    std::map<int, CLIENT_DES>::iterator it;
    for (it = clients.begin(); it != clients.end(); it++)
    {
        CLIENT_DES des = it->second;
        if (bcp_string_match(des.host.c_str(), client_file_name_wildcard))
        {
            matched.push_back(des.clientfd);
            if (has_wildcard == false)
            {
                break;
            }
        }
    }
    mutex_clients.unlock();

    int ret = 0;
    for (int i = 0; i < matched.size(); i++)
    {
        ret = bcp_socket_tcp_send(matched[i], data, data_size);
    }
    return ret;
}

std::string& UnixDomainTcpServer::getServerFileName()
{
    return server_file_name;
}

int UnixDomainTcpServer::getSocketfd()
{
    return socketfd;
}

void UnixDomainTcpServer::onConnectionChanged(std::string& client_file_name, int socketfd, bool connected)
{
}

void UnixDomainTcpServer::onRecv(std::string& client_file_name, int socketfd, uint8* data, int data_size)
{
}

#endif
