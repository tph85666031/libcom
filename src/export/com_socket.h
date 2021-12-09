#ifndef __COM_SOCKET_H__
#define __COM_SOCKET_H__

#include <atomic>
#include "com_base.h"
#include "com_thread.h"
#include "com_err.h"

#if __linux__ == 1
#include "com_socket_linux.h"
#elif defined(__APPLE__)
#include "com_socket_mac.h"
#elif defined(_WIN32) || defined(_WIN64)
#include "com_socket_win.h"
#endif

#define LENGTH_MAC    6

#define SOCKET_SERVER_MAX_CLIENTS 500

class NicInfo
{
public:
    std::string name;
    std::string descritpion;
    uint32 flags;
    int addr_family;
    uint8 mac[LENGTH_MAC];
    std::string ip;
    std::string ip_mask;
    std::string ip_broadcast;
public:
    NicInfo();
    virtual ~NicInfo();
};

void com_socket_set_recv_timeout(int sock, int timeout_ms);
void com_socket_set_send_timeout(int sock, int timeout_ms);

int com_socket_get_tcp_connection_status(int sock);//-1获取失败,0连接断开,1=连接成功
int com_unix_domain_tcp_open(const char* my_name, const char* server_name);
int com_socket_udp_open(const char* interface_name, uint16 recv_port, bool broadcast = false);
int com_socket_tcp_open(const char* remote_host, uint16 remote_port,
                        uint32 timeout_ms = 10000, const char* interface_name = NULL);
int com_socket_udp_send(int socketfd, const char* dest_host, int dest_port,
                        const void* data, int data_size);
int com_socket_tcp_send(int socketfd, const void* data, int data_size);
int com_socket_udp_read(int socketfd, uint8* data, int data_size,
                        uint32 timeout_ms = 10000, char* sender_ip = NULL, int sender_ip_size = 0);
int com_socket_tcp_read(int socketfd, uint8* data, int data_size,
                        uint32 timeout_ms = 10000, bool block_mode = true);
void com_socket_close(int socketfd);

bool com_net_get_mac(const char* interface_name, uint8* mac);
bool com_net_get_nic(const char* interface_name, NicInfo& nic);
//反向原地址可达性校验，0=不校验，1=严格校验，2=宽松校验
void com_net_set_rpfilter(const char* interface_name, uint8 flag);
uint8 com_net_get_rpfilter(const char* interface_name);

bool com_net_is_interface_exist(const char* interface_name);
std::vector<std::string> com_net_get_interface_all();

class Socket
{
public:
    Socket();
    virtual ~Socket();
    void setHost(const char* host);
    void setPort(uint16 port);
    void setSocketfd(int fd);
    void setInterface(const char* interface_name);
    std::string getHost();
    uint16 getPort();
    int getSocketfd();
    std::string getInterface();
protected:
    std::atomic<uint16> port;
    std::atomic<int> socketfd;
    std::string interface_name;
private:
    CPPMutex mutxe;
    std::string host;
};

class SocketTcpClient : public Socket
{
public:
    SocketTcpClient();
    SocketTcpClient(const char* host, uint16 port);
    virtual ~SocketTcpClient();
    virtual bool startClient();
    virtual void stopClient();
    int send(const void* data, int data_size);
    bool isConnected();
    void reconnect();
    void setReconnectInterval(int reconnect_interval_ms);
protected:
    virtual void onConnectionChanged(bool connected);
    virtual void onRecv(uint8* data, int data_size);
private:
    static void ThreadSocketClientRunner(SocketTcpClient* socket_client);
private:
    std::atomic<bool> running;
    std::thread thread_runner;
    std::atomic<bool> reconnect_at_once;
    std::atomic<int> reconnect_interval_ms = {1000};
    std::atomic<bool> connected;
};

class UnixDomainTcpClient
{
public:
    UnixDomainTcpClient(const char* server_file_name, const char* file_name);
    virtual ~UnixDomainTcpClient();
    int startClient();
    void stopClient();
    int send(const void* data, int data_size);
    void reconnect();
    virtual void onConnectionChanged(bool connected);
    virtual void onRecv(uint8* data, int data_size);
    std::string& getFileName();
    std::string& getServerFileName();
    int getSocketfd();
private:
    bool connect();
    static void ThreadUnixDomainClientReceiver(UnixDomainTcpClient* client);
private:
    std::atomic<int> socketfd;
    std::string file_name;
    std::string server_file_name;
    std::atomic<bool> receiver_running;
    std::thread thread_receiver;
    std::atomic<bool> need_reconnect;
};

#endif /* __COM_SOCKET_H__ */
