#ifndef __COM_AUTO_TEST_H__
#define __COM_AUTO_TEST_H__

#include "com_base.h"
#include "httplib.h"

class COM_EXPORT AutoTestService
{
public:
    AutoTestService();
    virtual ~AutoTestService();

    template<class T>
    void setLatestInfo(const char* group, const char* key, const T val)
    {
#if 0
        if(http_server_running == false)
        {
            return;
        }
#endif
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

    bool hasLatestInfo(const char* group, const char* key = NULL);
    void removeLatestInfo(const char* group = NULL, const char* key = NULL);

    Message getLatestInfo(const char* group);
    std::string getLatestInfo(const char* group, const char* key);
    int64 getLatestInfoAsNumber(const char* group, const char* key, int64 default_val = 0);
    double getLatestInfoAsDouble(const char* group, const char* key, double default_val = 0);
    bool getLatestInfoAsBool(const char* group, const char* key, bool default_val = false);
    ComBytes getLatestInfoAsBytes(const char* group, const char* key);

    httplib::Server& getHttpServer();
    bool isHttpServerRunning();
    void addConfigFile(const char* file);
    bool startService();
    void stopService();
private:
    void initHttpController();
    static void ThreadController(AutoTestService* ctx);
private:
    std::mutex mutex_config_file_list;
    std::vector<std::string> config_file_list;

    std::mutex mutex_latest_info;
    std::map<std::string, Message> latest_info;

    std::atomic<bool> thread_controller_running = {false};
    std::thread thread_controller;
    ComSem sem;

    httplib::Server http_server;
    std::atomic<bool> http_server_running = {false};
};

COM_EXPORT AutoTestService& GetAutoTestService();

#endif /* __COM_AUTO_TEST_H__ */

