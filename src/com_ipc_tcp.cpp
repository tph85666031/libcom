#include "com_ipc_tcp.h"
#include "com_serializer.h"
#include "com_log.h"

#if __linux__ == 1
#define TCP_IPC_SOF            (0xA5)
#define TCP_IPC_NAME_LENGTH    (32)
#define TCP_IPC_MSG_HEAD_SIZE  (sizeof(uint8) + TCP_IPC_NAME_LENGTH * 2 + sizeof(uint32))

CPPBytes TcpIpcMessage::toBytes()
{
    Serializer s;
    sof = TCP_IPC_SOF;

    from.resize(TCP_IPC_NAME_LENGTH, 0);
    to.resize(TCP_IPC_NAME_LENGTH, 0);

    s.append(sof);

    s.append(from);
    s.append(to);

    s.append((uint32)bytes.getDataSize());
    s.append(bytes);
    return s.toBytes();
}

bool TcpIpcMessage::FromBytes(TcpIpcMessage& msg, uint8* data, int data_size)
{
    if (data == NULL || data_size < (int)TCP_IPC_MSG_HEAD_SIZE)
    {
        return false;
    }
    if (data[0] != TCP_IPC_SOF)
    {
        LOG_F("sof incorrect");
        return false;
    }
    Serializer s(data, data_size);
    s.detach(msg.sof);
    s.detach(msg.from, TCP_IPC_NAME_LENGTH);
    s.detach(msg.to, TCP_IPC_NAME_LENGTH);

    int remain_size = 0;
    s.detach(remain_size);
    if (remain_size > s.getDetachRemainSize())
    {
        //数据未接收完整
        return false;
    }
    s.detach(msg.bytes, remain_size);

    return true;
}

TcpIpcServer::TcpIpcServer(const char* name, uint16 port) : SocketTcpServer(port)
{
    if (name != NULL)
    {
        this->name = name;
    }
    forward_running = true;
    thread_forward = std::thread(ThreadForward, this);
    startServer();
}

TcpIpcServer::~TcpIpcServer()
{
    stopServer();
    forward_running = false;
    if (thread_forward.joinable())
    {
        thread_forward.join();
    }
}

void TcpIpcServer::onMessage(std::string& from_name, uint8* data, int data_size)
{
}

void TcpIpcServer::onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected)
{
    mutex_fds.lock();
    if (connected)
    {
        TCP_IPC_CLIENT des;
        des.clinetfd = socketfd;
        fdcaches[socketfd] = des;
    }
    else
    {
        if (fdcaches.count(socketfd) != 0)
        {
            fdcaches.erase(socketfd);
        }
    }
    mutex_fds.unlock();
}

void TcpIpcServer::onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size)
{
    buildMessage(socketfd, data, data_size);
}

std::string& TcpIpcServer::getName()
{
    return name;
}

void TcpIpcServer::buildMessage(int socketfd, uint8* data, int dataSize)
{
    if (socketfd <= 0 || data == NULL || dataSize <= 0)
    {
        return;
    }

    TCP_IPC_CLIENT des;
    des.clinetfd = socketfd;
    mutex_fds.lock();
    int count = fdcaches.count(socketfd);
    if (count > 0)
    {
        des = fdcaches[socketfd];
    }
    mutex_fds.unlock();
    if (count == 0)
    {
        LOG_W("fd not exist");
        return;
    }

    des.bytes.append(data, dataSize);
    for (int i = 0; i < des.bytes.getDataSize(); i++)
    {
        if (des.bytes.getData()[i] == TCP_IPC_SOF)
        {
            des.bytes.removeHead(i);
            break;
        }
    }

    if (des.bytes.empty())
    {
        return;
    }

    //LOG_W("raw=%s:%d", des.bytes.toHexString(true).c_str(), des.bytes.getDataSize());
    while (true)
    {
        TcpIpcMessage msg;
        if (TcpIpcMessage::FromBytes(msg, des.bytes.getData(), des.bytes.getDataSize()) == false)
        {
            break;
        }
        des.name = msg.from;
        mutex_msgs.lock();
        msgs.push_back(msg);
        sem_msgs.post();
        mutex_msgs.unlock();
        //printf("size=%d,sss=%s\n", msg.bytes.getDataSize(), des.bytes.toHexString(true).c_str());
        des.bytes.removeHead(TCP_IPC_MSG_HEAD_SIZE + msg.bytes.getDataSize());
        //printf("last=%d,%s\n", des.bytes.getDataSize(), des.bytes.toHexString(true).c_str());
    }

    mutex_fds.lock();
    fdcaches[socketfd] = des;
    mutex_fds.unlock();
}

void TcpIpcServer::ThreadForward(TcpIpcServer* server)
{
    while (server->forward_running)
    {
        server->sem_msgs.wait(1000);
        server->mutex_msgs.lock();
        if (server->msgs.empty())
        {
            server->mutex_msgs.unlock();
            continue;
        }
        TcpIpcMessage msg = server->msgs.front();
        server->msgs.erase(server->msgs.begin());
        server->mutex_msgs.unlock();

        if (msg.to == server->name)
        {
            server->onMessage(msg.from, msg.bytes.getData(), msg.bytes.getDataSize());
        }
        else
        {
            std::map<int, TCP_IPC_CLIENT>::iterator it;
            server->mutex_fds.lock();
            for (it = server->fdcaches.begin(); it != server->fdcaches.end(); ++it)
            {
                TCP_IPC_CLIENT* des = &it->second;
                if (com_string_match(des->name.c_str(), msg.to.c_str()))
                {
                    CPPBytes bytes = msg.toBytes();
                    server->send(des->clinetfd, bytes.getData(), bytes.getDataSize());
                }
            }
            server->mutex_fds.unlock();
        }
    }
}

TcpIpcClient::TcpIpcClient(const char* name, const char* server_name, const char* host, uint16 port) : SocketTcpClient(host, port)
{
    if (name != NULL)
    {
        this->name = name;
    }
    if (server_name != NULL)
    {
        this->server_name = server_name;
    }
    startClient();
}

TcpIpcClient::~TcpIpcClient()
{
    stopClient();
}

bool TcpIpcClient::sendMessage(const char* target_name, uint8* data, int data_size)
{
    if (target_name == NULL)
    {
        return false;
    }
    TcpIpcMessage msg;
    msg.from = name;
    msg.to = target_name;
    msg.bytes.append(data, data_size);
    CPPBytes bytes = msg.toBytes();
    int ret = send(bytes.getData(), bytes.getDataSize());
    //LOG_D("send [%d],%s", msg.bytes.getDataSize(), msg.bytes.toHexString(true).c_str());
    return (ret == bytes.getDataSize());
}

void TcpIpcClient::onMessage(std::string& from_name, uint8* data, int data_size)
{
}

void TcpIpcClient::onConnectionChanged(bool connected)
{
    if (connected)
    {
        sendMessage(server_name.c_str(), NULL, 0);
    }
}

void TcpIpcClient::onRecv(uint8* data, int data_size)
{
    buildMessage(data, data_size);
}

std::string TcpIpcClient::getName()
{
    return name;
}

std::string TcpIpcClient::getServerName()
{
    return server_name;
}

void TcpIpcClient::buildMessage(uint8* data, int data_size)
{
    if (data == NULL || data_size <= 0)
    {
        return;
    }
    bytes.append(data, data_size);
    for (int i = 0; i < bytes.getDataSize(); i++)
    {
        if (bytes.getData()[i] == TCP_IPC_SOF)
        {
            bytes.removeHead(i);
            break;
        }
    }

    if (bytes.empty())
    {
        return;
    }

    while (true)
    {
        TcpIpcMessage msg;
        if (TcpIpcMessage::FromBytes(msg, bytes.getData(), bytes.getDataSize()) == false)
        {
            break;
        }
        onMessage(msg.from, msg.bytes.getData(), msg.bytes.getDataSize());
        bytes.removeHead(TCP_IPC_MSG_HEAD_SIZE + msg.bytes.getDataSize());
    }
}
#endif

