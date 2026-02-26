#include "com_task.h"
#include "com_test.h"

class TestTask : public ComTask
{
public:
    TestTask(std::string name, Message msg) : ComTask(name, msg)
    {
        pid = 0;
        count = 0;
        addListener(1);
        addListener(4, 5, 6, 7, 8, 9, 10);
        ASSERT_STR_EQUAL(msg.getString("host").c_str(), "127.0.0.1");
        if(name == "task1")
        {
            ASSERT_INT_EQUAL(msg.getUInt16("port"), 0);
        }
        else if(name == "task2")
        {
            ASSERT_INT_EQUAL(msg.getUInt16("port"), 3610);
        }
        else if(name == "task3")
        {
            ASSERT_INT_EQUAL(msg.getUInt16("port"), 3610);
        }
    };
    ~TestTask()
    {
        removeListener(4, 5, 6);
        LOG_I("%s quit,flag=%x", getName().c_str(), flag);
        if(flag != 0x07)
        {
            LOG_E("ut failed, flag must be 7");
        }
        ASSERT_INT_EQUAL(flag, 7);
    }
private:
    void onMessage(Message& msg)
    {
        if(pid == 0)
        {
            pid = com_process_get_pid();
        }
        ASSERT_INT_EQUAL(pid, com_process_get_pid());
        //LOG_D("%s got message,id=%u,val=%s", getName().c_str(), msg.getID(), msg.getString("data").c_str());
        count++;
        flag |= 0x02;
        GetComTaskManager().sendAck(msg, "this is reply", sizeof("this is reply"));
    }
    void onStart()
    {
        if(pid == 0)
        {
            pid = com_process_get_pid();
        }
        ASSERT_INT_EQUAL(pid, com_process_get_pid());
        LOG_I("task %s start", getName().c_str());
        flag |= 0x01;
    }
    void onStop()
    {
        if(pid == 0)
        {
            pid = com_process_get_pid();
        }
        ASSERT_INT_EQUAL(pid, com_process_get_pid());
        LOG_I("task %s stop", getName().c_str());
        flag |= 0x04;
    }
private:
    std::atomic<int> count = {0};
    std::atomic<uint64> pid = {0};
    uint32 flag = 0;
};

static void test_thread(std::string name, int count)
{
    Message msg;
    for(int i = 0; i < count; i++)
    {
        msg.setID(1);
        msg.set("data", std::to_string(i));
        GetComTaskManager().sendMessage(name.c_str(), msg);
    }
}

void com_task_unit_test_suit(void** state)
{
    Message msg;
    msg.set("host", "127.0.0.1");
    GetComTaskManager().createTask<TestTask>("task1", msg);
    msg.set("port", 3610);
    GetComTaskManager().createTask<TestTask>("task2", msg);
    GetComTaskManager().createTask<TestTask>("task3", msg);
#if 0
    Message msg(1);
    msg.set("data", "test message");
    GetComTaskManager().sendMessage("task1", msg);
    GetComTaskManager().sendMessage("task2", msg);
    GetComTaskManager().sendMessage("task3", msg);
    GetComTaskManager().sendMessage("task1", msg);
    GetComTaskManager().sendMessage("task2", msg);
    GetComTaskManager().sendMessage("task3", msg);
    GetComTaskManager().sendBroadcastMessage(msg);
    com_sleep_ms(100);
    LOG_D("ut quit,%s", GetTaskManager().getTask("task1")->getName().c_str());
    //GetComTaskManager().destroyTask("task1");
#endif
    std::thread t1(test_thread, "task1", 10000);
    std::thread t2(test_thread, "task2", 10000);
    std::thread t3(test_thread, "task3", 10000);

    com_sleep_s(1);

    if(t1.joinable())
    {
        t1.join();
    }
    if(t2.joinable())
    {
        t2.join();
    }
    if(t3.joinable())
    {
        t3.join();
    }


    ComBytes reply = GetComTaskManager().sendMessageAndWait("task1", msg, 1000);
    LOG_I("reply=%s", reply.toString().c_str());

    GetComTaskManager().destroyTask("task1");

    ASSERT_FALSE(GetComTaskManager().isTaskExist("task1"));
    ASSERT_TRUE(GetComTaskManager().isTaskExist("task2"));
    ASSERT_TRUE(GetComTaskManager().isTaskExist("task3"));

    //GetComTaskManager().destroyTaskAll();
    GetComWorkerManager().createWorker("w1", [](Message msg, std::atomic<bool>& running)
    {
        LOG_I("w1 start");
        while(running)
        {
            LOG_I("w1 running");
            com_sleep_s(1);
        }
        LOG_I("w1 quit");
    });
    com_sleep_s(5);
    GetComWorkerManager().destroyWorker("w1");
    com_sleep_s(3);
}

