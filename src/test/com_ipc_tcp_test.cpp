#include "com.h"

#if __linux__ == 1
class MyTcpIpcClient : public TcpIpcClient
{
public:
    MyTcpIpcClient(const char* name, const char* server_name, const char* host, uint16 port)
    {
        setName(name);
        setServerName(server_name);
        setHost(host);
        setPort(port);
        recv_count = 0;
    }

    void onMessage(std::string& name, uint8* data, int dataSize)
    {
        //std::string v;
        //v.append((char*)data, dataSize);
        //LOG_D("%s got message from %s,count=%d", getName().c_str(), name.c_str(), recv_count.load());
        //ASSERT_STR_EQUAL(name.c_str(), "client");
        //int id = std::stoi(v.c_str());
        //ASSERT_INT_EQUAL(id, recv_count);
        recv_count++;
    }
    int getRecvCount()
    {
        return recv_count;
    }
    void resetRecvCount()
    {
        recv_count = 0;
    }
private:
    std::atomic<int> recv_count;
};
#endif

void com_ipc_tcp_unit_test_suit(void** state)
{
#if __linux__ == 1
    TcpIpcServer ipcServer("server", 9000);
    MyTcpIpcClient ipcClient1("client1", "server", "127.0.0.1", 9000);
    MyTcpIpcClient ipcClient2("client2", "server", "127.0.0.1", 9000);
    MyTcpIpcClient ipcClient3("client3", "server", "127.0.0.1", 9000);
    MyTcpIpcClient ipcClient4("client4", "server", "127.0.0.1", 9000);
    MyTcpIpcClient ipcClient5("client5", "server", "127.0.0.1", 9000);
    MyTcpIpcClient ipcClient6("client6", "server", "127.0.0.1", 9000);
    MyTcpIpcClient ipcClient7("client7", "server", "127.0.0.1", 9000);
    com_sleep_s(1);

    int count_a = 50;
    char buf[2048];
    memset(buf, 'a', sizeof(buf));
    for(int i = 0; i < count_a; i++)
    {
        //int ret = snprintf(buf, sizeof(buf), "%u", i);
        ASSERT_TRUE(ipcClient1.sendMessage("client*", (uint8*)buf, sizeof(buf)));
    }
    com_sleep_s(5);

    LOG_D("ipcClient1 receive count = %d", ipcClient1.getRecvCount());
    LOG_D("ipcClient2 receive count = %d", ipcClient2.getRecvCount());
    LOG_D("ipcClient3 receive count = %d", ipcClient3.getRecvCount());
    LOG_D("ipcClient4 receive count = %d", ipcClient4.getRecvCount());
    LOG_D("ipcClient5 receive count = %d", ipcClient5.getRecvCount());
    LOG_D("ipcClient6 receive count = %d", ipcClient6.getRecvCount());
    LOG_D("ipcClient7 receive count = %d", ipcClient7.getRecvCount());
    ASSERT_INT_EQUAL(ipcClient1.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ipcClient2.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ipcClient3.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ipcClient4.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ipcClient5.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ipcClient6.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ipcClient7.getRecvCount(), count_a);
#endif
}

