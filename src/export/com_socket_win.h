#ifndef __COM_SOCKET_LINUX_H__
#define __COM_SOCKET_LINUX_H__

#if defined(_WIN32) || defined(_WIN64)
#include "com_base.h"

class SocketTcpServer
{
public:
    SocketTcpServer();
    SocketTcpServer(uint16 port);
    virtual ~SocketTcpServer();
    virtual int startServer();
    virtual void stopServer();
    void closeClient(int fd);
    int send(int clientfd, uint8* data, int data_size);
    int send(const char* host, uint16 port, uint8* data, int data_size);
    virtual void onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected);
    virtual void onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size);
};

class UnixDomainTcpServer
{
public:
    UnixDomainTcpServer(const char* server_file_name)
    {
        return;
    };
    virtual ~UnixDomainTcpServer()
    {
        return;
    };
    int startServer()
    {
        return -1;
    };
    void stopServer()
    {
        return;
    };
    int send(const char* client_file_name_wildcard, uint8* data, int data_size)
    {
        return -1;
    };
    int send(int clientfd, uint8* data, int data_size)
    {
        return -1;
    };
    std::string& getServerFileName()
    {
        return server_file_name;
    };
    int getSocketfd()
    {
        return -1;
    };
    virtual void onConnectionChanged(std::string& client_file_name, int socketfd, bool connected)
    {
        return;
    };
    virtual void onRecv(std::string& client_file_name, int socketfd, uint8* data, int data_size)
    {
        return;
    };
private:
    std::string server_file_name;
};

#endif

#endif /* __COM_SOCKET_LINUX_H__ */

