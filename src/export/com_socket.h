#ifndef __COM_SOCKET_H__
#define __COM_SOCKET_H__

#include <atomic>
#include "com_base.h"
#include "com_thread.h"
#include "com_err.h"

#if defined(__APPLE__)
typedef struct kevent IOM_EVENT;
#define iom_event_fd(x)         (int)(intptr_t)((x).udata)
#define iom_event_events(x)     (x).filter
#define iom_error_events(x)     (!((x) & EVFILT_READ))
#else
typedef struct epoll_event IOM_EVENT;
#define iom_event_fd(x)         (x).data.fd
#define iom_event_events(x)     (x).events
#define iom_error_events(x)     (((x) & EPOLLERR) || ((x) & EPOLLHUP) || ((x) & EPOLLRDHUP) || !((x) & EPOLLIN))
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

int com_socket_get_tcp_connection_status(int sock);
int com_unix_domain_tcp_open(const char* my_name, const char* server_name);
int com_socket_udp_open(const char* interface_name, uint16 recv_port, bool broadcast = false);
int com_socket_tcp_open(const char* remote_host, uint16 remote_port,
                        uint32 timeout_ms = 10000, const char* interface_name = NULL);
int com_socket_udp_send(int socketfd, const char* dest_host, int dest_port,
                        void* data, int data_size);
int com_socket_tcp_send(int socketfd, void* data, int data_size);
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

typedef struct
{
    std::string host;
    uint16 port;
    int clientfd;
} CLIENT_DES;

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
    virtual int startClient();
    virtual void stopClient();
    int send(uint8* data, int data_size);
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
    int send(uint8* data, int data_size);
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



// SERVER
class TCPServer : public Socket
{
public:
    TCPServer();
    virtual ~TCPServer();
public:
    bool IOMCreate();
    void IOMAddMonitor(int clientfd);
    void IOMRemoveMonitor(int fd);
    int IOMWaitFor(IOM_EVENT *event_list, int event_len);
public:
    virtual int startServer();
    virtual bool initListen() { return false; }
    virtual int acceptClient() { return -1; }
    virtual void closeClient(int fd);
    virtual void stopServer();
    virtual int send(int clientfd, uint8* data, int data_size);
    virtual int recvData(int clientfd);
    virtual void onConnectionChanged(std::string& host_or_file, uint16 port, int socketfd, bool connected) {}
    virtual void onRecv(std::string& host_or_file, uint16 port, int socketfd, uint8* data, int data_size) {}
public:
    static void ThreadTCPServerReceiver(TCPServer* socket_server);
    static void ThreadTCPServerListener(TCPServer* socket_server);
protected:
    int epollfd;
    std::atomic<bool> receiver_running;
    std::atomic<bool> listener_running;
    int epoll_timeout_ms;
    std::thread thread_listener;
    std::thread thread_receiver;
    CPPMutex mutex_clients;
    std::map<int, CLIENT_DES> clients;
    CPPMutex mutexfds;
    CPPSem semfds;
    std::queue<CLIENT_DES> ready_fds;
};

class SocketTcpServer : public TCPServer
{
public:
    SocketTcpServer(uint16 port);
    virtual ~SocketTcpServer();
public:
    virtual bool initListen() override;
    virtual int acceptClient() override;
    int send(const char* host, uint16 port, uint8* data, int data_size);
private:
    static void ThreadSocketServerDispatcher(SocketTcpServer* socket_server);
};

class UnixDomainTcpServer : public TCPServer
{
public:
    UnixDomainTcpServer(const char* server_file_name);
    virtual ~UnixDomainTcpServer();
public:
    virtual bool initListen() override;
    virtual int acceptClient() override;
    int send(const char* client_file_name_wildcard, uint8* data, int data_size);
};

#endif /* __COM_SOCKET_H__ */
