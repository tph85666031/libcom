#include "com_thread.h"
#include "com_log.h"
#include "com_test.h"

class MyThreadPoll: public ThreadPool, public ThreadRunner<std::string>
{
public:
    void threadPoolRunner(Message& msg)
    {
        LOG_D("thread poll got message, id=%u, data=%s", msg.getID(), msg.getString("data").c_str());
    };

    void threadRunner(std::string& msg)
    {
        LOG_I("called,str=%s", msg.c_str());
        ASSERT_STR_EQUAL(msg.c_str(), "threadRunner");
    };
};

static void thread_cpp_test(int val1, int val2)
{
    LOG_D("vals=%d,val2=%d", val1, val2);
    LOG_D("tid=%llu", com_thread_get_tid());
    return;
}

void com_thread_unit_test_suit(void** state)
{
    LOG_I("current process name=%s", com_process_get_name().c_str());
    LOG_I("process name of %d is %s", com_process_get_pid(), com_process_get_name(com_process_get_pid()).c_str());
    LOG_I("current process id=%d", com_process_get_pid());
    LOG_I("current process ppid=%d", com_process_get_ppid());
    LOG_I("process id of %s,%d", "/usr/lib/bluetooth/obexd", com_process_get_pid("obexd"));
    LOG_I("process ppid of%s,%d", "/usr/lib/bluetooth/obexd",com_process_get_ppid(com_process_get_pid("/usr/lib/bluetooth/obexd")));

    com_thread_set_name("T testthread");
    LOG_D("thread name=%s", com_thread_get_name(com_thread_get_tid_posix()).c_str());
    MyThreadPoll pool;
    pool.setThreadsCount(3, 5);
    pool.startThreadPool();
    for(int i = 0; i < 100; i++)
    {
        Message msg;
        msg.setID(i);
        msg.set("data", std::to_string(i));
        pool.pushPoolMessage(msg);
    }
    pool.waitAllDone();
    std::thread t1(thread_cpp_test, 1, 2);
    if(t1.joinable())
    {
        t1.join();
    }

    MyThreadPoll x;
    x.pushRunnerMessage("threadRunner");
    com_sleep_s(3);
}

