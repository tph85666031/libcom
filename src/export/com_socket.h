#ifndef __COM_SOCKET_H__
#define __COM_SOCKET_H__

#include <atomic>
#include "com_base.h"
#include "com_thread.h"
#include "com_err.h"
#include "com_file.h"
#include "com_log.h"
#include "com_serializer.h"

#define LENGTH_MAC    6

#define SOCKET_SERVER_MAX_CLIENTS 500

class COM_EXPORT NicInfo
{
public:
    std::string name;
    std::string descritpion;
    uint32 flags;
    int addr_family;
    uint8 mac[LENGTH_MAC];
    std::string ip;
    std::string ip_mask;
    std::string ip_broadcast;
public:
    NicInfo();
    virtual ~NicInfo();
};

COM_EXPORT void com_socket_global_init();
COM_EXPORT void com_socket_global_uninit();

COM_EXPORT void com_socket_set_recv_timeout(int sock, int timeout_ms);
COM_EXPORT void com_socket_set_send_timeout(int sock, int timeout_ms);

COM_EXPORT int com_socket_get_tcp_connection_status(int sock);//-1获取失败,0连接断开,1=连接成功
COM_EXPORT int com_socket_unix_domain_open(const char* my_name, const char* server_name);
COM_EXPORT int com_socket_udp_open(const char* interface_name, uint16 recv_port, bool broadcast = false);
COM_EXPORT int com_socket_tcp_open(const char* remote_host, uint16 remote_port,
                                   uint32 timeout_ms = 10000, const char* interface_name = NULL);
COM_EXPORT int com_socket_udp_send(int socketfd, const char* dest_host, int dest_port,
                                   const void* data, int data_size);
COM_EXPORT int com_socket_tcp_send(int socketfd, const void* data, int data_size);
COM_EXPORT int com_socket_udp_read(int socketfd, uint8* data, int data_size,
                                   uint32 timeout_ms = 10000, char* sender_ip = NULL, int sender_ip_size = 0);
COM_EXPORT int com_socket_tcp_read(int socketfd, uint8* data, int data_size,
                                   uint32 timeout_ms = 10000, bool block_mode = true);
COM_EXPORT void com_socket_close(int socketfd);

COM_EXPORT bool com_net_get_mac(const char* interface_name, uint8* mac);
COM_EXPORT bool com_net_get_nic(const char* interface_name, NicInfo& nic);
//反向原地址可达性校验，0=不校验，1=严格校验，2=宽松校验
COM_EXPORT void com_net_set_rpfilter(const char* interface_name, uint8 flag);
COM_EXPORT uint8 com_net_get_rpfilter(const char* interface_name);

COM_EXPORT bool com_net_is_interface_exist(const char* interface_name);
COM_EXPORT std::vector<std::string> com_net_get_interface_all();

class COM_EXPORT Socket
{
public:
    Socket();
    virtual ~Socket();
    void setHost(const char* host);
    void setPort(uint16 port);
    void setSocketfd(int fd);
    void setInterface(const char* interface_name);
    std::string getHost();
    uint16 getPort();
    int getSocketfd();
    std::string getInterface();
protected:
    std::atomic<uint16> port;
    std::atomic<int> socketfd;
    std::string interface_name;
private:
    std::mutex mutxe;
    std::string host;
};

class COM_EXPORT SocketTcpClient : public Socket
{
public:
    SocketTcpClient();
    SocketTcpClient(const char* host, uint16 port);
    virtual ~SocketTcpClient();
    virtual bool startClient();
    virtual void stopClient();
    int send(const void* data, int data_size);
    bool isConnected();
    void reconnect();
    void setReconnectInterval(int reconnect_interval_ms);
protected:
    virtual void onConnectionChanged(bool connected);
    virtual void onRecv(uint8* data, int data_size);
private:
    static void ThreadSocketClientRunner(SocketTcpClient* socket_client);
private:
    std::atomic<bool> running;
    std::thread thread_runner;
    std::atomic<bool> reconnect_at_once;
    std::atomic<int> reconnect_interval_ms = {3000};
    std::atomic<bool> connected;
};

class UnixDomainTcpClient
{
public:
    UnixDomainTcpClient(const char* server_file_name, const char* file_name);
    UnixDomainTcpClient(const char* server_file_name) : UnixDomainTcpClient(server_file_name, NULL) {}
    virtual ~UnixDomainTcpClient();
    void setServerFileName(const char* server_file_name);
    void setFileName(const char* file_name);
    int startClient();
    void stopClient();
    int send(const void* data, int data_size);
    void reconnect();
    virtual void onConnectionChanged(bool connected);
    virtual void onRecv(uint8* data, int data_size);
    std::string& getFileName();
    std::string& getServerFileName();
    int getSocketfd();
private:
    bool connect();
    static void ThreadUnixDomainClientReceiver(UnixDomainTcpClient* client);
private:
    std::atomic<int> socketfd;
    std::string file_name;
    std::string server_file_name;
    std::atomic<bool> receiver_running;
    std::thread thread_receiver;
    std::atomic<bool> need_reconnect;
};

class COM_EXPORT MulticastNode : public Socket
{
public:
    MulticastNode();
    virtual ~MulticastNode();

    bool startNode();
    void stopNode();
    int send(const void* data, int data_size);

private:
    virtual void onRecv(uint8* data, int data_size);
    static void ThreadRX(MulticastNode* client);

private:
    std::atomic<bool> thread_rx_running = {false};
    std::thread thread_rx;
    uint8 buf[4096];
};

class COM_EXPORT MulticastNodeString : public MulticastNode
{
public:
    MulticastNodeString();
    virtual ~MulticastNodeString();

    using MulticastNode::send;
    int send(const char* content);
    int send(const std::string& content);

    int getCount();
    bool fetchString(std::string& item);
    bool waitString(int timeout_ms);

private:
    void onRecv(uint8* data, int data_size);

private:
    std::string item;
    ComCondition condition_queue;
    std::mutex mutex_queue;
    std::queue<std::string> queue;

};

class COM_EXPORT StringIPCClient : protected SocketTcpClient
{
public:
    StringIPCClient();
    virtual ~StringIPCClient();

    bool startIPC(const char* name, const char* host, uint16 port);
    void stopIPC();
    bool sendString(const char* name_to, const std::string& message);
    bool sendString(const char* name_to, const char* message);
    std::string getName();

protected:
    virtual void onMessage(const std::string& name, const std::string& message);
    virtual void onConnectionChanged(bool connected);
private:
    static void ThreadReceiver(StringIPCClient* ctx);
    void onRecv(uint8* data, int data_size);
private:
    ComCondition condition_queue;
    std::mutex mutex_queue;
    std::queue<std::string> queue;

    std::atomic<bool> thread_receiver_running = {false};
    std::thread thread_receiver;

    std::string val;
    std::string name;
};

/* Server相关代码存放开始 */
#if __linux__ == 1
#include "com_socket_linux.h"
#elif defined(_WIN32) || defined(_WIN64)
#include "com_socket_win.h"
#elif defined(__APPLE__)
#include "com_socket_mac.h"
#endif

class COM_EXPORT StringIPCServer : protected SocketTcpServer
{
public:
    StringIPCServer();
    virtual ~StringIPCServer();

    bool startIPC(const char* name, uint16 port);
    void stopIPC();
    bool sendString(const char* name_to, const std::string& message);
    bool sendString(const char* name_to, const char* message);
    std::string getName();

protected:
    virtual void onMessage(const std::string& name, const std::string& message);
private:
    static void ThreadReceiver(StringIPCServer* ctx);
    void onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected);
    void onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size);
    void setClientFD(const char* name, int fd);
    int getClientFD(const char* name);
    void removeClientFD(int fd);
    void removeClientFD(const char* name);;
private:
    std::map<int, std::string> vals;

    ComCondition condition_queue;
    std::mutex mutex_queue;
    std::queue<std::string> queue;

    std::mutex mutex_clients;
    std::map<std::string, int> clients;

    std::atomic<bool> thread_receiver_running = {false};
    std::thread thread_receiver;

    std::string name;
};

#endif /* __COM_SOCKET_H__ */

