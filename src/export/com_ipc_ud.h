#ifndef __COM_IPC_UD_H__
#define __COM_IPC_UD_H__

#include "com_base.h"
#include "com_socket.h"

#if __linux__ == 1
typedef struct
{
    uint32 time;
    CPPBytes bytes;
} UD_IPC_CLIENT;

class UnixDomainIPCServer : public UnixDomainTcpServer
{
public:
    UnixDomainIPCServer(const char* file_name);
    virtual ~UnixDomainIPCServer();
private:
    virtual void onMessage(std::string& from_file_name, uint8* data, int data_size);
    void onConnectionChanged(std::string& client_file_name, int socketfd, bool connected);
    void onRecv(std::string& client_file_name, int socketfd, uint8* data, int dataSize);
    void forwardMessage(std::string& from_file_name, uint8* data, int dataSize);
    static void ThreadCleaner(UnixDomainIPCServer* server);
private:
    std::atomic<bool> cleaner_running;
    CPPMutex mutex_caches;
    std::thread  thread_cleaner;
    std::map<std::string, UD_IPC_CLIENT> caches;
};

class UnixDomainIPCClient: public UnixDomainTcpClient
{
public:
    UnixDomainIPCClient(const char* server_file_name, const char* file_name);
    virtual ~UnixDomainIPCClient();
    bool sendMessage(const char* to_file_name, uint8* data, int data_size);
private:
    virtual void onMessage(std::string& from_file_name, uint8* data, int data_size);
    void onConnectionChanged(bool connected);
    void onRecv(uint8* data, int data_size);
private:
    CPPBytes bytes;
};
#endif

#endif /* __COM_IPC_UD_H__ */