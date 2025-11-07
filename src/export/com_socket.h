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

class COM_EXPORT ComSocketAddr
{
public:
    void* toSockaddrStorage();
    bool valid();
public:
    bool is_ipv6 = false;
    uint32_be ipv4 = 0;
    uint8 ipv6[16] = {0};
    uint16 port = 0;
private:
    uint8 buf[128];
};

class COM_EXPORT ComNicInfo
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
    ComNicInfo();
    virtual ~ComNicInfo();
};

COM_EXPORT void com_socket_global_init();
COM_EXPORT void com_socket_global_uninit();

COM_EXPORT void com_socket_set_recv_timeout(int sock, int timeout_ms);
COM_EXPORT void com_socket_set_send_timeout(int sock, int timeout_ms);

COM_EXPORT int com_socket_get_tcp_connection_status(int sock);//-1获取失败,0连接断开,1=连接成功
COM_EXPORT int com_socket_unix_domain_open(const char* my_name, const char* server_name);
COM_EXPORT int com_socket_udp_open(const char* interface_name, uint32_be bind_ipv4 = 0, uint8* bind_ipv6 = NULL,
                                   uint16 bind_port = 0, bool broadcast = false);
COM_EXPORT int com_socket_tcp_open(const char* remote_host, uint16 remote_port, uint32 timeout_ms = 10000,
                                   uint32_be bind_ipv4 = 0, uint8* bind_ipv6 = NULL, 
                                   uint16 bind_port = 0, const char* bind_interface = NULL);
COM_EXPORT int com_socket_udp_send(int socketfd, const char* dest_host, int dest_port,
                                   const void* data, int data_size);
COM_EXPORT int com_socket_tcp_send(int socketfd, const void* data, int data_size);
COM_EXPORT int com_socket_udp_read(int socketfd, uint8* data, int data_size,
                                   uint32 timeout_ms = 10000, char* sender_ip = NULL, int sender_ip_size = 0);
COM_EXPORT int com_socket_tcp_read(int socketfd, uint8* data, int data_size,
                                   uint32 timeout_ms = 10000, bool block_mode = true);
COM_EXPORT void com_socket_close(int socketfd);

COM_EXPORT bool com_net_get_mac(const char* interface_name, uint8* mac);
COM_EXPORT bool com_net_get_nic(const char* interface_name, ComNicInfo& nic);
//反向原地址可达性校验，0=不校验，1=严格校验，2=宽松校验
COM_EXPORT void com_net_set_rpfilter(const char* interface_name, uint8 flag);
COM_EXPORT uint8 com_net_get_rpfilter(const char* interface_name);

COM_EXPORT bool com_net_is_interface_exist(const char* interface_name);
COM_EXPORT std::vector<std::string> com_net_get_interface_all();

class COM_EXPORT ComSocket
{
public:
    ComSocket();
    virtual ~ComSocket();
    void setHost(const char* host);
    void setPort(uint16 port);
    void setInterface(const char* interface_name);
    void setBindIPv4(uint32_be ipv4);
    void setBindIPv6(uint8* ipv6);
    void setBindPort(uint16 port);

    void closeSocket();

protected:
    std::string host;
    uint16 port = 0;
    int socketfd = -1;
    std::string interface_name;
    uint32 bind_port = 0;
    uint32_be bind_ipv4 = 0;
    uint8 bind_ipv6[16] = {0};
    int timeout_recv_ms = 10000;
    int timeout_send_ms = 10000;
};

class COM_EXPORT ComSocketTcp : public ComSocket
{
public:
    ComSocketTcp();
    virtual ~ComSocketTcp();

    bool openSocket();
    int readData(uint8* buf, int buf_size, uint32 timeout_ms = 10000);
    int writeData(const void* data, int data_size);
};

class COM_EXPORT ComSocketUdp : public ComSocket
{
public:
    ComSocketUdp();
    virtual ~ComSocketUdp();

    bool openSocket(bool broadcast = false);
    int readData(uint8* buf, int buf_size, ComSocketAddr* addr_from = NULL, uint32 timeout_ms = 10000);
    int writeData(const void* data, int data_size);
private:
    ComSocketAddr addr_to;
};

class COM_EXPORT ComTcpClient : public ComSocketTcp
{
public:
    ComTcpClient();
    ComTcpClient(const char* host, uint16 port);
    virtual ~ComTcpClient();
    virtual bool startClient();
    virtual void stopClient();
    int send(const void* data, int data_size);
    bool waitForConnected(int timeout_ms = 0);
    void reconnect();
    void setReconnectInterval(int reconnect_interval_ms);
public:
    inline bool isConnected()
    {
        return connected;
    }
protected:
    virtual void onConnectionChanged(bool connected);
    virtual void onRecv(uint8* data, int data_size);
private:
    static void ThreadRX(ComTcpClient* socket_client);
private:
    std::atomic<bool> thread_receiver_running;
    std::thread thread_receiver;
    std::atomic<bool> reconnect_now;
    std::atomic<int> reconnect_interval_ms = {3000};
    std::atomic<bool> connected;
    ComCondition condition_connection;
};

class COM_EXPORT ComUnixDomainClient
{
public:
    ComUnixDomainClient(const char* server_file_name, const char* file_name = NULL);
    virtual ~ComUnixDomainClient();
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
    static void ThreadReceiver(ComUnixDomainClient* client);
private:
    std::atomic<int> socketfd;
    std::string file_name;
    std::string server_file_name;
    std::atomic<bool> thread_receiver_running;
    std::thread thread_receiver;
    std::atomic<bool> need_reconnect;
};

class COM_EXPORT ComMulticastNode : public ComSocket
{
public:
    ComMulticastNode();
    virtual ~ComMulticastNode();

    bool startNode();
    void stopNode();
    int send(const void* data, int data_size);

private:
    virtual void onRecv(uint8* data, int data_size);
    static void ThreadRX(ComMulticastNode* client);

private:
    std::atomic<bool> thread_rx_running = {false};
    std::thread thread_rx;
    uint8 buf[4096];
};

class COM_EXPORT ComMulticastNodeString : public ComMulticastNode
{
public:
    ComMulticastNodeString();
    virtual ~ComMulticastNodeString();

    using ComMulticastNode::send;
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

class COM_EXPORT ComStringIpcClient : protected ComTcpClient
{
public:
    ComStringIpcClient();
    virtual ~ComStringIpcClient();

    bool startIPC(const char* name, const char* host, uint16 port);
    void stopIPC();
    bool sendString(const char* name_to, const std::string& message);
    bool sendString(const char* name_to, const char* message);
    std::string getName();

protected:
    virtual void onMessage(const std::string& name, const std::string& message);
    virtual void onConnectionChanged(bool connected);
private:
    static void ThreadReceiver(ComStringIpcClient* ctx);
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

class COM_EXPORT ComStringIpcServer : protected ComTcpServer
{
public:
    ComStringIpcServer();
    virtual ~ComStringIpcServer();

    bool startIPC(const char* name, uint16 port);
    void stopIPC();
    bool sendString(const char* name_to, const std::string& message);
    bool sendString(const char* name_to, const char* message);
    std::string getName();

protected:
    virtual void onMessage(const std::string& name, const std::string& message);
private:
    static void ThreadReceiver(ComStringIpcServer* ctx);
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

