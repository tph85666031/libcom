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
    this->addConfigFile(FILE_AUTO_TEST_CONFIG);
    initHttpController();
    startService();
}

AutoTestService::~AutoTestService()
{
    LOG_I("called");
    stopService();
}

void AutoTestService::addConfigFile(const char* file)
{
    if(file == NULL)
    {
        return;
    }
    std::lock_guard<std::mutex> lck(mutex_config_file_list);
    config_file_list.push_back(file);
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

void AutoTestService::removeLatestInfo(const char* group, const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_latest_info);
    if(group == NULL && key == NULL)
    {
        latest_info.clear();
    }
    else if(group != NULL && key == NULL)
    {
        for(auto it = latest_info.begin(); it != latest_info.end();)
        {
            if(com_string_match(it->first.c_str(), group))
            {
                it = latest_info.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
    else if(group != NULL && key != NULL)
    {
        Message& msg = latest_info[group];
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
        return std::string();
    }
    Message& msg = latest_info[group];
    return msg.getString(key);
}

int64 AutoTestService::getLatestInfoAsNumber(const char* group, const char* key, int64 default_val)
{
    std::string str = getLatestInfo(group, key);
    if(str.empty())
    {
        return default_val;
    }
    return strtoll(str.c_str(), NULL, 10);
}

double AutoTestService::getLatestInfoAsDouble(const char* group, const char* key, double default_val)
{
    std::string str = getLatestInfo(group, key);
    if(str.empty())
    {
        return default_val;
    }
    return strtod(str.c_str(), NULL);
}

bool AutoTestService::getLatestInfoAsBool(const char* group, const char* key, bool default_val)
{
    std::string str = getLatestInfo(group, key);
    if(str.empty())
    {
        return default_val;
    }
    com_string_to_lower(str);
    if(str == "true")
    {
        return true;
    }
    else if(str == "false")
    {
        return false;
    }
    else
    {
        return (strtol(str.c_str(), NULL, 10) == 1);
    }
}

CPPBytes AutoTestService::getLatestInfoAsBytes(const char* group, const char* key)
{
    std::lock_guard<std::mutex> lck(mutex_latest_info);
    if(group == NULL || key == NULL || latest_info.count(group) <= 0)
    {
        return CPPBytes();
    }
    Message& msg = latest_info[group];
    return msg.getBytes(key);
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
    sem.post();
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
                response.status = 500;
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
                response.status = 500;
            }
        }
        else
        {
            mutex_latest_info.lock();
            result += "{\n";
            for(auto it = latest_info.begin(); it != latest_info.end(); it++)
            {
                Message& msg = it->second;
                result += "\"" + it->first + "\":\n" + msg.toJSON(true) + ",\n";
            }
            result.pop_back();
            result.pop_back();
            result += "\n}";
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
            response.status = 500;
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
        std::vector<std::string> config_files;
        ctx->mutex_config_file_list.lock();
        config_files = ctx->config_file_list;
        ctx->mutex_config_file_list.unlock();

        for(const std::string& path : config_files)
        {
            if(com_file_type(path.c_str()) != FILE_TYPE_FILE)
            {
                continue;
            }
            config.load(path.c_str());
            if(config.isKeyExist(app_name.c_str(), "port"))
            {
                uint16 port = config.getUInt16(app_name.c_str(), "port");
                if(port == 0)
                {
                    port = (uint16)(com_process_get_pid() % 64536);
                    port += 1000;
                }

                LOG_I("start service for %s, path=http://127.0.0.1:%u", app_name.c_str(), port);
                ctx->http_server_running = true;
                ctx->http_server.listen("0.0.0.0", port);
                ctx->http_server_running = false;
            }
        }
        ctx->sem.wait(10 * 1000);
    }
    return;
}
