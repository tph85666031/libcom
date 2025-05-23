#ifndef __COM_SOCKET_WIN_H__
#define __COM_SOCKET_WIN_H__

#if defined(_WIN32) || defined(_WIN64)
#include "com_base.h"
#include "com_log.h"

class COM_EXPORT ComTcpServer
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
    int recvData(int clientfd);
    static void ThreadReceiver(ComTcpServer* ctx);
    static void ThreadListener(ComTcpServer* ctx);
private:
    uint16 server_port;
    int server_fd;
    int max_fd;
    std::mutex mutex_select_fd;
    fd_set select_fd;
    std::atomic<bool> thread_listener_running;
    std::atomic<bool> thread_receiver_running;
    int select_timeout_ms;
    std::thread thread_listener;
    std::thread thread_receiver;
    std::mutex mutex_clients;
    std::map<int, SOCKET_CLIENT_DES> clients;
    std::mutex mutexfds;
    ComSem semfds;
    std::deque<SOCKET_CLIENT_DES> ready_fds;
};

//以下留空即可，win不支持
class COM_EXPORT ComUnixDomainServer
{
public:
    ComUnixDomainServer(const char* server_file_name)
    {
        LOG_E("not support");
        return;
    };
    virtual ~ComUnixDomainServer()
    {
        LOG_E("not support");
        return;
    };
    int startServer()
    {
        LOG_E("not support");
        return -1;
    };
    void stopServer()
    {
        LOG_E("not support");
        return;
    };
    int send(const char* client_file_name_wildcard, const void* data, int data_size)
    {
        LOG_E("not support");
        return -1;
    };
    int send(int clientfd, const void* data, int data_size)
    {
        LOG_E("not support");
        return -1;
    };
    std::string& getServerFileName()
    {
        LOG_E("not support");
        return server_file_name;
    };
    int getSocketfd()
    {
        LOG_E("not support");
        return -1;
    };
    virtual void onConnectionChanged(std::string& client_file_name, int socketfd, bool connected)
    {
        LOG_E("not support");
        return;
    };
    virtual void onRecv(std::string& client_file_name, int socketfd, uint8* data, int data_size)
    {
        LOG_E("not support");
        return;
    };
private:
    std::string server_file_name;
};

#endif

#endif /* __COM_SOCKET_WIN_H__ */

