#include <vector>
#include <thread>
#include "com_timer.h"
#include "com_file.h"
#include "com_config.h"
#include "com_log.h"

#define TIMER_INTERVAL_MIN_MS 100

ComTimerManager& GetTimerManager()
{
    static ComTimerManager timer_manager;
    return timer_manager;
}

ComTimerManager::ComTimerManager()
{
    setThreadsCount(1, 10).setQueueSize(10);
    startTimerManager();
}

ComTimerManager::~ComTimerManager()
{
    LOG_I("called");
    stopTimerManager();
}

void ComTimerManager::startTimerManager()
{
    LOG_I("called");
    stopTimerManager();
    running_loop = true;
    thread_timer_loop = std::thread(ThreadTimerLoop, this);
    startThreadPool();
}

void ComTimerManager::stopTimerManager()
{
    LOG_I("called");
    running_loop = false;
    if(thread_timer_loop.joinable())
    {
        thread_timer_loop.join();
    }
    stopThreadPool(true);
}

void ComTimerManager::threadPoolRunner(Message& msg)
{
    uint8 id  = msg.getUInt8("id", 0);
    fc_timer fc = (fc_timer)msg.getPtr("fc");
    void* ctx = msg.getPtr("ctx");
    if(fc)
    {
        fc(id, ctx);
    }
}

void ComTimerManager::ThreadTimerLoop(ComTimerManager* manager)
{
    LOG_I("running...");
    std::vector<Message> timeout_msgs;
    uint64 time_start = 0;
    com_thread_set_name("T-TimerMGR");

    while(manager->running_loop)
    {
        time_start = com_time_cpu_ms();
        timeout_msgs.clear();
        manager->mutex_timers.lock();
        std::vector<ComTimer>::iterator it;
        for(it = manager->timers.begin(); it != manager->timers.end();)
        {
            ComTimer timer = *it;
            timer.interval_ms -= TIMER_INTERVAL_MIN_MS;
            if(timer.interval_ms > 0)
            {
                *it = timer;
                it++;
                continue;
            }
            if(timer.task_name.empty() == false)
            {
                Message msg;
                msg.setID(manager->message_id);
                msg.set("id", timer.id);
                msg.set("task", timer.task_name);
                timeout_msgs.push_back(msg);
            }

            if(timer.fc != NULL)
            {
                Message msg;
                msg.setID(manager->message_id);
                msg.set("id", timer.id);
                msg.setPtr("fc", (const void*)timer.fc);
                msg.setPtr("ctx", timer.ctx);
                timeout_msgs.push_back(msg);
            }

            if(timer.repeat && timer.interval_set_ms > 0)
            {
                timer.interval_ms = timer.interval_set_ms;
                *it = timer;
                it++;
                continue;
            }
            //remove it
            it = manager->timers.erase(it);
        }
        manager->mutex_timers.unlock();

        for(size_t i = 0; i < timeout_msgs.size(); i++)
        {
            std::string task_name = timeout_msgs[i].getString("task");
            void* fc = timeout_msgs[i].getPtr("fc");
            if(task_name.empty() == false)
            {
                GetTaskManager().sendMessage(task_name.c_str(), timeout_msgs[i]);
            }
            
            if(fc != NULL)
            {
                manager->pushPoolMessage(timeout_msgs[i]);
            }
        }
        int64 remain_sleep = TIMER_INTERVAL_MIN_MS - (com_time_cpu_ms() - time_start);
        if(remain_sleep > 0)
        {
            com_sleep_ms(remain_sleep);
        }
    }
    LOG_I("quit...");
    return;
}

bool ComTimerManager::isTimerExist(std::string uuid)
{
    std::lock_guard<std::mutex> lck(mutex_timers);
    for(size_t i = 0; i < timers.size(); i++)
    {
        if(timers[i].uuid == uuid)
        {
            return true;
        }
    }
    return false;
}

void ComTimerManager::removeTimer(std::string uuid)
{
    mutex_timers.lock();
    for(size_t i = 0; i < timers.size(); i++)
    {
        if(timers[i].uuid == uuid)
        {
            timers.erase(timers.begin() + i);
            break;
        }
    }
    mutex_timers.unlock();
    return;
}

void ComTimerManager::updateTimer(ComTimer& timer)
{
    mutex_timers.lock();
    for(size_t i = 0; i < timers.size(); i++)
    {
        if(timers[i].uuid == timer.uuid)
        {
            timers.erase(timers.begin() + i);
            break;
        }
    }
    timer.interval_ms = timer.interval_set_ms;
    timers.push_back(timer);
    mutex_timers.unlock();
    return;
}

void ComTimerManager::setMessageID(uint32 id)
{
    this->message_id = id;
}

uint32 ComTimerManager::getMessageID()
{
    return message_id;
}

ComTimer::ComTimer()
{
    uuid = com_uuid_generator();
}

ComTimer::ComTimer(uint8 id, const std::string& task_name)
{
    this->id = id;
    this->task_name = task_name;
    uuid = com_uuid_generator();
}


ComTimer::ComTimer(uint8 id, const char* task_name)
{
    if(task_name != NULL)
    {
        this->task_name = task_name;
    }
    this->id = id;
    uuid = com_uuid_generator();
}

ComTimer::ComTimer(uint8 id, fc_timer fc, void* ctx)
{
    this->id = id;
    this->fc = fc;
    this->ctx = ctx;
    uuid = com_uuid_generator();
}

ComTimer::~ComTimer()
{
}

uint8 ComTimer::getID()
{
    return this->id;
}

int ComTimer::getInterval()
{
    return this->interval_set_ms;
}

ComTimer& ComTimer::setType(const std::string& task_name)
{
    this->task_name = task_name;
    uuid = com_uuid_generator();
    return *this;
}

ComTimer& ComTimer::setType(const char* task_name)
{
    if(task_name != NULL)
    {
        this->task_name = task_name;
    }
    uuid = com_uuid_generator();
    return *this;
}

ComTimer& ComTimer::setType(fc_timer fc, void* ctx)
{
    this->fc = fc;
    this->ctx = ctx;
    uuid = com_uuid_generator();
    return *this;
}

ComTimer& ComTimer::setID(uint8 id)
{
    this->id = id;
    return *this;
}

ComTimer& ComTimer::setInterval(int interval_ms)
{
    this->interval_set_ms = interval_ms;
    return *this;
}

ComTimer& ComTimer::setRepeat(bool repeat)
{
    this->repeat = repeat;
    return *this;
}

bool ComTimer::start()
{
    LOG_D("timer start, uuid=%s", uuid.c_str());
    if(interval_set_ms <= 0 || id == 0)
    {
        LOG_E("arg incorrect");
        return false;
    }
    if(task_name.empty() && fc == NULL)
    {
        LOG_E("task OR fc is not set");
        return false;
    }
    GetTimerManager().updateTimer(*this);
    return true;
}

void ComTimer::stop()
{
    if(GetTimerManager().isTimerExist(this->uuid))
    {
        LOG_D("timer stop, uuid=%s", uuid.c_str());
        GetTimerManager().removeTimer(this->uuid);
    }
}

bool ComTimer::restart()
{
    return start();
}

bool ComTimer::isStarted()
{
    return GetTimerManager().isTimerExist(this->uuid);
}

bool ComTimer::isRepeat()
{
    return repeat;
}

