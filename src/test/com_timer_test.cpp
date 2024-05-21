#include "com_timer.h"
#include "com_test.h"

void test_timer_callback(uint8 id, void* user_arg)
{
    LOG_D("timer_fc %d timeout,user_arg=%p", id, user_arg);
}

class TimerTestTask : public Task
{
public:
    TimerTestTask(std::string name, Message init_msg) : Task(name, init_msg)
    {
    };
    ~TimerTestTask()
    {
        LOG_D("%s quit", getName().c_str());
    }
private:
    void onMessage(Message& msg)
    {
        LOG_D("%s got message,id=%u", getName().c_str(), msg.getID());
    }
    virtual void onStart()
    {
        LOG_D("task %s start", getName().c_str());
    }
    virtual void onStop()
    {
        LOG_D("task %s stop", getName().c_str());
    }
};

void com_timer_unit_test_suit(void** state)
{
    com_log_set_level("DEBUG");
    GetTaskManager().createTask<TimerTestTask>("timer_test_task");
    ComTimer t1(1, "timer_test_task");
    t1.setInterval(1000).setRepeat(true);
    t1.start();

    ComTimer t2(2, test_timer_callback, NULL);
    t2.setInterval(100).setRepeat(true);
    t2.start();
    ASSERT_TRUE(t2.isStarted());

    ComTimer t3(3, test_timer_callback, NULL);
    t3.setInterval(100).setRepeat(true);
    t3.start();
    ASSERT_TRUE(t3.isStarted());

    com_sleep_s(4);
}