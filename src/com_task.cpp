#include "com_task.h"
#include "com_sync.h"
#include "com_log.h"

ComWorker::ComWorker(std::string name)
{
    this->name = name;
}

ComWorker::~ComWorker()
{
}

std::string ComWorker::getName()
{
    return name;
}

void ComWorker::stopWorker()
{
    running = false;
    if(thread_runner.joinable())
    {
        thread_runner.join();
    }
}

ComWorkerManager::ComWorkerManager()
{
}

ComWorkerManager::~ComWorkerManager()
{
    LOG_D("called");
    destroyWorkerAll();
}

bool ComWorkerManager::isWorkerExist(const char* task_name)
{
    if(task_name == NULL)
    {
        return false;
    }
    mutex_workers.lock();
    bool exist = workers.count(task_name) > 0;
    mutex_workers.unlock();
    return exist;
}

bool ComWorkerManager::isWorkerExist(const std::string& task_name)
{
    return isWorkerExist(task_name.c_str());
}

void ComWorkerManager::destroyWorker(const char* worker_name_wildcard)
{
    if(worker_name_wildcard == NULL)
    {
        return;
    }
    mutex_workers.lock();
    std::map<std::string, ComWorker*>::iterator it;
    for(it = workers.begin(); it != workers.end();)
    {
        ComWorker* t = it->second;
        if(t == NULL)
        {
            it = workers.erase(it);
            continue;
        }
        if(com_string_match(t->getName().c_str(), worker_name_wildcard) == false)
        {
            it++;
            continue;
        }
        t->stopWorker();
        delete t;
        it = workers.erase(it);
    }
    mutex_workers.unlock();
}

void ComWorkerManager::destroyWorker(const std::string& task_name_wildcard)
{
    destroyWorker(task_name_wildcard.c_str());
}

void ComWorkerManager::destroyWorkerAll()
{
    LOG_D("called");
    mutex_workers.lock();
    std::map<std::string, ComWorker*>::iterator it;
    for(it = workers.begin(); it != workers.end(); it++)
    {
        ComWorker* t = it->second;
        if(t != NULL)
        {
            t->stopWorker();
            delete t;
        }
    }
    workers.clear();
    mutex_workers.unlock();
}

bool ComWorkerManager::createWorker(const char* worker_name, std::function<void(Message msg, std::atomic<bool>& running)> runner, Message msg)
{
    if(worker_name == NULL || worker_name[0] == '\0' || runner == NULL)
    {
        return false;
    }
    mutex_workers.lock();
    size_t count = workers.count(worker_name);
    if(count > 0)
    {
        mutex_workers.unlock();
        LOG_W("worker_name %s already exist", worker_name);
        return false;
    }
    ComWorker* worker = new ComWorker(worker_name);
    worker->running = true;
    worker->thread_runner = std::thread(runner, msg, std::ref(worker->running));
    workers[worker_name] = worker;
    mutex_workers.unlock();
    return true;
};

bool ComWorkerManager::createWorker(const std::string& worker_name, std::function<void(Message msg, std::atomic<bool>& running)> runner, Message msg)
{
    return createWorker(worker_name.c_str(), runner, msg);
};

ComTask::ComTask(std::string name, Message msg)
{
    this->name = name;
    running = false;
}

ComTask::~ComTask()
{
}

void ComTask::startTask()
{
    running = true;
    thread_runner = std::thread(TaskRunner, this);
}

void ComTask::stopTask(bool force)
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

void ComTask::TaskRunner(ComTask* task)
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

void ComTask::onMessage(Message& msg)
{
}

void ComTask::onStart()
{
}

void ComTask::onStop()
{
}

void ComTask::pushTaskMessage(const Message& msg)
{
    mutex_msgs.lock();
    msgs.push(msg);
    mutex_msgs.unlock();
    condition.notifyOne();
}

void ComTask::setProtectMode(bool enable)
{
    this->protect_mode = enable;
}

bool ComTask::isListenerExist(uint32 id)
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

std::string ComTask::getName()
{
    return name;
}

ComTaskManager::ComTaskManager()
{
}

ComTaskManager::~ComTaskManager()
{
    LOG_D("called");
    destroyTaskAll();
}

bool ComTaskManager::isTaskExist(const char* task_name)
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

bool ComTaskManager::isTaskExist(const std::string& task_name)
{
    return isTaskExist(task_name.c_str());
}

void ComTaskManager::destroyTask(const char* task_name_wildcard, bool force)
{
    if(task_name_wildcard == NULL)
    {
        return;
    }
    mutex_tasks.lock();
    std::map<std::string, ComTask*>::iterator it;
    for(it = tasks.begin(); it != tasks.end();)
    {
        ComTask* t = it->second;
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

void ComTaskManager::destroyTask(const std::string& task_name_wildcard, bool force)
{
    destroyTask(task_name_wildcard.c_str(), force);
}

void ComTaskManager::destroyTaskAll(bool force)
{
    LOG_D("called");
    mutex_tasks.lock();
    std::map<std::string, ComTask*>::iterator it;
    for(it = tasks.begin(); it != tasks.end(); it++)
    {
        ComTask* t = it->second;
        if(t != NULL)
        {
            t->stopTask(force);
            delete t;
        }
    }
    tasks.clear();
    mutex_tasks.unlock();
}

void ComTaskManager::sendAck(uint64 uuid, const void* data, int data_size)
{
    GetSyncManager().getAdapter("com_task").syncPost(uuid, data, data_size);
}

void ComTaskManager::sendAck(const Message& msg_received, const void* data, int data_size)
{
    if(msg_received.getBool("need_ack"))
    {
        sendAck(msg_received.getUInt64("uuid"), data, data_size);
    }
}

void ComTaskManager::sendMessage(const std::string& task_name_wildcard, const Message& msg)
{
    sendMessage(task_name_wildcard.c_str(), msg);
}

void ComTaskManager::sendMessage(const char* task_name_wildcard, const Message& msg)
{
    if(task_name_wildcard == NULL)
    {
        return;
    }
    bool has_wildcard = false;
    for(int i = 0; i < com_string_length(task_name_wildcard); i++)
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
        ComTask* t = it->second;
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

ComBytes ComTaskManager::sendMessageAndWait(const std::string& task_name, Message& msg, int timeout_ms)
{
    return sendMessageAndWait(task_name.c_str(), msg, timeout_ms);
}

ComBytes ComTaskManager::sendMessageAndWait(const char* task_name, Message& msg, int timeout_ms)
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
        ComTask* t = it->second;
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

void ComTaskManager::sendBroadcastMessage(const Message& msg)
{
    mutex_tasks.lock();
    for(auto it = tasks.begin(); it != tasks.end(); it++)
    {
        ComTask* t = it->second;
        if(t->isListenerExist(msg.getID()))
        {
            t->pushTaskMessage(msg);
        }
    }
    mutex_tasks.unlock();
}

ComWorkerManager& GetComWorkerManager()
{
    static ComWorkerManager worker_manager;
    return worker_manager;
}

ComTaskManager& GetComTaskManager()
{
    static ComTaskManager task_manager;
    return task_manager;
}

