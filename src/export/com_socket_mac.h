#ifndef __COM_SOCKET_MAC_H__
#define __COM_SOCKET_MAC_H__

#if defined(__APPLE__)
#include "com_base.h"

class TCPServer
{
public:
    TCPServer();
    virtual ~TCPServer();
public:
    bool IOMCreate();
    void IOMAddMonitor(int clientfd);
    void IOMRemoveMonitor(int fd);
    int IOMWaitFor(struct kevent* event_list, int event_len);
public:
    virtual int startServer();
    virtual bool initListen()
    {
        return false;
    }
    virtual int acceptClient()
    {
        return -1;
    }
    virtual void closeClient(int fd);
    virtual void stopServer();
    virtual int send(int clientfd, const void* data, int data_size);
    virtual void broadcast(const void* data, int data_size);
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
    std::mutex mutex_clients;
    std::map<int, SOCKET_CLIENT_DES> clients;
    std::mutex mutexfds;
    ComSem semfds;
    std::queue<SOCKET_CLIENT_DES> ready_fds;
};

class ComTcpServer : public TCPServer
{
public:
    ComTcpServer();
    ComTcpServer(uint16 port);
    virtual ~ComTcpServer();
    ComTcpServer& setPort(uint16 port);
public:
    virtual bool initListen() override;
    virtual int acceptClient() override;
    int send(int clientfd, const void* data, int data_size) override;
    int send(const char* host, uint16 port, const void* data, int data_size);
    virtual void broadcast(const void* data, int data_size) override;
private:
    static void ThreadDispatcher(ComTcpServer* socket_server);
};

class ComUnixDomainServer : public TCPServer
{
public:
    ComUnixDomainServer(const char* server_file_name);
    virtual ~ComUnixDomainServer();
public:
    virtual bool initListen() override;
    virtual int acceptClient() override;
    int send(int clientfd, const void* data, int data_size) override;
    int send(const char* client_file_name_wildcard, const void* data, int data_size);
    virtual void broadcast(const void* data, int data_size) override;
private:
    std::string server_file_name;
};
#endif

#endif /* __COM_SOCKET_MAC_H__ */

