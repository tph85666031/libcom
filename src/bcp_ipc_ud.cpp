#include "bcp_ipc_ud.h"
#include "bcp_log.h"

#if __linux__ == 1
#define UD_IPC_SOF 0xA5
#define UD_IPC_NAME_LEN  32

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint8 sof;
    char from[UD_IPC_NAME_LEN];
    char to[UD_IPC_NAME_LEN];
    uint32 data_size;
    uint8 data[0];
} UD_IPC_MSG;
#pragma pack(pop)

UnixDomainIPCServer::UnixDomainIPCServer(const char* file_name) : UnixDomainTcpServer(file_name)
{
    if (bcp_string_len(file_name) > UD_IPC_NAME_LEN)
    {
        LOG_W("domain server name exceeds length limit of %d", UD_IPC_NAME_LEN);
    }
    cleaner_running = true;
    thread_cleaner = std::thread(ThreadCleaner, this);
    startServer();
}

UnixDomainIPCServer::~UnixDomainIPCServer()
{
    cleaner_running = false;
    if (thread_cleaner.joinable())
    {
        thread_cleaner.join();
    }
}

void UnixDomainIPCServer::onMessage(std::string& from_file_name, uint8* data, int data_size)
{
    LOG_D("%s go message from %s", getServerFileName().c_str(), from_file_name.c_str());
}

void UnixDomainIPCServer::onRecv(std::string& client_file_name, int socketfd, uint8* data, int dataSize)
{
    forwardMessage(client_file_name, data, dataSize);
}

void UnixDomainIPCServer::onConnectionChanged(std::string& client_file_name, int socketfd, bool connected)
{
}

void UnixDomainIPCServer::forwardMessage(std::string& from_file_name, uint8* data, int data_size)
{
    if (from_file_name.empty() || data == NULL || data_size <= 0)
    {
        return;
    }
    UD_IPC_CLIENT des;
    mutex_caches.lock();
    if (caches.count(from_file_name) != 0)
    {
        des = caches[from_file_name];
    }
    mutex_caches.unlock();
    if (des.bytes.empty())
    {
        for (int i = 0; i < data_size; i++)
        {
            if (data[i] == UD_IPC_SOF)
            {
                des.bytes.append(data + i, data_size - i);
                break;
            }
        }
    }
    else
    {
        des.bytes.append(data, data_size);
    }

    while (des.bytes.getDataSize() >= sizeof(UD_IPC_MSG))
    {
        UD_IPC_MSG* head = (UD_IPC_MSG*)des.bytes.getData();
        if (des.bytes.getDataSize() < sizeof(UD_IPC_MSG) + head->data_size)
        {
            break;
        }
        if (bcp_string_equal(getServerFileName().c_str(), head->to))
        {
            std::string from;
            from.assign(head->from, sizeof(head->from));
            onMessage(from, (uint8*)head, sizeof(UD_IPC_MSG) + head->data_size);
        }
        else
        {
            send(head->to, (uint8*)head, sizeof(UD_IPC_MSG) + head->data_size);
        }
        des.bytes.removeHead(sizeof(UD_IPC_MSG) + head->data_size);
        for (int j = 0; j < des.bytes.getDataSize(); j++)
        {
            if (des.bytes.getAt(j) == UD_IPC_SOF)
            {
                des.bytes.removeHead(j);
                break;
            }
        }
    }

    mutex_caches.lock();
    if (des.bytes.empty())
    {
        caches.erase(from_file_name);
    }
    else
    {
        des.time = bcp_time_cpu_s();
        caches[from_file_name] = des;
    }
    mutex_caches.unlock();
}

void UnixDomainIPCServer::ThreadCleaner(UnixDomainIPCServer* server)
{
    while (server->cleaner_running)
    {
        std::map<std::string, UD_IPC_CLIENT>::iterator it;
        server->mutex_caches.lock();
        for (it = server->caches.begin(); it != server->caches.end(); ++it)
        {
            UD_IPC_CLIENT* des = &it->second;
            if (bcp_time_cpu_s() - des->time > 5)
            {
                server->caches.erase(it);
            }
        }
        server->mutex_caches.unlock();
        bcp_sleep_s(1);
    }
}

UnixDomainIPCClient::UnixDomainIPCClient(const char* server_file_name, const char* file_name) : UnixDomainTcpClient(server_file_name, file_name)
{
    if (bcp_string_len(server_file_name) > UD_IPC_NAME_LEN || bcp_string_len(file_name) > UD_IPC_NAME_LEN)
    {
        LOG_W("domain client/server name exceeds length limit of %d", UD_IPC_NAME_LEN);
    }
    startClient();
}

UnixDomainIPCClient::~UnixDomainIPCClient()
{
}

bool UnixDomainIPCClient::sendMessage(const char* to_file_name, uint8* data, int data_size)
{
    if (to_file_name == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    UD_IPC_MSG msg;
    memset(&msg, 0, sizeof(UD_IPC_MSG));
    msg.sof = UD_IPC_SOF;
    bcp_strncpy(msg.from, getFileName().c_str(), sizeof(msg.from));
    bcp_strncpy(msg.to, to_file_name, sizeof(msg.to));
    msg.data_size = data_size;
    ByteArray bytes;
    bytes.append((uint8*)&msg, sizeof(UD_IPC_MSG));
    bytes.append(data, data_size);
    int ret = send(bytes.getData(), bytes.getDataSize());
    return (ret == bytes.getDataSize());
}

void UnixDomainIPCClient::onMessage(std::string& from_file_name, uint8* data, int data_size)
{
}

void UnixDomainIPCClient::onConnectionChanged(bool connected)
{
    //LOG_D("%s %s", getFileName(), connected ? "true" : "false");
}

void UnixDomainIPCClient::onRecv(uint8* data, int data_size)
{
    if (bytes.empty())
    {
        for (int i = 0; i < data_size; i++)
        {
            if (data[i] == UD_IPC_SOF)
            {
                bytes.append(data + i, data_size - i);
                break;
            }
        }
    }
    else
    {
        bytes.append(data, data_size);
    }

    while (bytes.getDataSize() >= sizeof(UD_IPC_MSG))
    {
        UD_IPC_MSG* head = (UD_IPC_MSG*)bytes.getData();
        if (bytes.getDataSize() < sizeof(UD_IPC_MSG) + head->data_size)
        {
            break;
        }
        if (bcp_string_match(getFileName().c_str(), head->to))
        {
            std::string from;
            from.assign(head->from, sizeof(head->from));
            onMessage(from, head->data, head->data_size);
        }
        bytes.removeHead(sizeof(UD_IPC_MSG) + head->data_size);
        for (int j = 0; j < bytes.getDataSize(); j++)
        {
            if (bytes.getAt(j) == UD_IPC_SOF)
            {
                bytes.removeHead(j);
                break;
            }
        }
    }
}
#endif
