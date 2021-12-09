#ifndef __COM_SOCKET_LINUX_H__
#define __COM_SOCKET_LINUX_H__

#if __linux__ == 1
#include "com_base.h"

typedef struct
{
    std::string host;
    uint16 port;
    int clientfd;
} CLIENT_DES;

class SocketTcpServer
{
public:
    SocketTcpServer();
    SocketTcpServer(uint16 port);
    virtual ~SocketTcpServer();
    SocketTcpServer& setPort(uint16 port);
    virtual int startServer();
    virtual void stopServer();
    void closeClient(int fd);
    int send(int clientfd, const void* data, int data_size);
    int send(const char* host, uint16 port, const void* data, int data_size);
    virtual void onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected);
    virtual void onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size);
private:
    int acceptClient();
    int recvData(int clientfd);
    static void ThreadSocketServerReceiver(SocketTcpServer* socket_server);
    static void ThreadSocketServerListener(SocketTcpServer* socket_server);
    static void ThreadSocketServerDispatcher(SocketTcpServer* socket_server);
private:
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

class UnixDomainTcpServer
{
public:
    UnixDomainTcpServer(const char* server_file_name);
    virtual ~UnixDomainTcpServer();
    int startServer();
    void stopServer();
    int send(const char* client_file_name_wildcard, uint8* data, int data_size);
    int send(int clientfd, uint8* data, int data_size);
    std::string& getServerFileName();
    int getSocketfd();
    virtual void onConnectionChanged(std::string& client_file_name, int socketfd, bool connected);
    virtual void onRecv(std::string& client_file_name, int socketfd, uint8* data, int data_size);
private:
    int acceptClient();
    void closeClient(int fd);
    int recvData(int clientfd);
    static void ThreadUnixDomainServerReceiver(UnixDomainTcpServer* socket_server);
    static void ThreadUnixDomainServerListener(UnixDomainTcpServer* socket_server);
private:
    std::string server_file_name;
    int server_fd;
    int epollfd;
    std::atomic<bool> listener_running;
    std::atomic<bool> receiver_running;
    int epoll_timeout_ms;
    std::thread thread_listener;
    std::thread thread_receiver;
    CPPMutex mutex_clients;
    std::map<int, CLIENT_DES> clients;
    CPPMutex mutexfds;
    CPPSem semfds;
    std::queue<CLIENT_DES> fds;
};

#endif

#endif /* __COM_SOCKET_LINUX_H__ */

