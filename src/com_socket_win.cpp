#if defined(_WIN32) || defined(_WIN64)
#include "com_socket.h"

ComTcpServer::ComTcpServer()
{
    com_socket_global_init();
}

ComTcpServer::ComTcpServer(uint16 port)
{
    this->server_port = port;
}

ComTcpServer::~ComTcpServer()
{
    stopServer();
    com_socket_global_uninit();
}

ComTcpServer& ComTcpServer::setPort(uint16 port)
{
    this->server_port = port;
    return *this;
}

void ComTcpServer::ThreadReceiver(ComTcpServer* ctx)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
//    LOG_D("Tcp Server Receiver Enter");

    uint8 buf[4096];
    while(ctx->thread_receiver_running)
    {
        ctx->semfds.wait(1000);
        ctx->mutexfds.lock();
        if(ctx->ready_fds.size() <= 0)
        {
            ctx->mutexfds.unlock();
//            LOG_D("server receiver, ready fds is empty");
            continue;
        }
//        LOG_D("Tcp Server, Receiver ready_fds before pop, has content count:%d ", ctx->ready_fds.size());
        SOCKET_CLIENT_DES des = ctx->ready_fds.front();
        ctx->ready_fds.pop_front();
//        LOG_D("Tcp Server, Receiver get des fd:%d, after pop, has content count:%d", des.clientfd, ctx->ready_fds.size());
        ctx->mutexfds.unlock();
        while(true)
        {
            memset(buf, 0, sizeof(buf));

            int ret = recv(des.clientfd, (char*)buf, sizeof(buf), 0);
//            LOG_D("server receiver, recv fd:%d buffer, return: %d", des.clientfd, ret);
            if(ret == 0)
            {
                ctx->closeClient(des.clientfd);
                break;
            }
            else if(ret < 0)
            {

                int error = WSAGetLastError();
//                LOG_D("server receiver, recv fd:%d return: %d, error:%d", des.clientfd, ret, error);
                if(error == WSAECONNRESET || error == WSAECONNABORTED || error == WSAETIMEDOUT || error == WSAENOTSOCK)
                {
                    ctx->closeClient(des.clientfd);
                }

                break;
            }
            ctx->onRecv(des.host, des.port, des.clientfd, buf, ret);
        }
//        LOG_D("Tcp Server, Receiver end des fd:%d", des.clientfd);
    }
    return;
}

void ComTcpServer::ThreadListener(ComTcpServer* ctx)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }

    ctx->mutex_select_fd.lock();
    FD_ZERO(&ctx->select_fd);
    FD_SET(ctx->server_fd, &ctx->select_fd);
    ctx->mutex_select_fd.unlock();

    while(ctx->thread_listener_running)
    {
        ctx->select_timeout_ms = 1000;
        ctx->mutex_select_fd.lock();
        fd_set select_fd_bak = ctx->select_fd;
        ctx->mutex_select_fd.unlock();
        struct timeval st;
        st.tv_sec = ctx->select_timeout_ms / 1000;//seconds
        st.tv_usec = ctx->select_timeout_ms % 1000 * 1000;//microsecond
        int ret = select(ctx->max_fd + 1, &select_fd_bak, 0, 0, &st);
        if(ret < 0)
        {
            LOG_E("select error,errno=%d:%s", errno, strerror(errno));
            break;
        }
        else if(ret == 0)
        {
            //LOG_D("epoll timeout");
            continue;
        }
        if(ctx->thread_listener_running == false)
        {
            break;
        }

        for(int i = 0; i < select_fd_bak.fd_count; i++)
        {
            if(FD_ISSET(select_fd_bak.fd_array[i], &select_fd_bak) == false)
            {
//                LOG_D("server listener, fd is not in set:%d", select_fd_bak.fd_array[i]);
                continue;
            }
            if(select_fd_bak.fd_array[i] == ctx->server_fd)
            {
                ctx->acceptClient();
            }
            else
            {
                ctx->recvData(select_fd_bak.fd_array[i]);
            }
        }
    }
//    LOG_I("socket server quit, server_port=%d", ctx->server_port);
    return;
}

int ComTcpServer::recvData(int clientfd)//ThreadSocketServerRunner线程
{
    mutex_clients.lock();
    if(clients.count(clientfd) == 0)
    {
//        LOG_W("Recv Data By fd, there is no fd client:%d", clientfd);
        mutex_clients.unlock();
        return -1;
    }
//    LOG_D("Recv Data By fd, get fd client:%d, all clients count:%d", clientfd, clients.size());
    SOCKET_CLIENT_DES des = clients[clientfd];
    mutex_clients.unlock();
    mutexfds.lock();
//    LOG_D("Recv Data By fd, before push des fd:%d, ready_fds count is :%d", clientfd, ready_fds.size());
    for(size_t i = 0; i < ready_fds.size(); i++)
    {
        if(ready_fds[i].clientfd == clientfd)
        {
            mutexfds.unlock();
            return 0;
        }
    }
    ready_fds.push_back(des);
    int after_ount = ready_fds.size();
    semfds.post();
    mutexfds.unlock();
//    LOG_D("Recv Data By fd, return 0, fd:%d , after push, ready_fds count is:%d", clientfd, after_ount);
    return 0;
}

void ComTcpServer::closeClient(int fd)
{
//    LOG_D("Tcp Server Close fd:%d", fd);
    mutex_select_fd.lock();
    FD_CLR(fd, &select_fd);
    mutex_select_fd.unlock();

    SOCKET_CLIENT_DES des;
    des.clientfd = fd;
    mutex_clients.lock();
    if(clients.count(fd) == 0)
    {
//        LOG_W("Tcp Server Close fd, there is no client for fd:%d", fd);
        mutex_clients.unlock();
        return;
    }
    des = clients[fd];
    mutex_clients.unlock();
    onConnectionChanged(des.host, des.port, des.clientfd, false);
    com_socket_close(fd);
    mutex_clients.lock();
    if(clients.count(fd) != 0)
    {
        clients.erase(fd);
    }
    mutex_clients.unlock();
}

int ComTcpServer::acceptClient()
{
    struct sockaddr_in sin;
    int len = sizeof(struct sockaddr_in);
    memset(&sin, 0, len);
    int clientfd = accept(server_fd, (struct sockaddr*)&sin, &len);
    if(clientfd < 0)
    {
        LOG_E("bad accept client, errno=%d:%s", errno, strerror(errno));
        return -1;
    }

    if(clientfd > max_fd)
    {
        max_fd = clientfd;
    }

    LOG_D("Accept Connection, fd=%d, addr=%s,server_port=%u",
          clientfd, com_string_from_ipv4(sin.sin_addr.s_addr).c_str(), sin.sin_port);
    SOCKET_CLIENT_DES des;
    des.clientfd = clientfd;
    des.host = com_string_from_ipv4(sin.sin_addr.s_addr);
    des.port = sin.sin_port;
    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到SELECT的监听中
    mutex_select_fd.lock();
    FD_SET(clientfd, &select_fd);
    mutex_select_fd.unlock();
    onConnectionChanged(des.host, des.port, des.clientfd, true);
    return 0;
}

int ComTcpServer::startServer()
{
    if(server_port == 0)
    {
        LOG_E("server_port not set");
        return -1;
    }
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd <= 0)
    {
        LOG_E("socket create failed : fd = %d", server_fd);
        return -1;
    }
    max_fd = server_fd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family  =  AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);
    char resuse_flag = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &resuse_flag, sizeof(int));
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        LOG_E("socket bind failed : fd = %d", server_fd);
        com_socket_close(server_fd);
        return -2;
    }
    if(listen(server_fd, 5) < 0)
    {
        LOG_E("socket listen failed : fd = %d", server_fd);
        com_socket_close(server_fd);
        return -3;
    }
    //com_socket_set_recv_timeout(server_fd, 0);
    unsigned long none_block = 1;
    ioctlsocket(server_fd, FIONBIO, &none_block);
    thread_listener_running = true;
    thread_listener = std::thread(ThreadListener, this);
    thread_receiver_running = true;
    thread_receiver = std::thread(ThreadReceiver, this);
    return 0;
}

void ComTcpServer::stopServer()
{
    thread_listener_running = false;
    if(thread_listener.joinable())
    {
        thread_listener.join();
    }
    thread_receiver_running = false;
    if(thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    com_socket_close(server_fd);
    server_fd = -1;
    mutex_clients.lock();
    for(size_t i = 0; i < clients.size(); i++)
    {
        com_socket_close(clients[i].clientfd);
    }
    mutex_clients.unlock();
    LOG_I("socket server stopped");
}

int ComTcpServer::send(const char* host, uint16 port, const void* data, int data_size)
{
    LOG_D("enter");
    if(host == NULL || data == NULL || data_size <= 0)
    {
        LOG_D("return 0");
        return 0;
    }

    mutex_clients.lock();
    int clientfd = -1;
    std::map<int, SOCKET_CLIENT_DES>::iterator it;
    for(it = clients.begin(); it != clients.end(); it++)
    {
        SOCKET_CLIENT_DES des = it->second;
        if(com_string_equal(des.host.c_str(), host) && des.port == port)
        {
            clientfd = des.clientfd;
            break;
        }
    }
    mutex_clients.unlock();
    if(clientfd == -1)
    {
        LOG_D("fd is -1, return");
        return -1;
    }
    int ret = com_socket_tcp_send(clientfd, data, data_size);
    LOG_D("tcp server, call tcp send to fd: %d return:%d", clientfd, ret);
    if(ret == -1)
    {
        LOG_D("tcp server, call tcp send to fd: %d failure and close it", clientfd);
        closeClient(clientfd);
    }
    LOG_D("return %d", ret);
    return ret;
}

int ComTcpServer::send(int clientfd, const void* data, int data_size)
{
    LOG_D("enter");
    if(data == NULL || data_size <= 0)
    {
        LOG_D("data invalidate, return 0");
        return 0;
    }
    int ret = com_socket_tcp_send(clientfd, data, data_size);
    if(ret == -1)
    {
        LOG_D("send failure, fd:%d return:", clientfd, ret);
        closeClient(clientfd);
    }
    LOG_D("return %d", ret);
    return ret;
}

void ComTcpServer::onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected)
{
}

void ComTcpServer::onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size)
{
}

void ComTcpServer::broadcast(const void* data, int data_size)
{
    mutex_clients.lock();
    for(auto iter : clients)
    {
        int ret = com_socket_tcp_send(iter.first, data, data_size);
        if(ret < 0)
        {
            LOG_E("send data_size=%d to fd=%d, failed!", data_size, iter.first);
        }
        else
        {
            LOG_T("send data_size=%d to fd=%d, success!", data_size, iter.first);
        }
    }
    mutex_clients.unlock();
}

#endif
