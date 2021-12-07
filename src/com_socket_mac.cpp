#ifdef __APPLE__

typedef struct kevent IOM_EVENT;
#define iom_event_fd(x)         (int)(intptr_t)((x).udata)
#define iom_event_events(x)     (x).filter
#define iom_error_events(x)     (!((x) & EVFILT_READ))

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
    EV_SET(&events[0], socketfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void*)(intptr_t)socketfd);
    if(kevent(epollfd, events, 1, NULL, 0, NULL) < 0)
    {
        LOG_E("kqueue add failed : fd = %d", getSocketfd());
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
        LOG_E("kqueue add accept fd failed : clientfd = %d", clientfd);
    }
}

void TCPServer::IOMRemoveMonitor(int fd)
{
    struct kevent ev[1];
    EV_SET(&ev[0], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    if(kevent(epollfd, ev, 1, NULL, 0, NULL) < 0)
    {
        LOG_E("kqueue close failed : fd = %d", fd);
    }
}

int TCPServer::IOMWaitFor(IOM_EVENT* event_list, int event_len)
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
    if(listen(socketfd, 5) < 0)
    {
        LOG_E("socket listen failed : socketfd = %d", socketfd.load());
        com_socket_close(socketfd);
        return -3;
    }
    if(! IOMCreate())
    {
        LOG_E("IO multiplex create failed : fd = %d", getSocketfd());
        return -4;
    }
    listener_running = true;
    thread_listener = std::thread(ThreadTCPServerListener, this);
    receiver_running = true;
    thread_receiver = std::thread(ThreadTCPServerReceiver, this);
    return 0;
}

void TCPServer::closeClient(int fd)
{
    this->IOMRemoveMonitor(fd);
    CLIENT_DES des;
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

void TCPServer::stopServer()
{
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
    com_socket_close(socketfd);
    socketfd = -1;
    com_socket_close(epollfd);
    epollfd = -1;
    mutex_clients.lock();
    for(int i = 0; i < clients.size(); i++)
    {
        com_socket_close(clients[i].clientfd);
    }
    mutex_clients.unlock();
    LOG_I("socket server stopped");
}

int TCPServer::send(int clientfd, uint8* data, int dataSize)
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

int TCPServer::recvData(int socketfd)
{
    mutex_clients.lock();
    if(clients.count(socketfd) == 0)
    {
        mutex_clients.unlock();
        return -1;
    }
    CLIENT_DES des = clients[socketfd];
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
        //epoll_wait
        IOM_EVENT eventList[SOCKET_SERVER_MAX_CLIENTS];
        int count = socket_server->IOMWaitFor(eventList, SOCKET_SERVER_MAX_CLIENTS);
        if(count < 0)
        {
            LOG_E("iom_event error,errno=%d:%s", errno, strerror(errno));
            break;
        }
        else if(count == 0)
        {
            LOG_D("iom_event timeout");
            continue;
        }
        if(socket_server->listener_running == false)
        {
            break;
        }
        //直接获取了事件数量,给出了活动的流,这里是和poll区别的关键
        for(int i = 0; i < count; i++)
        {
            int fd = iom_event_fd(eventList[i]);
            int events = iom_event_events(eventList[i]);
            if(iom_error_events(events))
            {
                LOG_D("iom_event client quit, fd=%d, event=0x%X", fd, events);
                socket_server->closeClient(fd);
                break;
            }
            if(fd == socket_server->socketfd)
            {
                socket_server->acceptClient();
            }
            else
            {
                socket_server->recvData(fd);
            }
        }
    }
    LOG_I("socket server quit, port=%d", socket_server->getPort());
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
        CLIENT_DES des = socket_server->ready_fds.front();
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


// SocketTcpServer
SocketTcpServer::SocketTcpServer(uint16 port)
{
    setHost("127.0.0.1");
    setPort(port);
}

SocketTcpServer::~SocketTcpServer()
{

}

bool SocketTcpServer::initListen()
{
    if(port == 0)
    {
        LOG_E("port not set");
        return false;
    }
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd <= 0)
    {
        LOG_E("socket create failed : fd = %d", getSocketfd());
        return false;
    }
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family  =  AF_INET;
    server_addr.sin_port = htons(getPort());
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);
    int resuse_flag = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &resuse_flag, sizeof(int));
    if(bind(getSocketfd(), (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        LOG_E("socket bind failed : fd = %d", getSocketfd());
        com_socket_close(socketfd);
        return false;
    }
    return true;
}

int SocketTcpServer::acceptClient()
{
    struct sockaddr_in sin;
    socklen_t len = sizeof(struct sockaddr_in);
    memset(&sin, 0, len);
    int clientfd = accept(socketfd, (struct sockaddr*)&sin, &len);
    if(clientfd < 0)
    {
        LOG_E("bad accept client, errno=%d:%s", errno, strerror(errno));
        return -1;
    }
    LOG_D("Accept Connection, fd=%d, addr=%s,port=%u",
          clientfd, com_ip_to_string(sin.sin_addr.s_addr).c_str(), sin.sin_port);
    CLIENT_DES des;
    des.clientfd = clientfd;
    des.host = com_ip_to_string(sin.sin_addr.s_addr);
    des.port = sin.sin_port;

    mutex_clients.lock();
    clients[des.clientfd] = des;
    mutex_clients.unlock();
    //将新建立的连接添加到监听队列中
    IOMAddMonitor(clientfd);
    onConnectionChanged(des.host, des.port, des.clientfd, true);
    return 0;
}

int SocketTcpServer::send(const char* host, uint16 port, uint8* data, int dataSize)
{
    if(host == NULL || data == NULL || dataSize <= 0)
    {
        return 0;
    }
    mutex_clients.lock();
    int clientfd = -1;
    std::map<int, CLIENT_DES>::iterator it;
    for(it = clients.begin(); it != clients.end(); it++)
    {
        CLIENT_DES des = it->second;
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


// UnixDomainTcpServer
UnixDomainTcpServer::UnixDomainTcpServer(const char* server_file_name)
{
    setHost(server_file_name);
    setPort(0);
}

UnixDomainTcpServer::~UnixDomainTcpServer()
{
    if(getHost().size() > 0)
    {
        com_file_remove(getHost().c_str());
    }
}

bool UnixDomainTcpServer::initListen()
{
    socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(socketfd <= 0)
    {
        LOG_E("socket create failed : socketfd = %d", socketfd.load());
        return false;
    }
    com_file_remove(getHost().c_str());
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, getHost().c_str());
    long len = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);
    if(bind(socketfd.load(), (struct sockaddr*)(&server_addr), (int)len) < 0)
    {
        LOG_E("socket bind failed : socketfd = %d", socketfd.load());
        com_socket_close(socketfd);
        return false;
    }
    return true;
}

int UnixDomainTcpServer::acceptClient()
{
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_un));
    socklen_t len = sizeof(struct sockaddr_un);
    int clientfd = accept(socketfd, (struct sockaddr*)&client_addr, &len);
    if(clientfd < 0)
    {
        LOG_E("bad accept client");
        return -1;
    }
    LOG_D("Accept Connection, fd=%d, file_path=%s", clientfd, client_addr.sun_path);
    CLIENT_DES des;
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

int UnixDomainTcpServer::send(const char* client_file_name_wildcard, uint8* data, int data_size)
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
    std::map<int, CLIENT_DES>::iterator it;
    for(it = clients.begin(); it != clients.end(); it++)
    {
        CLIENT_DES des = it->second;
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

#endif

