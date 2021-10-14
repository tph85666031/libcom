#ifndef __COM_TASK_H__
#define __COM_TASK_H__

#include <queue>
#include <map>
#include <atomic>
#include "com_base.h"
#include "com_log.h"
#include "com_thread.h"
#include <iostream>

class Task
{
    friend class TaskManager;
public:
    Task(std::string name, Message init_msg = Message());
    virtual ~Task();
    std::string getName();
    bool isListenerExist(uint32 id);
    template <class... T>
    void addListener(T... ids)
    {
        mutex_listeners.lock();
        // *INDENT-OFF*
        for(uint32 id : {ids...})
        // *INDENT-ON*
        {
            listeners[id] = id;
        }
        mutex_listeners.unlock();
    }
    template <class... T>
    void removeListener(T... ids)
    {
        mutex_listeners.lock();
        // *INDENT-OFF*
        for(uint32 id : {ids...})
        // *INDENT-ON*
        {
            if (listeners.count(id) > 0)
            {
                listeners.erase(id);
            }
        }
        mutex_listeners.unlock();
    }
protected:
    virtual void onStart();
    virtual void onStop();
    virtual void onMessage(Message& msg);
private:
    void pushMessage(Message& msg);
    void startTask();
    void stopTask();
    static void taskRunner(Task* task);
private:
    std::string name;
    std::thread thread_runner;
    CPPMutex mutex_msgs = {"TASK:mutex_msgs"};
    std::queue<Message> msgs;
    std::atomic<bool> running;
    CPPMutex mutex_listeners = {"TASK:mutex_listeners"};
    std::map<uint32, uint32> listeners;
    CPPCondition condition = {"TASK:condition"};
};

class TaskManager
{
public:
    TaskManager();
    virtual ~TaskManager();
    bool isTaskExist(std::string task_name);
    void destroyTask(std::string task_name);
    void destroyTaskAll();
    void sendMessage(std::string task_name_wildcard, Message& msg);
    void sendBroadcastMessage(Message& msg);
    template<class T>
    T* createTask(std::string task_name, Message init_msg = Message())
    {
        mutex_tasks.lock();
        int count = tasks.count(task_name);
        if (count > 0)
        {
            mutex_tasks.unlock();
            LOG_W("task %s already exist", task_name.c_str());
            return NULL;
        }
        T* task = new T(task_name, init_msg);
        task->startTask();
        tasks[task_name] = task;
        mutex_tasks.unlock();
        return task;
    };
private:
    CPPMutex mutex_tasks = {"TaskManager:mutex_tasks"};
    std::map<std::string, Task*> tasks;
};

TaskManager& GetTaskManager();
void InitTaskManager();
void UninitTaskManager();

inline bool com_task_exist(std::string task_name)
{
    return GetTaskManager().isTaskExist(task_name);
}

inline void com_task_destroy(std::string task_name)
{
    GetTaskManager().destroyTask(task_name);
}

inline void com_task_send_message(std::string task_name_wildcard, Message& msg)
{
    GetTaskManager().sendMessage(task_name_wildcard, msg);
}

inline void com_task_send_broadcast_message(Message& msg)
{
    GetTaskManager().sendBroadcastMessage(msg);
}

template<class T>
T* com_task_create(std::string task_name, Message init_msg = Message())
{
    return GetTaskManager().createTask<T>(task_name, init_msg);
}

#endif /* __COM_TASK_H__ */

