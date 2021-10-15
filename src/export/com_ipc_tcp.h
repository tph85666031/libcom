#ifndef __COM_IPC_TCP_H__
#define __COM_IPC_TCP_H__

#include "com_base.h"
#include "com_log.h"
#include "com_socket.h"

#if __linux__ == 1
class TcpIpcMessage final
{
public:
    CPPBytes toBytes();
    static bool FromBytes(TcpIpcMessage& msg, uint8_t* data, int dataSize);
public:
    std::string from;//char[32]
    std::string to;//char[32]
    CPPBytes bytes;
private:
    uint8_t sof;
};

typedef struct
{
    std::string name;
    int clinetfd;
    CPPBytes bytes;
} TCP_IPC_CLIENT;

class TcpIpcServer : public SocketTcpServer
{
public:
    TcpIpcServer(const char* name, uint16_t port);
    virtual ~TcpIpcServer();
    std::string& getName();
private:
    virtual void onMessage(std::string& from_name, uint8_t* data, int data_size);
    void onConnectionChanged(std::string& host, uint16_t port, int socketfd, bool connected);
    void onRecv(std::string& host, uint16_t port, int socketfd, uint8_t* data, int data_size);
    void buildMessage(int socketfd, uint8_t* data, int data_size);
    static void ThreadForward(TcpIpcServer* server);
private:
    std::atomic<bool> forward_running;
    std::thread thread_forward;
    CPPMutex mutex_fds;
    std::map<int, TCP_IPC_CLIENT> fdcaches;
    CPPMutex mutex_msgs;
    CPPSem sem_msgs;
    std::vector<TcpIpcMessage> msgs;
    std::string name;
};

class TcpIpcClient: public SocketTcpClient
{
public:
    TcpIpcClient(const char* name, const char* serverName, const char* host, uint16_t port);
    virtual ~TcpIpcClient();
    bool sendMessage(const char* name, uint8_t* data, int dataSize);
    std::string getName();
    std::string getServerName();
private:
    virtual void onMessage(std::string& from_name, uint8_t* data, int data_size);
    void buildMessage(uint8_t* data, int data_size);
    void onConnectionChanged(bool connected);
    void onRecv(uint8_t* data, int data_size);
private:
    CPPBytes bytes;
    std::string name;
    std::string server_name;
};
#endif

#endif /* __COM_IPC_TCP_H__ */