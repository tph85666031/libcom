#include "com_log.h"
#include "com_file.h"
#include "com_config.h"
#include "com_auto_test.h"

#define FILE_AUTO_TEST_CONFIG "/tmp/at.cfg"

using namespace httplib;

AutoTestService& GetAutoTestService()
{
    static AutoTestService auto_test_service;
    return auto_test_service;
}

AutoTestService::AutoTestService()
{
    initHttpServer();
    startService();
}

AutoTestService::~AutoTestService()
{
    LOG_I("called");
    stopService();
}

Message AutoTestService::getLatestInfo(const char* group)
{
    std::lock_guard<std::mutex> lck(mutex_latest_info);
    if(group != NULL && latest_info.count(group) > 0)
    {
        return latest_info[group];
    }
    else
    {
        return Message();
    }
}

std::string AutoTestService::getLatestInfo(const char* group, const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_latest_info);
    if(group == NULL || key == NULL || latest_info.count(group) <= 0)
    {
        LOG_W("%s:%s not found", group, key);
        return std::string();
    }
    Message& msg = latest_info[group];
    return msg.getString(key);
}

Message AutoTestService::getStatisticInfo(const char* name)
{
    std::lock_guard<std::mutex> lck(mutex_statistic_info);
    if(name != NULL && statistic_info.count(name) > 0)
    {
        return statistic_info[name];
    }
    else
    {
        return Message();
    }
}

std::string AutoTestService::getStatisticInfo(const char* group, const char* name)
{
    std::lock_guard<std::mutex> lck(mutex_statistic_info);
    if(group == NULL || name == NULL || latest_info.count(group) > 0)
    {
        return std::string();
    }
    Message& msg = statistic_info[group];
    return msg.getString(name);
}

bool AutoTestService::startService()
{
    LOG_I("called");
    stopService();
    thread_controller_running = true;
    thread_controller = std::thread(ThreadController, this);
    return true;
}

void AutoTestService::stopService()
{
    LOG_I("called");
    http_server.stop();
    thread_controller_running = false;
    if(thread_controller.joinable())
    {
        thread_controller.join();
    }
}

void AutoTestService::initHttpServer()
{
    std::string base_path;
    http_server.Get(base_path + "/show", [this](const Request & request, Response & response)
    {
        std::string result;
        if(request.has_param("group") && request.has_param("key"))
        {
            std::string group = request.get_param_value("group");
            std::string key = request.get_param_value("key");
            result = getLatestInfo(group.c_str(), key.c_str());
        }
        else if(request.has_param("group"))
        {
            std::string group = request.get_param_value("group");
            Message msg = getLatestInfo(group.c_str());
            result = msg.toJSON();
        }
        else
        {
            mutex_latest_info.lock();
            for(auto it = latest_info.begin(); it != latest_info.end(); it++)
            {
                Message& msg = it->second;
                result += it->first + ":\n" + msg.toJSON() + "\n";
            }
            mutex_latest_info.unlock();
        }
        response.set_content(result, "text/plain");
    });

    http_server.Post(base_path + "/stop",[this](const Request & request, Response & response)
    {
        this->http_server.stop();
        LOG_I("http_server stopped");
    });
}

void AutoTestService::ThreadController(AutoTestService* ctx)
{
    CPPConfig config;
    std::string app_name = com_get_bin_name();
    while(ctx->thread_controller_running)
    {
        com_sleep_s(10);
        if(com_file_type(FILE_AUTO_TEST_CONFIG) != FILE_TYPE_FILE)
        {
            continue;
        }

        config.load(FILE_AUTO_TEST_CONFIG);
        if(config.isKeyExist(app_name.c_str(), "port") == false)
        {
            continue;
        }
        uint16 port = config.getUInt16(app_name.c_str(), "port");

        LOG_I("start service for %s, port=%u", app_name.c_str(), port);
        ctx->http_server.listen("0.0.0.0", port);
    }
    return;
}

