#if defined(_WIN32) || defined(_WIN64)
#include "coM_socket_win.h"

SocketTcpServer::SocketTcpServer()
{
}

SocketTcpServer::SocketTcpServer(uint16 port)
{
    this->server_port = port;
}

SocketTcpServer::~SocketTcpServer()
{
    stopServer();
}

SocketTcpServer& SocketTcpServer::setPort(uint16 port)
{
    this->server_port = port;
    return *this;
}

int SocketTcpServer::startServer()
{
    //开启此server，初始化server socket并listene,accept等
    return -1;
}

void SocketTcpServer::stopServer()
{
    //停止此server
}

void SocketTcpServer::closeClient(int fd)
{
    //关闭到指定client的链接
}

int SocketTcpServer::send(int clientfd, const void* data, int data_size)
{
    //向指定client发送数据，返回实际发送的字节数,-1表示发送失败
    return -1;
}

int SocketTcpServer::send(const char* host, uint16 port, const void* data, int data_size)
{
    //向指定client发送数据(host：port为指定client的地址和端口号)，返回实际发送的字节数,-1表示发送失败
    return -1;
}

#endif
