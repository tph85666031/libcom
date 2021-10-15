#include "com.h"

#if __linux__ == 1
class MySocketTcpClient : public SocketTcpClient
{
public:
    MySocketTcpClient(const char* host, uint16_t port) : SocketTcpClient(host, port)
    {
        data_size = 0;
    }
    void onConnectionChanged(bool connected)
    {
        LOG_D("%s %s", getHost().c_str(), connected ? "true" : "false");
    }

    void onRecv(uint8_t* data, int data_size)
    {
        this->data_size += data_size;
        CPPBytes bytes(data, data_size);
        //LOG_D("%s got = %s", getHost(), bytes.toHexString().c_str());
    }
    std::atomic<int> data_size;
};

class MySocketTcpServer : public SocketTcpServer
{
public:
    MySocketTcpServer(uint16_t port) : SocketTcpServer(port)
    {
        data_size = 0;
    }
    void onConnectionChanged(std::string& host, uint16_t port, int socketfd, bool connected)
    {
        LOG_D("%s connection %s", host.c_str(), connected ? "true" : "false");
    }

    void onRecv(std::string& host, uint16_t port, int socketfd, uint8_t* data, int data_size)
    {
        this->data_size += data_size;
        CPPBytes bytes(data, data_size);
        //LOG_D("recv %s:%d = %s,data_size=%d", host, port, bytes.toString().c_str(),this->data_size.load());
    }
    std::atomic<int> data_size;
};

class MyUnixDomainTcpClient : public UnixDomainTcpClient
{
public:
    MyUnixDomainTcpClient(const char* server_name, const char* name) : UnixDomainTcpClient(server_name, name)
    {
        data_size = 0;
    }
    void onConnectionChanged(bool connected)
    {
        LOG_D("%s %s", getFileName().c_str(), connected ? "true" : "false");
    }

    void onRecv(uint8_t* data, int data_size)
    {
        this->data_size += data_size;
        CPPBytes bytes(data, data_size);
        //LOG_D("%s got = %s", getFileName(), bytes.toHexString().c_str());
    }
    std::atomic<int> data_size;
};

class MyUnixDomainTcpServer : public UnixDomainTcpServer
{
public:
    MyUnixDomainTcpServer(const char* name) : UnixDomainTcpServer(name)
    {
        data_size = 0;
    }
    void onConnectionChanged(std::string& client_name, int socketfd, bool connected)
    {
        LOG_D("%s connection %s", client_name.c_str(), connected ? "true" : "false");
    }

    void onRecv(std::string& client_name,  int socketfd, uint8_t* data, int data_size)
    {
        this->data_size += data_size;
        CPPBytes bytes(data, data_size);
        LOG_D("recv %s = %s", client_name.c_str(), bytes.toString().c_str());
    }
    std::atomic<int> data_size;
};
#endif

void com_socket_unit_test_suit(void** state)
{
#if __linux__ == 1
    uint8_t mac[LENGTH_MAC]={0};
    com_net_get_mac("eth0", mac);
    std::string mac_str = com_string_format("%02X-%02X-%02X-%02X-%02X-%02X",
                                            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    LOG_D("mac=%s", mac_str.c_str());
    NicInfo nic;
    com_net_get_nic("eth0", nic);
    LOG_D("nic:%s,up=%s,mac_type=%d,mac=%02X-%02X-%02X-%02X-%02X-%02X,ip=%s,ip_mask=%s,ip_broadcast=%s",
          nic.name.c_str(), nic.flags & 0x02 ? "true" : "false", nic.addr_family,
          nic.mac[0], nic.mac[1], nic.mac[2],
          nic.mac[3], nic.mac[4], nic.mac[5],
          nic.ip.c_str(), nic.ip_mask.c_str(), nic.ip_broadcast.c_str());

    MySocketTcpServer socket_server(9000);
    ASSERT_INT_EQUAL(socket_server.startServer(), 0);
    com_sleep_s(1);
    MySocketTcpClient socket_client1("127.0.0.1", 9000);
    MySocketTcpClient socket_client2("127.0.0.1", 9000);
    ASSERT_INT_EQUAL(socket_client1.startClient(), 0);
    ASSERT_INT_EQUAL(socket_client2.startClient(), 0);
    com_sleep_s(1);
    int count = 0;
    int data_size = 0;
    while (count < 1000)
    {
        std::string v = std::to_string(count);
        v.append(" ");
        data_size += v.size();
        ASSERT_INT_EQUAL(socket_client1.send((uint8_t*)v.data(), v.size()), v.size());
        data_size += v.size();
        ASSERT_INT_EQUAL(socket_client2.send((uint8_t*)v.data(), v.size()), v.size());
        count++;
    }
    com_sleep_s(1);
    socket_client1.stopClient();
    socket_client2.stopClient();
    ASSERT_INT_EQUAL(socket_server.data_size, data_size);
    MySocketTcpServer* x = new MySocketTcpServer(1897);
    delete x;
#endif
}

void com_unix_domain_unit_test_suit(void** state)
{
#if __linux__ == 1
    MyUnixDomainTcpServer ud_server("un_server_test");
    MyUnixDomainTcpClient ud_client1("un_server_test", "un_client1_test");

    ud_server.startServer();
    com_sleep_s(1);
    ud_client1.startClient();
    int count = 0;
    int data_size = 0;
    while (count < 1000)
    {
        std::string v = std::to_string(count);
        v.append(" ");
        data_size += v.size();
        ud_client1.send((uint8_t*)v.data(), v.size());
        //com_sleep_ms(1);
        count++;
    }
    com_sleep_s(1);
    ASSERT_INT_EQUAL(ud_server.data_size, data_size);
#endif
}