#ifndef __COM_AUTO_TEST_H__
#define __COM_AUTO_TEST_H__

#include "com_base.h"
#include "httplib.h"

class AutoTestService
{
public:
    AutoTestService();
    virtual ~AutoTestService();

    template<class T>
    void setLatestInfo(const char* group, const char* key, const T val)
    {
        std::lock_guard<std::mutex> lck(mutex_latest_info);
        if(group != NULL && key != NULL)
        {
            if(latest_info.count(group) == 0)
            {
                Message msg;
                msg.set(key, val);
                latest_info[group] = msg;
            }
            else
            {
                Message& msg = latest_info[group];
                msg.set(key, val);
            }
        }
    };

    template<class T>
    void setStatisticInfo(const char* group, const char* key, const T val)
    {
        std::lock_guard<std::mutex> lck(mutex_statistic_info);
        if(group != NULL && key != NULL)
        {
            if(statistic_info.count(group) == 0)
            {
                Message msg;
                msg.set(key, val);
                statistic_info[group] = msg;
            }
            else
            {
                Message& msg = statistic_info[group];
                msg.set(key, val);
            }
        }
    };

    bool hasLatestInfo(const char* group, const char* key = NULL);
    bool hasStatisticInfo(const char* group, const char* key = NULL);
    void removeLatestInfo(const char* group = NULL, const char* key = NULL);
    void removeStatisticInfo(const char* group = NULL, const char* key = NULL);
    Message getLatestInfo(const char* name);
    std::string getLatestInfo(const char* group, const char* name);
    Message getStatisticInfo(const char* name);
    std::string getStatisticInfo(const char* group, const char* name);
    httplib::Server& getHttpServer();
    bool isHttpServerRunning();

    bool startService();
    void stopService();
private:
    void initHttpController();
    static void ThreadController(AutoTestService* ctx);
private:
    std::mutex mutex_latest_info;
    std::mutex mutex_statistic_info;

    std::map<std::string, Message> latest_info;
    std::map<std::string, Message> statistic_info;

    std::atomic<bool> thread_controller_running = {false};
    std::thread thread_controller;

    httplib::Server http_server;
    std::atomic<bool> http_server_running = {false};
};

AutoTestService& GetAutoTestService();

#endif /* __COM_AUTO_TEST_H__ */
