#ifdef __APPLE__
#include <sys/event.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <com_socket_mac.h>

#include "com_socket.h"

TCPServer::TCPServer()
{
    epollfd = -1;
    receiver_running = false;
    listener_running = false;
    epoll_timeout_ms = 3000;
}

TCPServer::~TCPServer()
{
    stopServer();
}

bool TCPServer::IOMCreate()
{
    epollfd = kqueue();
    if(epollfd < 0)
    {
        LOG_E("Failed to open kqueue");
        return false;
    }
    struct kevent events[1];
    EV_SET(&events[0], server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void*)(intptr_t)server_fd);
    if(kevent(epollfd, events, 1, NULL, 0, NULL) < 0)
    {
        LOG_E("kqueue add failed : fd = %d", server_fd);
        return false;
    }
    return true;
}

void TCPServer::IOMAddMonitor(int clientfd)
{
    struct kevent ev[1];
    EV_SET(&ev[0], clientfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void*)(intptr_t)clientfd);
    if(kevent(epollfd, ev, 1, NULL, 0, NULL) < 0)
    {
        LOG_E("kqueue add io monitor failed! clientfd=%d", clientfd);
    }
    else
    {
        LOG_I("kqueue add io monitor success! clientfd=%d", clientfd);
    }
}

void TCPServer::IOMRemoveMonitor(int fd)
{
    struct kevent ev[1];
    EV_SET(&ev[0], fd, EVFILT_READ, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    if(kevent(epollfd, ev, 1, NULL, 0, NULL) < 0)
    {
        LOG_E("kqueue close failed : fd = %d", fd);
    }
    else
    {
        LOG_I("kqueue close success : fd = %d", fd);
    }
}

int TCPServer::IOMWaitFor(struct kevent* event_list, int event_len)
{
    int count = 0;
    struct timespec timeout;
    timeout.tv_sec = epoll_timeout_ms / 1000;
    timeout.tv_nsec = (epoll_timeout_ms % 1000) * 1000 * 1000;
    count = kevent(epollfd, NULL, 0, event_list, event_len, &timeout);
    return count;
}

int TCPServer::startServer()
{
    if(! initListen())
    {
        LOG_E("start server failed");
        return -1;
    }
    if(listen(server_fd, 5) < 0)
    {
        LOG_E("socket listen failed! socketfd: %d, error: %s", server_fd, strerror(errno));
        com_socket_close(server_fd);
        return -3;
    }
    if(! IOMCreate())
    {
        LOG_E("IO multiplex create failed! socketfd: %d", server_fd);
        return -4;
    }
    listener_running = true;
    thread_listener = std::thread(ThreadTCPServerListener, this);
    receiver_running = true;
    thread_receiver = std::thread(ThreadTCPServerReceiver, this);
    LOG_I("TCPServer started");
    return 0;
}

void TCPServer::closeClient(int fd)
{
    LOG_I("start close client fd: %d", fd);
    this->IOMRemoveMonitor(fd);
    mutex_clients.lock();
    if(clients.count(fd) == 0)
    {
        LOG_E("cannot find SOCKET_CLIENT_DES for fd: %d", fd);
        mutex_clients.unlock();
        return;
    }
    SOCKET_CLIENT_DES des = clients[fd];
    mutex_clients.unlock();
    onConnectionChanged(des.host, des.port, des.clientfd, false);
    com_socket_close(fd);
    mutex_clients.lock();
    if(clients.count(fd) != 0)
    {
        clients.erase(fd);
    }
    LOG_D("remain clients count: %ld", clients.size());
    mutex_clients.unlock();
}

void TCPServer::stopServer()
{
    // 1. stop receiving connection
    com_socket_close(server_fd);
    server_fd = -1;
    com_socket_close(epollfd);
    epollfd = -1;

    // 2. stop handler threads
    listener_running = false;
    if(thread_listener.joinable())
    {
        thread_listener.join();
    }
    receiver_running = false;
    if(thread_receiver.joinable())
    {
        thread_receiver.join();
    }
    
    // 3. close clients' sockets
    mutex_clients.lock();
    for(int i = 0; i < clients.size(); i++)
    {
        com_socket_close(clients[i].clientfd);
    }
    mutex_clients.unlock();
    LOG_I("socket server stopped");
}

int TCPServer::send(int clientfd, const void* data, int dataSize)
{
    if(data == NULL || dataSize <= 0)
    {
        return 0;
    }
    int ret = com_socket_tcp_send(clientfd, data, dataSize);
    if(ret == -1)
    {
        closeClient(clientfd);
    }
    return ret;
}

void TCPServer::broadcast(const void* data, int data_size)
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

int TCPServer::recvData(int socketfd)
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
    ready_fds.push(des);
    semfds.post();
    mutexfds.unlock();
    return 0;
}

void TCPServer::ThreadTCPServerListener(TCPServer* socket_server)
{
    if(socket_server == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }
    while(socket_server->listener_running)
    {
        socket_server->epoll_timeout_ms = 1000;
        struct kevent events[SOCKET_SERVER_MAX_CLIENTS];
        int count = socket_server->IOMWaitFor(events, SOCKET_SERVER_MAX_CLIENTS);
        if(count <= 0)
        {
            if(count < 0)
            {
                LOG_E("iom_event error, errno=%d:%s", errno, strerror(errno));
                sleep(3);
            }
            continue;
        }
        if(socket_server->listener_running == false)
        {
            break;
        }
        for(int i = 0; i < count; i++)
        {
            int fd = (int)events[i].ident;
            int16_t filter = events[i].filter;
            uint16_t flags = events[i].flags;
            // When the client disconnects an EOF is sent. By closing the file
            // descriptor the event is automatically removed from the kqueue.
            if(flags & EV_EOF)
            {
                LOG_D("Client has disconnected, fd: %d", fd);
                socket_server->closeClient(fd);
            }
            else if(fd == socket_server->server_fd)
            {
                socket_server->acceptClient();
            }
            else if(filter & EVFILT_READ)
            {
                socket_server->recvData(fd);
            }
        }
    }
    LOG_I("socket server quit");
    return;
}

void TCPServer::ThreadTCPServerReceiver(TCPServer* socket_server)
{
    uint8 buf[4096];
    while(socket_server->receiver_running)
    {
        socket_server->semfds.wait(1000);
        socket_server->mutexfds.lock();
        if(socket_server->ready_fds.size() <= 0)
        {
            socket_server->mutexfds.unlock();
            continue;
        }
        SOCKET_CLIENT_DES des = socket_server->ready_fds.front();
        socket_server->ready_fds.pop();
        socket_server->mutexfds.unlock();
        while(true)
        {
            memset(buf, 0, sizeof(buf));
            int ret = (int)recv(des.clientfd, buf, sizeof(buf), MSG_DONTWAIT);
            if(ret <= 0)
            {
                break;
            }
            socket_server->onRecv(des.host, des.port, des.clientfd, buf, ret);
        }
    }
    return;
}


// ComTcpServer
ComTcpServer::ComTcpServer()
{
}

ComTcpServer::ComTcpServer(uint16 port)
{
    server_port = port;
}

ComTcpServer::~ComTcpServer()
{
}

ComTcpServer& ComTcpServer::setPort(uint16 port)
{
    this->server_port = port;
    return *this;
}

bool ComTcpServer::initListen()
{
    if(server_port == 0)
    {
        LOG_E("server_port not set");
        return false;
    }
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd <= 0)
    {
        LOG_E("socket create failed : fd = %d", server_fd);
        return false;
    }
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family  =  AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);
    int resuse_flag = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &resuse_flag, sizeof(int));
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        LOG_E("socket bind failed! fd: %d, error: %s", server_fd, strerror(errno));
        com_socket_close(server_fd);
        return false;
    }
    LOG_I("ComTcpServer init listen, server_fd: %d, server port: %d", server_fd, server_port);
    return true;
}

int ComTcpServer::acceptClient()
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
    LOG_D("accept new connection, fd=%d, client=%s:%u",
          clientfd, com_ipv4_to_string(sin.sin_addr.s_addr).c_str(), sin.sin_port);
    SOCKET_CLIENT_DES des;
    des.clientfd = clientfd;
    des.host = com_ipv4_to_string(sin.sin_addr.s_addr);
    des.port = sin.sin_port;

    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到监听队列中
    IOMAddMonitor(clientfd);
    onConnectionChanged(des.host, des.port, des.clientfd, true);
    return 0;
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

int ComTcpServer::send(const char* host, uint16 port, const void* data, int dataSize)
{
    if(host == NULL || data == NULL || dataSize <= 0)
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
    int ret = com_socket_tcp_send(clientfd, data, dataSize);
    if(ret == -1)
    {
        closeClient(clientfd);
    }
    return ret;
}

void ComTcpServer::broadcast(const void *data, int data_size) {
    TCPServer::broadcast(data, data_size);
}


// UnixDomainTcpServer
ComUnixDomainServer::ComUnixDomainServer(const char* server_file_name)
{
    if(server_file_name != NULL)
    {
        this->server_file_name = server_file_name;
    }
}

ComUnixDomainServer::~ComUnixDomainServer()
{
    com_file_remove(server_file_name.c_str());
}

bool ComUnixDomainServer::initListen()
{
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_fd <= 0)
    {
        LOG_E("socket create failed : socketfd = %d", server_fd);
        return false;
    }
    com_file_remove(this->server_file_name.c_str());
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, this->server_file_name.c_str());
    if(bind(server_fd, (struct sockaddr*)(&server_addr), sizeof(server_addr)) < 0)
    {
        LOG_E("socket bind failed! fd: %d, error: %s", server_fd, strerror(errno));
        com_socket_close(server_fd);
        return false;
    }
    LOG_I("UnixDomainTcpServer init listen, server_fd: %d, unix domain file: %s", server_fd, this->server_file_name.c_str());
    return true;
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
    des.port = 0; // no use
    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到监听队列中
    IOMAddMonitor(clientfd);
    onConnectionChanged(des.host, des.port, des.clientfd, true);
    return 0;
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

void ComUnixDomainServer::broadcast(const void *data, int data_size) {
    TCPServer::broadcast(data, data_size);
}

#endif

