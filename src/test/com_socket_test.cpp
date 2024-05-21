#include "com_socket.h"
#include "com_log.h"
#include "com_test.h"

class MySocketTcpClient : public SocketTcpClient
{
public:
    MySocketTcpClient(const char* host, uint16 port) : SocketTcpClient(host, port)
    {
        data_size = 0;
    }
    void onConnectionChanged(bool connected)
    {
        LOG_D("%s %s", getHost().c_str(), connected ? "true" : "false");
    }

    void onRecv(uint8* data, int data_size)
    {
        this->data_size += data_size;
        ComBytes bytes(data, data_size);
        //LOG_D("%s got = %s", getHost(), bytes.toHexString().c_str());
    }
    std::atomic<int> data_size;
};

class MySocketTcpServer : public SocketTcpServer
{
public:
    MySocketTcpServer(uint16 port) : SocketTcpServer(port)
    {
        data_size = 0;
    }
    void onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected)
    {
        LOG_D("%s connection %s", host.c_str(), connected ? "true" : "false");
    }

    void onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size)
    {
        this->data_size += data_size;
        ComBytes bytes(data, data_size);
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

    void onRecv(uint8* data, int data_size)
    {
        this->data_size += data_size;
        ComBytes bytes(data, data_size);
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

    void onRecv(std::string& client_name, int socketfd, uint8* data, int data_size)
    {
        this->data_size += data_size;
        ComBytes bytes(data, data_size);
        LOG_D("recv %s = %s", client_name.c_str(), bytes.toString().c_str());
    }
    std::atomic<int> data_size;
};

void com_socket_unit_test_suit(void** state)
{
    uint8 mac[LENGTH_MAC] = {0};
    std::vector<std::string> ifs = com_net_get_interface_all();
    ASSERT_FALSE(ifs.empty());
    ASSERT_TRUE(com_net_get_mac(ifs[0].c_str(), mac));
    std::string mac_str = com_string_format("%02X-%02X-%02X-%02X-%02X-%02X",
                                            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    LOG_D("if=%s, mac=%s", ifs[0].c_str(), mac_str.c_str());
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
    ASSERT_TRUE(socket_client1.startClient());
    ASSERT_TRUE(socket_client2.startClient());
    com_sleep_s(1);
    int count = 0;
    int data_size_client = 0;
    while(count < 1000)
    {
        std::string v = std::to_string(count);
        v.append(" ");
        data_size_client += v.size();
        ASSERT_INT_EQUAL(socket_client1.send((uint8*)v.data(), v.size()), v.size());
        data_size_client += v.size();
        ASSERT_INT_EQUAL(socket_client2.send((uint8*)v.data(), v.size()), v.size());
        count++;
    }
    com_sleep_s(1);
    socket_client1.stopClient();
    socket_client2.stopClient();
    ASSERT_INT_EQUAL(socket_server.data_size, data_size_client);
    MySocketTcpServer* x = new MySocketTcpServer(1897);
    delete x;
}

void com_unix_domain_unit_test_suit(void** state)
{
#if defined(_WIN32) || defined(_WIN64)
#else
    MyUnixDomainTcpServer ud_server("un_server_test");
    MyUnixDomainTcpClient ud_client1("un_server_test", "un_client1_test");

    ud_server.startServer();
    com_sleep_s(1);
    ud_client1.startClient();
    int count = 0;
    int data_size = 0;
    while(count < 1000)
    {
        std::string v = std::to_string(count);
        v.append(" ");
        data_size += v.size();
        ud_client1.send((uint8*)v.data(), v.size());
        //com_sleep_ms(1);
        count++;
    }
    com_sleep_s(1);
    ASSERT_INT_EQUAL(ud_server.data_size, data_size);
#endif
}

void com_socket_multicast_unit_test_suit(void** state)
{
    MulticastNodeString a;
    a.setHost("224.0.0.88");
    a.setPort(8888);

    a.startNode();

    MulticastNodeString b;
    b.setHost("224.0.0.88");
    b.setPort(8888);

    b.startNode();

    MulticastNodeString c;
    c.setHost("224.0.0.88");
    c.setPort(8888);

    c.startNode();

    //const char* x = "123\0456\0789\0";
    const char x[12] = {'1', '2', '3', '\0', '4', '5', '6', '\0', '7', '8', '9', '\0'};
    a.send(x, 12);

    com_sleep_s(1);
    ASSERT_INT_EQUAL(a.getCount(), 3);
    ASSERT_INT_EQUAL(b.getCount(), 3);
    ASSERT_INT_EQUAL(c.getCount(), 3);
}

class MyStringIPCClient : public StringIPCClient
{
public:
    MyStringIPCClient() {};
    virtual ~MyStringIPCClient() {};

    void onConnectionChanged(bool connected)
    {
        if(connected)
        {
            sendString("server", com_string_format("hello from %s", getName().c_str()).c_str());
        }
    }

    void onMessage(const std::string& name, const std::string& message)
    {
        LOG_D("from name=%s,message=%s", name.c_str(), message.c_str());
        counts[name]++;
    }

    std::map<std::string, int> counts;
};

class MyStringIPCServer : public StringIPCServer
{
public:
    MyStringIPCServer() {};
    virtual ~MyStringIPCServer() {};

    void onMessage(const std::string& name, const std::string& message)
    {
        //LOG_I("from name=%s,message=%s", name.c_str(), message.c_str());
        counts[name]++;
    }

    std::map<std::string, int> counts;
};

void com_socket_stringipc_unit_test_suit(void** state)
{
    MyStringIPCServer s;
    MyStringIPCClient c1;
    MyStringIPCClient c2;

    int test_count = 10000;
    s.startIPC("server", 1234);
    c1.startIPC("client1", "127.0.0.1", 1234);
    c2.startIPC("client2", "127.0.0.1", 1234);
    com_sleep_s(1);
    for(int i = 0; i < test_count; i++)
    {
        c1.sendString("server", std::to_string(i).c_str());
        c1.sendString("client2", std::to_string(i).c_str());

        c2.sendString("server", std::to_string(i).c_str());
        c2.sendString("client1", std::to_string(i).c_str());
    }

    com_sleep_s(4);
    ASSERT_INT_EQUAL(c1.counts["client2"], test_count);
    ASSERT_INT_EQUAL(c2.counts["client1"], test_count);
    ASSERT_INT_EQUAL(s.counts["client1"], test_count + 1);
    ASSERT_INT_EQUAL(s.counts["client2"], test_count + 1);
}

