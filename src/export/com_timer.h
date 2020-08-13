#ifndef __BCP_TIMER_H__
#define __BCP_TIMER_H__

#include "com_base.h"
#include "com_task.h"

typedef void (*fc_timer)(uint8 id, void* user_arg);

class CPPTimer final
{
    friend class CPPTimerManager;
public:
    CPPTimer();
    CPPTimer(uint8 id, const std::string& task_name);
    CPPTimer(uint8 id, const char* task_name);
    CPPTimer(uint8 id, fc_timer fc, void* user_arg);
    ~CPPTimer();

    CPPTimer& setType(const char* task_name);
    CPPTimer& setType(const std::string& task_name);
    CPPTimer& setType(fc_timer fc, void* user_arg);
    CPPTimer& setID(uint8 id);
    CPPTimer& setInterval(int interval_ms);
    CPPTimer& setRepeat(bool repeat);
    bool start();
    void stop();
    bool restart();

    uint8 getID();
    int getInterval();
    bool isStarted();
    bool isRepeat();
private:
    fc_timer fc = NULL;
    void* user_arg = NULL;
    std::string task_name;
    uint8 id = 0;
    int interval_ms = 0;
    int interval_set_ms = 0;
    bool repeat = false;
    std::string uuid;
};

class CPPTimerManager final : private ThreadPool
{
    friend class CPPTimer;
public:
    CPPTimerManager();
    ~CPPTimerManager();

    void updateTimer(CPPTimer& timer);
    void removeTimer(std::string uuid);
    bool isTimerExist(std::string uuid);
private:
    void threadPoolRunner(Message& msg);
    static void ThreadTimerLoop(CPPTimerManager* manager);
private:
    std::thread thread_timer_loop;
    std::vector<CPPTimer> timers;
    std::atomic<bool> running_loop = {false};
    CPPMutex mutex_timers{"mutex_timers"};
    std::atomic<uint32> message_id = {0};
};

#endif /* __BCP_TIMER_H__ */