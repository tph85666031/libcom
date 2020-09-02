#include "com.h"

class MyThreadPoll: public ThreadPool
{
public:
    void run(Message& msg)
    {
        LOG_D("thread poll got message, id=%u, data=%s", msg.getID(), msg.getString("data").c_str());
    }
};

class MyThread : public EasyThread<Message>
{
public:
    void onLoop()
    {
        LOG_D("running ...");
        if (waitMessage(100))
        {
            LOG_D("got message");
            Message msg;
            if (getMessage(msg))
            {
                ASSERT_INT_EQUAL(msg.getID(), 3);
                ASSERT_STR_EQUAL(getName().c_str(), "MyThread");
            }
        }
    }
};

class MyThread2 : public EasyThread<std::string>
{
public:
    void onLoop()
    {
        LOG_D("running ...");
        if (waitMessage(100))
        {
            LOG_D("got message");
            std::string msg;
            if (getMessage(msg))
            {
                ASSERT_STR_EQUAL(msg.c_str(), "haha");
            }
        }
    }
};

static void thread_cpp_test(int val1, int val2)
{
    LOG_D("vals=%d,val2=%d", val1, val2);
    LOG_D("tid=%llu", com_thread_get_tid());
    return;
}

void com_thread_unit_test_suit(void** state)
{
    com_thread_set_name("T testthread");
    LOG_D("thread name=%s", com_thread_get_name(com_thread_get_tid_posix()).c_str());
    MyThreadPoll pool;
    pool.setThreadsCount(3, 5);
    pool.startThreadPool();
    for (int i = 0; i < 100; i++)
    {
        Message msg;
        msg.setID(i);
        msg.set("data", std::to_string(i));
        pool.pushMessage(msg);
    }
    pool.waitAllDone();
    std::thread t1(thread_cpp_test, 1, 2);
    if (t1.joinable())
    {
        t1.join();
    }

    MyThreadPoll x;
    
    MyThread my_thread;
    MyThread2 my_thread2;
    Message msg(3);
    my_thread.setName("MyThread");
    my_thread.startThread();
    my_thread.pushMessage(msg);
    my_thread2.pushMessage("haha");
    LOG_D("send done");

    com_sleep_s(2);
    my_thread.stopThread();
}

