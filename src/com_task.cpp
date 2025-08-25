#include "com_task.h"
#include "com_sync.h"
#include "com_log.h"

TaskManager& GetTaskManager()
{
    static TaskManager task_manager;
    return task_manager;
}

Task::Task(std::string name, Message msg)
{
    this->name = name;
    running = false;
}

Task::~Task()
{
}

void Task::startTask()
{
    running = true;
    thread_runner = std::thread(TaskRunner, this);
}

void Task::stopTask(bool force)
{
    running = false;
    if(force)
    {
        mutex_msgs.lock();
        while(msgs.empty() == false)
        {
            msgs.pop();
        }
        mutex_msgs.unlock();
    }
    condition.notifyOne();
    if(thread_runner.joinable())
    {
        thread_runner.join();
    }
}

void Task::TaskRunner(Task* task)
{
    if(task == NULL)
    {
        return;
    }
    task->onStart();
    while(task->running)
    {
        do
        {
            task->mutex_msgs.lock();
            if(task->msgs.empty())
            {
                task->mutex_msgs.unlock();
                break;
            }
            Message msg = std::move(task->msgs.front());
            task->msgs.pop();
            task->mutex_msgs.unlock();
            task->onMessage(msg);//可能存在派生类的onMessage已经被析构，则此处将调用的是基类的onMessage，若基类的onMessage为纯虚函数则会出错
        }
        while(true);
        if(task->running == false)
        {
            break;
        }
        task->condition.wait(1000);
    }
    task->onStop();
}

void Task::onMessage(Message& msg)
{
}

void Task::onStart()
{
}

void Task::onStop()
{
}

void Task::pushTaskMessage(const Message& msg)
{
    mutex_msgs.lock();
    msgs.push(msg);
    mutex_msgs.unlock();
    condition.notifyOne();
}

void Task::setProtectMode(bool enable)
{
    this->protect_mode = enable;
}

bool Task::isListenerExist(uint32 id)
{
    if(protect_mode == false)
    {
        return true;
    }
    mutex_listeners.lock();
    bool exist = listeners.count(id) > 0;
    mutex_listeners.unlock();
    return exist;
}

std::string Task::getName()
{
    return name;
}

TaskManager::TaskManager()
{
}

TaskManager::~TaskManager()
{
    LOG_D("called");
    destroyTaskAll();
}

bool TaskManager::isTaskExist(const char* task_name)
{
    if(task_name == NULL)
    {
        return false;
    }
    mutex_tasks.lock();
    bool exist = tasks.count(task_name) > 0;
    mutex_tasks.unlock();
    return exist;
}

bool TaskManager::isTaskExist(const std::string& task_name)
{
    return isTaskExist(task_name.c_str());
}

void TaskManager::destroyTask(const char* task_name_wildcard, bool force)
{
    if(task_name_wildcard == NULL)
    {
        return;
    }
    mutex_tasks.lock();
    std::map<std::string, Task*>::iterator it;
    for(it = tasks.begin(); it != tasks.end();)
    {
        Task* t = it->second;
        if(t == NULL)
        {
            it = tasks.erase(it);
            continue;
        }
        if(com_string_match(t->getName().c_str(), task_name_wildcard) == false)
        {
            it++;
            continue;
        }
        t->stopTask(force);
        delete t;
        it = tasks.erase(it);
    }
    mutex_tasks.unlock();
}

void TaskManager::destroyTask(const std::string& task_name_wildcard, bool force)
{
    destroyTask(task_name_wildcard.c_str(), force);
}

void TaskManager::destroyTaskAll(bool force)
{
    LOG_D("called");
    mutex_tasks.lock();
    std::map<std::string, Task*>::iterator it;
    for(it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if(t != NULL)
        {
            t->stopTask(force);
            delete t;
        }
    }
    tasks.clear();
    mutex_tasks.unlock();
}

void TaskManager::sendAck(uint64 uuid, const void* data, int data_size)
{
    GetSyncManager().getAdapter("com_task").syncPost(uuid, data, data_size);
}

void TaskManager::sendAck(const Message& msg_received, const void* data, int data_size)
{
    if(msg_received.getBool("need_ack"))
    {
        sendAck(msg_received.getUInt64("uuid"), data, data_size);
    }
}

void TaskManager::sendMessage(const std::string& task_name_wildcard, const Message& msg)
{
    sendMessage(task_name_wildcard.c_str(), msg);
}

void TaskManager::sendMessage(const char* task_name_wildcard, const Message& msg)
{
    if(task_name_wildcard == NULL)
    {
        return;
    }
    bool has_wildcard = false;
    for(int i = 0; i < com_string_len(task_name_wildcard); i++)
    {
        if(task_name_wildcard[i] == '?' || task_name_wildcard[i] == '*')
        {
            has_wildcard = true;
            break;
        }
    }

    mutex_tasks.lock();
    for(auto it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if((has_wildcard && com_string_match(t->getName().c_str(), task_name_wildcard))
                || t->getName() == task_name_wildcard)
        {
            if(t->isListenerExist(msg.getID()))
            {
                t->pushTaskMessage(msg);
            }
            if(has_wildcard == false)
            {
                break;
            }
        }
    }
    mutex_tasks.unlock();
    return;
}

ComBytes TaskManager::sendMessageAndWait(const std::string& task_name, Message& msg, int timeout_ms)
{
    return sendMessageAndWait(task_name.c_str(), msg, timeout_ms);
}

ComBytes TaskManager::sendMessageAndWait(const char* task_name, Message& msg, int timeout_ms)
{
    if(task_name == NULL)
    {
        return ComBytes();
    }

    mutex_tasks.lock();
    bool found = false;
    uint64 uuid = GetSyncManager().getAdapter("com_task").createUUID();
    for(auto it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if(t->getName() == task_name)
        {
            if(t->isListenerExist(msg.getID()))
            {
                found = true;
                msg.set("need_ack", true);
                msg.set("uuid", uuid);
                GetSyncManager().getAdapter("com_task").syncPrepare(uuid);
                t->pushTaskMessage(msg);
            }
            break;
        }
    }
    mutex_tasks.unlock();
    if(found == false)
    {
        return ComBytes();
    }
    return GetSyncManager().getAdapter("com_task").syncWait(uuid, timeout_ms);
}

void TaskManager::sendBroadcastMessage(const Message& msg)
{
    mutex_tasks.lock();
    for(auto it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if(t->isListenerExist(msg.getID()))
        {
            t->pushTaskMessage(msg);
        }
    }
    mutex_tasks.unlock();
}

