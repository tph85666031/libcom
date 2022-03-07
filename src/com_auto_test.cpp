#include "com_log.h"
#include "com_file.h"
#include "com_config.h"
#include "com_thread.h"
#include "com_auto_test.h"

#if defined(_WIN32) || defined(_WIN64)
#define FILE_AUTO_TEST_CONFIG "C:\\Windows\\Temp\\at.cfg"
#else
#define FILE_AUTO_TEST_CONFIG "/tmp/at.cfg"
#endif

using namespace httplib;

AutoTestService& GetAutoTestService()
{
    static AutoTestService auto_test_service;
    return auto_test_service;
}

AutoTestService::AutoTestService()
{
    initHttpController();
    startService();
}

AutoTestService::~AutoTestService()
{
    LOG_I("called");
    stopService();
}

bool AutoTestService::hasLatestInfo(const char* group, const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_latest_info);
    if(group == NULL && key == NULL)
    {
        return false;
    }
    else if(group != NULL && key == NULL)
    {
        return (latest_info.count(group) > 0);
    }
    else if(group != NULL && key != NULL)
    {
        if(latest_info.count(group) <= 0)
        {
            return false;
        }
        Message& msg = latest_info[group];
        return msg.isKeyExist(key);
    }
    return false;
}

bool AutoTestService::hasStatisticInfo(const char* group, const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_latest_info);
    if(group == NULL && key == NULL)
    {
        return false;
    }
    else if(group != NULL && key == NULL)
    {
        return (statistic_info.count(group) > 0);
    }
    else if(group != NULL && key != NULL)
    {
        if(statistic_info.count(group) <= 0)
        {
            return false;
        }
        Message& msg = statistic_info[group];
        return msg.isKeyExist(key);
    }
    return false;
}

void AutoTestService::removeLatestInfo(const char* group, const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_latest_info);
    if(group == NULL && key == NULL)
    {
        latest_info.clear();
    }
    else if(group != NULL && key == NULL)
    {
        latest_info.erase(group);
    }
    else if(group != NULL && key != NULL)
    {
        Message& msg = latest_info[group];
        msg.remove(key);
    }
    return;
}

void AutoTestService::removeStatisticInfo(const char* group, const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_statistic_info);
    if(group == NULL && key == NULL)
    {
        statistic_info.clear();
    }
    else if(group != NULL && key == NULL)
    {
        statistic_info.erase(group);
    }
    else if(group != NULL && key != NULL)
    {
        Message& msg = statistic_info[group];
        msg.remove(key);
    }
    return;
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

Server& AutoTestService::getHttpServer()
{
    return http_server;
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

bool AutoTestService::isHttpServerRunning()
{
    return http_server_running;
}

void AutoTestService::initHttpController()
{
    std::string base_path;
    http_server.Get(base_path + "/show", [this](const Request & request, Response & response)
    {
        std::string result;
        if(request.has_param("group") && request.has_param("key"))
        {
            std::string group = request.get_param_value("group");
            std::string key = request.get_param_value("key");
            if(hasLatestInfo(group.c_str(), key.c_str()))
            {
                result = getLatestInfo(group.c_str(), key.c_str());
            }
            else
            {
                response.status = 400;
            }
        }
        else if(request.has_param("group"))
        {
            std::string group = request.get_param_value("group");
            Message msg = getLatestInfo(group.c_str());
            if(hasLatestInfo(group.c_str()))
            {
                result = msg.toJSON(true);
            }
            else
            {
                response.status = 400;
            }
        }
        else
        {
            mutex_latest_info.lock();
            for(auto it = latest_info.begin(); it != latest_info.end(); it++)
            {
                Message& msg = it->second;
                result += it->first + ":\n" + msg.toJSON(true) + "\n\n";
            }
            mutex_latest_info.unlock();
        }
        response.set_content(result, "text/plain;charset=UTF-8");
    });

    http_server.Post(base_path + "/set", [this](const Request & request, Response & response)
    {
        if(request.has_param("group") && request.has_param("key") && request.has_param("value"))
        {
            std::string group = request.get_param_value("group");
            std::string key = request.get_param_value("key");
            std::string value = request.get_param_value("value");
            setLatestInfo(group.c_str(), key.c_str(), value.c_str());
        }
        else
        {
            response.status = 400;
        }
    });

    http_server.Post(base_path + "/remove", [this](const Request & request, Response & response)
    {
        std::string result;
        if(request.has_param("group") && request.has_param("key"))
        {
            std::string group = request.get_param_value("group");
            std::string key = request.get_param_value("key");
            removeLatestInfo(group.c_str(), key.c_str());
        }
        else if(request.has_param("group"))
        {
            std::string group = request.get_param_value("group");
            removeLatestInfo(group.c_str());
        }
        else
        {
            removeLatestInfo();
        }
    });

    http_server.Post(base_path + "/stop", [this](const Request & request, Response & response)
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
        if(com_file_type(FILE_AUTO_TEST_CONFIG) != FILE_TYPE_FILE)
        {
            com_sleep_s(10);
            continue;
        }

        config.load(FILE_AUTO_TEST_CONFIG);
        if(config.isKeyExist(app_name.c_str(), "port") == false)
        {
            com_sleep_s(10);
            continue;
        }

        uint16 port = config.getUInt16(app_name.c_str(), "port");
        if(port == 0)
        {
            port = (uint16)(com_thread_get_pid() % 64536);
            port += 1000;
        }

        LOG_I("start service for %s, path=http://127.0.0.1:%u", app_name.c_str(), port);
        ctx->http_server_running = true;
        ctx->http_server.listen("0.0.0.0", port);
        ctx->http_server_running = false;
        com_sleep_s(10);
    }
    return;
}

