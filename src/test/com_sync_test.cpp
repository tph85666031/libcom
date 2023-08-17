#include "com_sync.h"
#include "com_log.h"
#include "com_test.h"

class SyncTest
{
public:
    SyncTest()
    {
        running = true;
        thread_test = std::thread(ThreadTest, this);
    }
    ~SyncTest()
    {
        running = false;
        if(thread_test.joinable())
        {
            thread_test.join();
        }
    }
    std::string send(int timeout_ms)
    {
        uint64 uuid = 2;
        GetSyncManager().getAdapter("sync_test").syncPrepare(uuid);
        flag = uuid;//send message with uuid to other thread and wait reply
        CPPBytes bytes;
        GetSyncManager().getAdapter("sync_test").syncWait(uuid, bytes, timeout_ms);
        return bytes.toString();
    }

    static void ThreadTest(SyncTest* ctx)
    {
        while(ctx->running)
        {
            if(ctx->flag > 0)
            {
                LOG_I("send ack");
                GetSyncManager().getAdapter("sync_test").syncPost(ctx->flag, "this is reply", sizeof("this is reply"));
                ctx->flag = 0;
            }
            com_sleep_ms(100);
        }
    }

    std::thread thread_test;
    bool running = false;
    std::atomic<uint64> flag = {0};
};


void com_sync_unit_test_suit(void** state)
{
    SyncTest sync_test;
    std::string reply = sync_test.send(1000);
    LOG_I("reply=%s", reply.c_str());
    ASSERT_STR_EQUAL(reply.c_str(), "this is reply");

    reply = sync_test.send(1);
    LOG_I("reply=%s", reply.c_str());
    ASSERT_STR_EQUAL(reply.c_str(), "");

    com_sleep_s(1);
}

