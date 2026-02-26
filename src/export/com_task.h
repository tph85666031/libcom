#ifndef __COM_TASK_H__
#define __COM_TASK_H__

#include <queue>
#include <map>
#include <atomic>
#include "com_base.h"
#include "com_log.h"
#include "com_thread.h"
#include <iostream>

class COM_EXPORT Task
{
    friend class TaskManager;
public:
    Task(std::string name, Message msg = Message());
    virtual ~Task();
protected:
    std::string getName();
    void setProtectMode(bool enable);
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
            if(listeners.count(id) > 0)
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
    void pushTaskMessage(const Message& msg);
protected:
    std::atomic<bool> running;
private:
    void startTask();
    void stopTask(bool force = false);
    static void TaskRunner(Task* task);
private:
    std::string name;
    std::thread thread_runner;
    std::mutex mutex_msgs;
    std::queue<Message> msgs;
    std::mutex mutex_listeners;
    std::map<uint32, uint32> listeners;
    ComCondition condition = {"TASK:condition"};
    std::atomic<bool> protect_mode = {false};
};

class COM_EXPORT TaskManager
{
public:
    TaskManager();
    virtual ~TaskManager();
    bool isTaskExist(const char* task_name);
    bool isTaskExist(const std::string& task_name);
    void destroyTask(const char* task_name_wildcard, bool force = false);
    void destroyTask(const std::string& task_name_wildcard, bool force = false);
    void destroyTaskAll(bool force = false);
    ComBytes sendMessageAndWait(const char* task_name, Message& msg, int timeout_ms = 1000);
    ComBytes sendMessageAndWait(const std::string& task_name, Message& msg, int timeout_ms = 1000);
    void sendMessage(const char* task_name_wildcard, const Message& msg);
    void sendMessage(const std::string& task_name_wildcard, const Message& msg);
    void sendBroadcastMessage(const Message& msg);
    void sendAck(uint64 uuid, const void* data, int data_size);
    void sendAck(const Message& msg_received, const void* data, int data_size);
    template<class T>
    T* createTask(const char* task_name, Message msg = Message())
    {
        if(task_name == NULL || task_name[0] == '\0')
        {
            return NULL;
        }
        mutex_tasks.lock();
        size_t count = tasks.count(task_name);
        if(count > 0)
        {
            mutex_tasks.unlock();
            LOG_W("task %s already exist", task_name);
            return NULL;
        }
        T* task = new T(task_name, msg);
        task->startTask();
        tasks[task_name] = task;
        mutex_tasks.unlock();
        return task;
    };
    template<class T>
    T* createTask(const std::string& task_name, Message msg = Message())
    {
        if(task_name.empty())
        {
            return NULL;
        }
        mutex_tasks.lock();
        size_t count = tasks.count(task_name);
        if(count > 0)
        {
            mutex_tasks.unlock();
            LOG_W("task %s already exist", task_name.c_str());
            return NULL;
        }
        T* task = new T(task_name, msg);
        task->startTask();
        tasks[task_name] = task;
        mutex_tasks.unlock();
        return task;
    };
private:
    std::mutex mutex_tasks;
    std::map<std::string, Task*> tasks;
};

COM_EXPORT TaskManager& GetTaskManager();

#endif /* __COM_TASK_H__ */

