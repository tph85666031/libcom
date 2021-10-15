#include "com_task.h"
#include "com_log.h"

TaskManager& GetTaskManager()
{
    static TaskManager task_manager;
    return task_manager;
}

void InitTaskManager()
{
    GetTaskManager();
}

void UninitTaskManager()
{
    GetTaskManager().destroyTaskAll();
}

Task::Task(std::string name, Message init_msg)
{
    this->name = name;
    running = false;
}

Task::~Task()
{
    stopTask();
}

void Task::startTask()
{
    running = true;
    thread_runner = std::thread(taskRunner, this);
}

void Task::stopTask()
{
    running = false;
    condition.notifyOne();
    if (thread_runner.joinable())
    {
        thread_runner.join();
    }
}

void Task::taskRunner(Task* task)
{
    if (task == NULL)
    {
        return;
    }
    task->onStart();
    while (task->running)
    {
        do
        {
            task->mutex_msgs.lock();
            if (task->msgs.empty())
            {
                task->mutex_msgs.unlock();
                break;
            }
            Message msg = task->msgs.front();
            task->msgs.pop();
            task->mutex_msgs.unlock();
            task->onMessage(msg);//可能存在派生类的onMessage已经被析构，则此处将调用的是基类的onMessage，若基类的onMessage为纯虚函数则会出错
        }
        while (true);
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

void Task::pushMessage(Message& msg)
{
    mutex_msgs.lock();
    msgs.push(msg);
    mutex_msgs.unlock();
    condition.notifyOne();
}

bool Task::isListenerExist(uint32_t id)
{
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
    destroyTaskAll();
}

bool TaskManager::isTaskExist(std::string task_name)
{
    mutex_tasks.lock();
    bool exist = tasks.count(task_name) > 0;
    mutex_tasks.unlock();
    return exist;
}

void TaskManager::destroyTask(std::string task_name)
{
    mutex_tasks.lock();
    std::map<std::string, Task*>::iterator it;
    for (it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if (t != NULL && t->getName() == task_name)
        {
            t->stopTask();
            delete t;
            tasks.erase(it);
            break;
        }
    }
    mutex_tasks.unlock();
}

void TaskManager::destroyTaskAll()
{
    mutex_tasks.lock();
    std::map<std::string, Task*>::iterator it;
    for (it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if (t != NULL)
        {
            t->stopTask();
            delete t;
        }
    }
    tasks.clear();
    mutex_tasks.unlock();
}

void TaskManager::sendMessage(std::string task_name_wildcard, Message& msg)
{
    bool has_wildcard = false;
    for (size_t i = 0; i < task_name_wildcard.size(); i++)
    {
        if (task_name_wildcard[i] == '?' || task_name_wildcard[i] == '*')
        {
            has_wildcard = true;
            break;
        }
    }

    mutex_tasks.lock();
    for (auto it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if ((has_wildcard && com_string_match(t->getName().c_str(), task_name_wildcard.c_str()))
                || com_string_equal(t->getName().c_str(), task_name_wildcard.c_str()))
        {
            if (t->isListenerExist(msg.getID()))
            {
                t->pushMessage(msg);
            }
            if (has_wildcard == false)
            {
                break;
            }
        }
    }
    mutex_tasks.unlock();
    return;
}

void TaskManager::sendBroadcastMessage(Message& msg)
{
    mutex_tasks.lock();
    for (auto it = tasks.begin(); it != tasks.end(); it++)
    {
        Task* t = it->second;
        if (t->isListenerExist(msg.getID()))
        {
            t->pushMessage(msg);
        }
    }
    mutex_tasks.unlock();
}

