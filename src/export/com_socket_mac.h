#ifndef __COM_SOCKET_LINUX_H__
#define __COM_SOCKET_LINUX_H__

#if defined(__APPLE__)
#include "com_base.h"

typedef struct
{
    std::string host;
    uint16 port;
    int clientfd;
} CLIENT_DES;

class TCPServer
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
    uint16 server_port;
    int server_fd;
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
private:
    int server_fd;
    std::string server_file_name;
};
#endif

#endif /* __COM_SOCKET_LINUX_H__ */

