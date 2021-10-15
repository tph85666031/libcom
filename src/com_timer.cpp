#include <vector>
#include <thread>
#include "com_timer.h"
#include "com_file.h"
#include "com_config.h"
#include "com_log.h"

#define TIMER_INTERVAL_MIN_MS 100

static CPPTimerManager& GetTimerManager()
{
    static CPPTimerManager timer_manager;
    return timer_manager;
}

void InitTimerManager()
{
    CPPTimerManager& timer_manager = GetTimerManager();
    timer_manager.setThreadsCount(1, 10).setQueueSize(10);
    timer_manager.startTimerManager();
}

void UninitTimerManager()
{
    GetTimerManager().stopTimerManager();
}

CPPTimerManager::CPPTimerManager()
{
}

CPPTimerManager::~CPPTimerManager()
{
    stopTimerManager();
}

void CPPTimerManager::startTimerManager()
{
    stopTimerManager();
    running_loop = true;
    thread_timer_loop = std::thread(ThreadTimerLoop, this);
    startThreadPool();
}

void CPPTimerManager::stopTimerManager()
{
    running_loop = false;
    if(thread_timer_loop.joinable())
    {
        thread_timer_loop.join();
    }
    stopThreadPool();
}

void CPPTimerManager::threadPoolRunner(Message& msg)
{
    uint8_t id  = msg.getUInt8("id", 0);
    fc_timer fc = (fc_timer)com_number_to_ptr(msg.getUInt64("fc"));
    void* arg = com_number_to_ptr(msg.getUInt64("arg"));
    if(fc)
    {
        fc(id, arg);
    }
}

void CPPTimerManager::ThreadTimerLoop(CPPTimerManager* manager)
{
    LOG_I("running...");
    std::vector<Message> timeout_msgs;
    uint64_t time_start = 0;
    com_thread_set_name("T-TimerMGR");

    while(manager->running_loop)
    {
        time_start = com_time_cpu_ms();
        timeout_msgs.clear();
        manager->mutex_timers.lock();
        std::vector<CPPTimer>::iterator it;
        for(it = manager->timers.begin(); it != manager->timers.end();)
        {
            CPPTimer timer = *it;
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
                msg.set("fc", com_ptr_to_number((const void*)timer.fc));
                msg.set("arg", com_ptr_to_number(timer.user_arg));
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
            uint64_t fc = timeout_msgs[i].getUInt64("fc");
            if(task_name.empty() == false)
            {
                GetTaskManager().sendMessage(task_name.c_str(), timeout_msgs[i]);
            }
            if(fc > 0)
            {
                manager->pushMessage(timeout_msgs[i]);
            }
        }
        int64_t remain_sleep = TIMER_INTERVAL_MIN_MS - (com_time_cpu_ms() - time_start);
        if(remain_sleep > 0)
        {
            com_sleep_ms(remain_sleep);
        }
    }
    LOG_I("quit...");
    return;
}

bool CPPTimerManager::isTimerExist(std::string uuid)
{
    AutoMutex a(mutex_timers);
    for(size_t i = 0; i < timers.size(); i++)
    {
        if(timers[i].uuid == uuid)
        {
            return true;
        }
    }
    return false;
}

void CPPTimerManager::removeTimer(std::string uuid)
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

void CPPTimerManager::updateTimer(CPPTimer& timer)
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

void CPPTimerManager::setMessageID(uint32_t id)
{
    this->message_id = id;
}

CPPTimer::CPPTimer()
{
    uuid = com_uuid_generator();
}

CPPTimer::CPPTimer(uint8_t id, const std::string& task_name)
{
    this->id = id;
    this->task_name = task_name;
    uuid = com_uuid_generator();
}


CPPTimer::CPPTimer(uint8_t id, const char* task_name)
{
    if(task_name != NULL)
    {
        this->task_name = task_name;
    }
    this->id = id;
    uuid = com_uuid_generator();
}

CPPTimer::CPPTimer(uint8_t id, fc_timer fc, void* user_arg)
{
    this->id = id;
    this->fc = fc;
    this->user_arg = user_arg;
    uuid = com_uuid_generator();
}

CPPTimer::~CPPTimer()
{
}

uint8_t CPPTimer::getID()
{
    return this->id;
}

int CPPTimer::getInterval()
{
    return this->interval_set_ms;
}

CPPTimer& CPPTimer::setType(const std::string& task_name)
{
    this->task_name = task_name;
    uuid = com_uuid_generator();
    return *this;
}

CPPTimer& CPPTimer::setType(const char* task_name)
{
    if(task_name != NULL)
    {
        this->task_name = task_name;
    }
    uuid = com_uuid_generator();
    return *this;
}

CPPTimer& CPPTimer::setType(fc_timer fc, void* user_arg)
{
    this->fc = fc;
    this->user_arg = user_arg;
    uuid = com_uuid_generator();
    return *this;
}

CPPTimer& CPPTimer::setID(uint8_t id)
{
    this->id = id;
    return *this;
}

CPPTimer& CPPTimer::setInterval(int interval_ms)
{
    this->interval_set_ms = interval_ms;
    return *this;
}

CPPTimer& CPPTimer::setRepeat(bool repeat)
{
    this->repeat = repeat;
    return *this;
}

bool CPPTimer::start()
{
    LOG_D("timer start, uuid=%s", uuid.c_str());
    if(interval_set_ms <= 0 || id == 0)
    {
        LOG_E("arg incorrect");
        return false;
    }
    if(task_name.empty() && fc == NULL)
    {
        LOG_E("task OR ipc OR fc is not set");
        return false;
    }
    GetTimerManager().updateTimer(*this);
    return true;
}

void CPPTimer::stop()
{
    if(GetTimerManager().isTimerExist(this->uuid))
    {
        LOG_D("timer stop, uuid=%s", uuid.c_str());
        GetTimerManager().removeTimer(this->uuid);
    }
}

bool CPPTimer::restart()
{
    return start();
}

bool CPPTimer::isStarted()
{
    return GetTimerManager().isTimerExist(this->uuid);
}

bool CPPTimer::isRepeat()
{
    return repeat;
}

