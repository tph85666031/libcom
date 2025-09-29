#ifndef __COM_SOCKET_LINUX_H__
#define __COM_SOCKET_LINUX_H__

#if __linux__ == 1
#include "com_base.h"
#include "com_thread.h"

class COM_EXPORT ComTcpServer: public ComThreadPool<Message>
{
public:
    ComTcpServer();
    ComTcpServer(uint16 port);
    virtual ~ComTcpServer();
    ComTcpServer& setPort(uint16 port);
    int startServer();
    void stopServer();
    void closeClient(int fd);
    int send(int clientfd, const void* data, int data_size);
    int send(const char* host, uint16 port, const void* data, int data_size);
    virtual void broadcast(const void* data, int data_size);
protected:
    virtual void onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected);
    virtual void onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size);
private:
    int acceptClient();
    void threadPoolRunner(Message& msg);
    static void ThreadListener(ComTcpServer* ctx);
private:
    uint16 server_port;
    int server_fd;
    int epollfd;
    std::atomic<bool> thread_listener_running;
    int epoll_timeout_ms;
    std::thread thread_listener;
    std::mutex mutex_clients;
    std::map<int, SOCKET_CLIENT_DES> clients;
};

class COM_EXPORT ComUnixDomainServer
{
public:
    ComUnixDomainServer(const char* server_file_name);
    virtual ~ComUnixDomainServer();
    int startServer();
    void stopServer();
    int send(const char* client_file_name_wildcard, const void* data, int data_size);
    int send(int clientfd, const void* data, int data_size);
    std::string& getServerFileName();
    int getSocketfd();
    virtual void broadcast(const void* data, int data_size);
protected:
    virtual void onConnectionChanged(std::string& client_file_name, int socketfd, bool connected);
    virtual void onRecv(std::string& client_file_name, int socketfd, uint8* data, int data_size);
private:
    int acceptClient();
    void closeClient(int fd);
    int recvData(int clientfd);
    static void ThreadReceiver(ComUnixDomainServer* socket_server);
    static void ThreadListener(ComUnixDomainServer* socket_server);
private:
    std::string server_file_name;
    int server_fd;
    int epollfd;
    std::atomic<bool> thread_listener_running;
    std::atomic<bool> thread_receiver_running;
    int epoll_timeout_ms;
    std::thread thread_listener;
    std::thread thread_receiver;
    std::mutex mutex_clients;
    std::map<int, SOCKET_CLIENT_DES> clients;
    ComMutex mutexfds;
    ComSem semfds;
    std::queue<SOCKET_CLIENT_DES> fds;
};

#endif

#endif /* __COM_SOCKET_LINUX_H__ */

