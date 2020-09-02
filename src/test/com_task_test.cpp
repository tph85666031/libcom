#include "com.h"

class TestTask : public Task
{
public:
    TestTask(std::string name, Message init_msg) : Task(name, init_msg)
    {
        tid = 0;
        count = 0;
        addListener(1);
        addListener(4, 5, 6, 7, 8, 9, 10);
        ASSERT_STR_EQUAL(init_msg.getString("host").c_str(), "127.0.0.1");
        if (name == "task1")
        {
            ASSERT_INT_EQUAL(init_msg.getUInt16("port"), 0);
        }
        else if (name == "task2")
        {
            ASSERT_INT_EQUAL(init_msg.getUInt16("port"), 3610);
        }
        else if (name == "task3")
        {
            ASSERT_INT_EQUAL(init_msg.getUInt16("port"), 3610);
        }
    };
    ~TestTask()
    {
        removeListener(4, 5, 6);
        LOG_I("%s quit,flag=%x", getName().c_str(), flag);
        if (flag != 0x07)
        {
            LOG_E("ut failed, flag must be 7");
        }
        ASSERT_INT_EQUAL(flag, 7);
    }
private:
    void onMessage(Message& msg)
    {
        if (tid == 0)
        {
            tid = com_thread_get_pid();
        }
        ASSERT_INT_EQUAL(tid, com_thread_get_pid());
        //LOG_D("%s got message,id=%u,val=%s", getName().c_str(), msg.getID(), msg.getString("data").c_str());
        count++;
        flag |= 0x02;
    }
    void onStart()
    {
        if (tid == 0)
        {
            tid = com_thread_get_pid();
        }
        ASSERT_INT_EQUAL(tid, com_thread_get_pid());
        LOG_I("task %s start", getName().c_str());
        flag |= 0x01;
    }
    void onStop()
    {
        if (tid == 0)
        {
            tid = com_thread_get_pid();
        }
        ASSERT_INT_EQUAL(tid, com_thread_get_pid());
        LOG_I("task %s stop", getName().c_str());
        flag |= 0x04;
    }
private:
    std::atomic<int> count = {0};
    std::atomic<uint64> tid = {0};
    uint32 flag = 0;
};

static void test_thread(std::string name, int count)
{
    Message msg;
    for (int i = 0; i < count; i++)
    {
        msg.setID(1);
        msg.set("data", std::to_string(i));
        GetTaskManager().sendMessage(name.c_str(), msg);
    }
}

void com_task_unit_test_suit(void** state)
{
    Message msg;
    msg.set("host", "127.0.0.1");
    GetTaskManager().createTask<TestTask>("task1", msg);
    msg.set("port", 3610);
    GetTaskManager().createTask<TestTask>("task2", msg);
    GetTaskManager().createTask<TestTask>("task3", msg);
#if 0
    Message msg(1);
    msg.set("data", "test message");
    GetTaskManager().sendMessage("task1", msg);
    GetTaskManager().sendMessage("task2", msg);
    GetTaskManager().sendMessage("task3", msg);
    GetTaskManager().sendMessage("task1", msg);
    GetTaskManager().sendMessage("task2", msg);
    GetTaskManager().sendMessage("task3", msg);
    GetTaskManager().sendBroadcastMessage(msg);
    com_sleep_ms(100);
    LOG_D("ut quit,%s", GetTaskManager().getTask("task1")->getName().c_str());
    //GetTaskManager().destroyTask("task1");
#endif
    std::thread t1(test_thread, "task1", 10000);
    std::thread t2(test_thread, "task2", 10000);
    std::thread t3(test_thread, "task3", 10000);

    com_sleep_s(1);

    if (t1.joinable())
    {
        t1.join();
    }
    if (t2.joinable())
    {
        t2.join();
    }
    if (t3.joinable())
    {
        t3.join();
    }
    GetTaskManager().destroyTask("task1");

    ASSERT_FALSE(GetTaskManager().isTaskExist("task1"));
    ASSERT_TRUE(GetTaskManager().isTaskExist("task2"));
    ASSERT_TRUE(GetTaskManager().isTaskExist("task3"));
}

