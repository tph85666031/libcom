#include "com.h"

#if __linux__ == 1
class MyUnixDomainIPCClient : public UnixDomainIPCClient
{
public:
    MyUnixDomainIPCClient(const char* server_file_path, const char* file_path): UnixDomainIPCClient(server_file_path, file_path)
    {
        recvCount = 0;
    }

    void onMessage(std::string& from_file_path, uint8_t* data, int data_size)
    {
        std::string v;
        v.append((char*)data, data_size);
        LOG_D("%s got message from %s:%s", getFileName().c_str(), from_file_path.c_str(), v.c_str());
        //ASSERT_STR_EQUAL(name.c_str(), "client");
        int id = std::stoi(v.c_str());
        ASSERT_INT_EQUAL(id, recvCount);
        recvCount++;
    }
    int getRecvCount()
    {
        return recvCount;
    }
    void resetRecvCount()
    {
        recvCount = 0;
    }
private:
    std::atomic<int> recvCount;
};
#endif

void com_ipc_ud_unit_test_suit(void** state)
{
#if __linux__ == 1
    UnixDomainIPCServer ud_server("server");
    MyUnixDomainIPCClient ud_client1("server", "client1");
    MyUnixDomainIPCClient ud_client2("server", "client2");
    MyUnixDomainIPCClient ud_client3("server", "client33");
    com_sleep_s(1);

    int count_a = 10;
    for (int i = 0; i < count_a; i++)
    {
        char buf[8];
        int ret = snprintf(buf, sizeof(buf), "%d", i);
        ASSERT_TRUE(ud_client1.sendMessage("client?", (uint8_t*)buf, ret));
        ud_client1.sendMessage("server", (uint8_t*)buf, ret);
    }

    com_sleep_s(1);
    ASSERT_INT_EQUAL(ud_client1.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ud_client2.getRecvCount(), count_a);
    ASSERT_INT_EQUAL(ud_client3.getRecvCount(), 0);

    int count_b = 10;
    ud_client1.resetRecvCount();
    ud_client2.resetRecvCount();
    ud_client3.resetRecvCount();
    for (int i = 0; i < count_b; i++)
    {
        char buf[8];
        int ret = snprintf(buf, sizeof(buf), "%d", i);
        ASSERT_TRUE(ud_client1.sendMessage("client*", (uint8_t*)buf, ret));
    }

    com_sleep_s(1);
    ASSERT_INT_EQUAL(ud_client1.getRecvCount(), count_b);
    ASSERT_INT_EQUAL(ud_client2.getRecvCount(), count_b);
    ASSERT_INT_EQUAL(ud_client3.getRecvCount(), count_b);
    LOG_D("ut done");
#endif
}
