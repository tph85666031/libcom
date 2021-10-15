#ifndef __COM_TIMER_H__
#define __COM_TIMER_H__

#include "com_base.h"
#include "com_task.h"

typedef void (*fc_timer)(uint8_t id, void* user_arg);

class CPPTimer
{
    friend class CPPTimerManager;
public:
    CPPTimer();
    CPPTimer(uint8_t id, const std::string& task_name);
    CPPTimer(uint8_t id, const char* task_name);
    CPPTimer(uint8_t id, fc_timer fc, void* user_arg);
    virtual ~CPPTimer();

    CPPTimer& setType(const char* task_name);
    CPPTimer& setType(const std::string& task_name);
    CPPTimer& setType(fc_timer fc, void* user_arg);
    CPPTimer& setID(uint8_t id);
    CPPTimer& setInterval(int interval_ms);
    CPPTimer& setRepeat(bool repeat);
    bool start();
    void stop();
    bool restart();

    uint8_t getID();
    int getInterval();
    bool isStarted();
    bool isRepeat();
private:
    fc_timer fc = NULL;
    void* user_arg = NULL;
    std::string task_name;
    uint8_t id = 0;
    int interval_ms = 0;
    int interval_set_ms = 0;
    bool repeat = false;
    std::string uuid;
};

class CPPTimerManager : public ThreadPool
{
    friend class CPPTimer;
public:
    CPPTimerManager();
    virtual ~CPPTimerManager();

    void startTimerManager();
    void stopTimerManager();

    void updateTimer(CPPTimer& timer);
    void removeTimer(std::string uuid);
    bool isTimerExist(std::string uuid);
    void setMessageID(uint32_t id);
private:
    void threadPoolRunner(Message& msg);
    static void ThreadTimerLoop(CPPTimerManager* manager);
private:
    std::thread thread_timer_loop;
    std::vector<CPPTimer> timers;
    std::atomic<bool> running_loop = {false};
    CPPMutex mutex_timers{"mutex_timers"};
    std::atomic<uint32_t> message_id = {0};
};

void InitTimerManager();
void UninitTimerManager();

#endif /* __COM_TIMER_H__ */