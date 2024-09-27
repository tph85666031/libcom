#if __linux__ == 1
#include <sys/epoll.h>
#include <sys/un.h>
#include <unistd.h>

#include "com_socket.h"
#include "com_serializer.h"
#include "com_thread.h"
#include "com_file.h"
#include "com_log.h"
#include <errno.h>

ComTcpServer::ComTcpServer()
{
    server_port = 0;
    server_fd = -1;
    epollfd = -1;
    thread_listener_running = false;
    epoll_timeout_ms = 3000;
    setAllowDuplicateMessage(false);
}

ComTcpServer::ComTcpServer(uint16 port)
{
    server_port = port;
    server_fd = -1;
    epollfd = -1;
    thread_listener_running = false;
    epoll_timeout_ms = 3000;
}

ComTcpServer::~ComTcpServer()
{
    stopServer();
}

ComTcpServer& ComTcpServer::setPort(uint16 port)
{
    this->server_port = port;
    return *this;
}

void ComTcpServer::threadPoolRunner(Message& msg)
{
    int clientfd = (int)msg.getID();
    std::string host = msg.getString("host");
    uint16 port = msg.getUInt16("port");
    uint8 buf[4096];
    while(true)
    {
        memset(buf, 0, sizeof(buf));
        int ret = recv(clientfd, buf, sizeof(buf), MSG_DONTWAIT);
        if(ret == 0)
        {
            break;
        }
        else if(ret < 0)
        {
            int err_code = errno;
            if(err_code != EAGAIN && err_code != EWOULDBLOCK && err_code != EINTR)
            {
                closeClient(clientfd);
            }
            break;
        }
        onRecv(host, port, clientfd, buf, ret);
    }
}

void ComTcpServer::ThreadListener(ComTcpServer* ctx)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
    struct epoll_event event_list[SOCKET_SERVER_MAX_CLIENTS];
    while(ctx->thread_listener_running)
    {
        ctx->epoll_timeout_ms = 1000;
        //epoll_wait
        int count = epoll_wait(ctx->epollfd, event_list, SOCKET_SERVER_MAX_CLIENTS, ctx->epoll_timeout_ms);
        if(count < 0)
        {
            LOG_E("epoll error,errno=%d:%s", errno, strerror(errno));
            break;
        }
        else if(count == 0)
        {
            //LOG_D("epoll timeout");
            continue;
        }
        if(ctx->thread_listener_running == false)
        {
            break;
        }
        //直接获取了事件数量,给出了活动的流,这里是和poll区别的关键
        for(int i = 0; i < count; i++)
        {
            if((event_list[i].events & EPOLLERR) ||
                    (event_list[i].events & EPOLLHUP) ||
                    (event_list[i].events & EPOLLRDHUP) ||//远端close
                    !(event_list[i].events & EPOLLIN))
            {
                LOG_D("epoll client quit, fd=%d, event=0x%X", event_list[i].data.fd, event_list[i].events);
                ctx->closeClient(event_list[i].data.fd);
                break;
            }
            if(event_list[i].data.fd == ctx->server_fd)
            {
                ctx->acceptClient();
            }
            else
            {
                ctx->mutex_clients.lock();
                auto it = ctx->clients.find(event_list[i].data.fd);
                if(it != ctx->clients.end())
                {
                    SOCKET_CLIENT_DES des = ctx->clients[event_list[i].data.fd];
                    Message msg(it->first);
                    msg.set("host", it->second.host);
                    msg.set("port", it->second.port);
                    ctx->pushPoolMessage(msg);
                }
                ctx->mutex_clients.unlock();
            }
        }
    }
    LOG_I("socket server quit, server_port=%d", ctx->server_port);
    return;
}

void ComTcpServer::closeClient(int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    SOCKET_CLIENT_DES des;
    des.clientfd = fd;
    mutex_clients.lock();
    if(clients.count(fd) == 0)
    {
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

int ComTcpServer::acceptClient()//ThreadSocketServerRunner线程
{
    struct sockaddr_in sin;
    socklen_t len = sizeof(struct sockaddr_in);
    memset(&sin, 0, len);
    int clientfd = accept(server_fd, (struct sockaddr*)&sin, &len);
    if(clientfd < 0)
    {
        LOG_E("bad accept client, errno=%d:%s", errno, strerror(errno));
        return -1;
    }
    LOG_D("Accept Connection, fd=%d, addr=%s,server_port=%u",
          clientfd, com_ipv4_to_string(sin.sin_addr.s_addr).c_str(), sin.sin_port);
    SOCKET_CLIENT_DES des;
    des.clientfd = clientfd;
    des.host = com_ipv4_to_string(sin.sin_addr.s_addr);
    des.port = ntohs(sin.sin_port);
    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到EPOLL的监听中
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = clientfd;
    event.events =  EPOLLIN | EPOLLET | EPOLLRDHUP;//边缘触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &event);
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
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);
    int resuse_flag = 1;
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
    // epoll 初始化
    epollfd = epoll_create(SOCKET_SERVER_MAX_CLIENTS);
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &event) < 0)
    {
        LOG_E("epoll add failed : fd = %d", server_fd);
        return -4;
    }
    startThreadPool();
    thread_listener_running = true;
    thread_listener = std::thread(ThreadListener, this);
    LOG_I("start done");
    return 0;
}

void ComTcpServer::stopServer()
{
    stopThreadPool();
    thread_listener_running = false;
    if(thread_listener.joinable())
    {
        thread_listener.join();
    }
    com_socket_close(server_fd);
    server_fd = -1;
    close(epollfd);
    epollfd = -1;
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
    if(host == NULL || data == NULL || data_size <= 0)
    {
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
        return -1;
    }
    int ret = com_socket_tcp_send(clientfd, data, data_size);
    if(ret == -1)
    {
        closeClient(clientfd);
    }
    return ret;
}

int ComTcpServer::send(int clientfd, const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return 0;
    }
    int ret = com_socket_tcp_send(clientfd, data, data_size);
    if(ret == -1)
    {
        closeClient(clientfd);
    }
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

ComUnixDomainServer::ComUnixDomainServer(const char* server_file_name)
{
    if(server_file_name != NULL)
    {
        this->server_file_name = server_file_name;
    }
    server_fd = -1;
    epollfd = -1;
    thread_listener_running = false;
    thread_receiver_running = false;
    epoll_timeout_ms = 3000;
}

ComUnixDomainServer::~ComUnixDomainServer()
{
    stopServer();
    com_file_remove(getServerFileName().c_str());
}

void ComUnixDomainServer::ThreadReceiver(ComUnixDomainServer* ctx)
{
    uint8 buf[4096];
    while(ctx->thread_receiver_running)
    {
        ctx->semfds.wait(1000);
        ctx->mutexfds.lock();
        if(ctx->fds.size() <= 0)
        {
            ctx->mutexfds.unlock();
            continue;
        }

        SOCKET_CLIENT_DES des = ctx->fds.front();
        ctx->fds.pop();
        ctx->mutexfds.unlock();
        while(true)
        {
            memset(buf, 0, sizeof(buf));
            int ret = recv(des.clientfd, buf, sizeof(buf), MSG_DONTWAIT);
            if(ret <= 0)
            {
                break;
            }
            ctx->onRecv(des.host, des.clientfd, buf, ret);
        }
    }
    return;
}

void ComUnixDomainServer::ThreadListener(ComUnixDomainServer* ctx)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
    struct epoll_event event_list[SOCKET_SERVER_MAX_CLIENTS];
    while(ctx->thread_listener_running)
    {
        ctx->epoll_timeout_ms = 1000;
        //epoll_wait
        int count = epoll_wait(ctx->epollfd, event_list, SOCKET_SERVER_MAX_CLIENTS, ctx->epoll_timeout_ms);
        if(count < 0)
        {
            LOG_E("epoll error");
            break;
        }
        else if(count == 0)
        {
            //LOG_D("epoll timeout");
            continue;
        }
        if(ctx->thread_listener_running == false)
        {
            break;
        }
        //直接获取了事件数量,给出了活动的流,这里是和poll区别的关键
        for(int i = 0; i < count; i++)
        {
            if((event_list[i].events & EPOLLERR) ||
                    (event_list[i].events & EPOLLHUP) ||
                    (event_list[i].events & EPOLLRDHUP) ||//远端close
                    !(event_list[i].events & EPOLLIN))
            {
                LOG_D("epoll client quit, fd=%d, event=0x%X", event_list[i].data.fd, event_list[i].events);
                ctx->closeClient(event_list[i].data.fd);
                break;
            }
            if(event_list[i].data.fd == ctx->server_fd)
            {
                ctx->acceptClient();
            }
            else
            {
                ctx->recvData(event_list[i].data.fd);
            }
        }
    }
    LOG_I("socket server quit, file_path=%s", ctx->server_file_name.c_str());
    return;
}

int ComUnixDomainServer::recvData(int socketfd)
{
    mutex_clients.lock();
    if(clients.count(socketfd) == 0)
    {
        mutex_clients.unlock();
        return -1;
    }
    SOCKET_CLIENT_DES des = clients[socketfd];
    mutex_clients.unlock();
    mutexfds.lock();
    fds.push(des);
    semfds.post();
    mutexfds.unlock();
    return 0;
}

void ComUnixDomainServer::closeClient(int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    SOCKET_CLIENT_DES des;
    des.clientfd = fd;
    mutex_clients.lock();
    if(clients.count(fd) == 0)
    {
        mutex_clients.unlock();
        return;
    }
    des = clients[fd];
    mutex_clients.unlock();
    onConnectionChanged(des.host, des.clientfd, false);
    com_socket_close(fd);
    mutex_clients.lock();
    if(clients.count(fd) != 0)
    {
        clients.erase(fd);
    }
    mutex_clients.unlock();
}

int ComUnixDomainServer::acceptClient()
{
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_un));
    socklen_t len = sizeof(struct sockaddr_un);
    int clientfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
    if(clientfd < 0)
    {
        LOG_E("bad accept client");
        return -1;
    }
    LOG_D("Accept Connection, fd=%d, file_path=%s", clientfd, client_addr.sun_path);
    SOCKET_CLIENT_DES des;
    des.clientfd = clientfd;
    des.host = client_addr.sun_path;
    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到EPOLL的监听中
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = clientfd;
    event.events =  EPOLLIN | EPOLLET | EPOLLRDHUP;//边缘触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &event);
    onConnectionChanged(des.host, des.clientfd, true);
    return 0;
}

int ComUnixDomainServer::startServer()
{
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_fd <= 0)
    {
        LOG_E("socket create failed : socketfd = %d", server_fd);
        return -1;
    }
    com_file_remove(server_file_name.c_str());
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, server_file_name.c_str());
    //int resuse_flag = 1;
    //setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &resuse_flag, sizeof(int));
    //int len = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);
    if(bind(server_fd, (struct sockaddr*)(&server_addr), sizeof(server_addr)) < 0)
    {
        LOG_E("socket bind failed : socketfd = %d", server_fd);
        com_socket_close(server_fd);
        return -2;
    }
    if(listen(server_fd, 128) < 0)
    {
        LOG_E("socket listen failed : socketfd = %d", server_fd);
        com_socket_close(server_fd);
        return -3;
    }
    // epoll 初始化
    epollfd = epoll_create(SOCKET_SERVER_MAX_CLIENTS);
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &event) < 0)
    {
        LOG_E("epoll add failed : socketfd = %d", server_fd);
        return -4;
    }
    thread_listener_running = true;
    thread_listener = std::thread(ThreadListener, this);
    thread_receiver_running = true;
    thread_receiver = std::thread(ThreadReceiver, this);
    return 0;
}

void ComUnixDomainServer::stopServer()
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
    com_socket_close(epollfd);
    epollfd = -1;
    mutex_clients.lock();
    for(size_t i = 0; i < clients.size(); i++)
    {
        com_socket_close(clients[i].clientfd);
    }
    mutex_clients.unlock();
    LOG_I("socket server stopped");
}

int ComUnixDomainServer::send(int clientfd, const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return 0;
    }
    return com_socket_tcp_send(clientfd, data, data_size);
}

int ComUnixDomainServer::send(const char* client_file_name_wildcard, const void* data, int data_size)
{
    if(client_file_name_wildcard == NULL || data == NULL || data_size <= 0)
    {
        return 0;
    }
    bool has_wildcard = false;
    for(int i = 0; i < com_string_len(client_file_name_wildcard); i++)
    {
        if(client_file_name_wildcard[i] == '?' || client_file_name_wildcard[i] == '*')
        {
            has_wildcard = true;
            break;
        }
    }
    std::vector<int> matched;
    mutex_clients.lock();
    std::map<int, SOCKET_CLIENT_DES>::iterator it;
    for(it = clients.begin(); it != clients.end(); it++)
    {
        SOCKET_CLIENT_DES des = it->second;
        if(com_string_match(des.host.c_str(), client_file_name_wildcard))
        {
            matched.push_back(des.clientfd);
            if(has_wildcard == false)
            {
                break;
            }
        }
    }
    mutex_clients.unlock();

    int ret = 0;
    for(size_t i = 0; i < matched.size(); i++)
    {
        ret = com_socket_tcp_send(matched[i], data, data_size);
    }
    return ret;
}

std::string& ComUnixDomainServer::getServerFileName()
{
    return server_file_name;
}

int ComUnixDomainServer::getSocketfd()
{
    return server_fd;
}

void ComUnixDomainServer::onConnectionChanged(std::string& client_file_name, int socketfd, bool connected)
{
}

void ComUnixDomainServer::onRecv(std::string& client_file_name, int socketfd, uint8* data, int data_size)
{
}

void ComUnixDomainServer::broadcast(const void* data, int data_size)
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

