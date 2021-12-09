#include "com_auto_test.h"
#include "com_log.h"
#include "com_file.h"
#include "com_config.h"

#define FILE_AUTO_TEST_CONFIG "/tmp/at.cfg"

AutoTestService& GetAutoTestService()
{
    static AutoTestService auto_test_service;
    return auto_test_service;
}

void InitAutoTestService()
{
    GetAutoTestService().startService();
}

void UninitAutoTestService()
{
    GetAutoTestService().stopService();
}

AutoTestService::AutoTestService()
{
}

AutoTestService::~AutoTestService()
{
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
    stopService();
    thread_controller_running = true;
    thread_controller = std::thread(ThreadController, this);
    return true;
}

void AutoTestService::stopService()
{
    thread_controller_running = false;
    if(thread_controller.joinable())
    {
        thread_controller.join();
    }
    stopServer();
}

void AutoTestService::onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected)
{
    LOG_I("connection from %s:%u fd=%d is %s", host.c_str(), port, socketfd, connected ? "connected" : "disconnected");
}

void AutoTestService::onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size)
{
    LOG_I("got data from %s:%u", host.c_str(), port);
    std::string value((char*)data, data_size);
    std::vector<std::string> items = com_string_split(value.c_str(), " ");
    if(items.size() < 2)
    {
        LOG_E("command size incorrect");
        return;
    }
    if(items[0] != "show" || items[1] != com_get_bin_name())
    {
        LOG_E("command synax incorrect");
        return;
    }

    std::string result;
    if(items.size() == 2)
    {
        mutex_latest_info.lock();
        for(auto it = latest_info.begin(); it != latest_info.end(); it++)
        {
            Message& msg = it->second;
            result += msg.toJSON();
        }
        mutex_latest_info.unlock();
    }
    else if(items.size() == 3)
    {
        Message msg = getLatestInfo(items[2].c_str());
        result = msg.toJSON();
    }
    else
    {
        result = getLatestInfo(items[2].c_str(), items[3].c_str());
    }
    LOG_I("send back result:%s", result.c_str());
    send(socketfd, result.c_str(), result.size() + 1);
}

void AutoTestService::ThreadController(AutoTestService* ctx)
{
    CPPConfig config;
    std::string app_name = com_get_bin_name();
    while(ctx->thread_controller_running)
    {
        if(com_file_type(FILE_AUTO_TEST_CONFIG) == FILE_TYPE_FILE)
        {
            config.load(FILE_AUTO_TEST_CONFIG);
            if(config.isKeyExist(app_name.c_str(), "port"))
            {
                break;
            }
        }
        com_sleep_s(10);
    }

    ctx->setPort(config.getUInt16(app_name.c_str(), "port"));
    LOG_I("start server,port=%u", config.getUInt16(app_name.c_str(), "port"));
    ctx->startServer();
    return;
}

