#ifndef __COM_TIMER_H__
#define __COM_TIMER_H__

#include "com_base.h"
#include "com_task.h"

typedef void (*fc_timer)(uint8 id, void* ctx);

class COM_EXPORT ComTimer
{
    friend class ComTimerManager;
public:
    ComTimer();
    ComTimer(uint8 id, const std::string& task_name);
    ComTimer(uint8 id, const char* task_name);
    ComTimer(uint8 id, fc_timer fc, void* ctx);
    virtual ~ComTimer();

    ComTimer& setType(const char* task_name);
    ComTimer& setType(const std::string& task_name);
    ComTimer& setType(fc_timer fc, void* ctx);
    ComTimer& setID(uint8 id);
    ComTimer& setInterval(int interval_ms);
    ComTimer& setRepeat(bool repeat);
    bool start();
    void stop();
    bool restart();

    uint8 getID();
    int getInterval();
    bool isStarted();
    bool isRepeat();
private:
    fc_timer fc = NULL;
    void* ctx = NULL;
    std::string task_name;
    uint8 id = 0;
    int interval_ms = 0;
    int interval_set_ms = 0;
    bool repeat = false;
    std::string uuid;
};

class COM_EXPORT ComTimerManager : public ComThreadPool<Message>
{
    friend class ComTimer;
public:
    ComTimerManager();
    virtual ~ComTimerManager();
    
    void setMessageID(uint32 id);
    uint32 getMessageID();
    void stopTimerManager();
private:
    void updateTimer(ComTimer& timer);
    void removeTimer(std::string uuid);
    bool isTimerExist(std::string uuid);
    void startTimerManager();
    void threadPoolRunner(Message& msg);
    static void ThreadTimerLoop(ComTimerManager* manager);
private:
    std::thread thread_timer_loop;
    std::vector<ComTimer> timers;
    std::atomic<bool> running_loop = {false};
    std::mutex mutex_timers;
    std::atomic<uint32> message_id = {0xFFFFFFFF};
};

COM_EXPORT ComTimerManager& GetTimerManager();

#endif /* __COM_TIMER_H__ */