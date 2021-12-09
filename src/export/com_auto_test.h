#ifndef __COM_AUTO_TEST_H__
#define __COM_AUTO_TEST_H__

#include "com_socket.h"

class AutoTestService : public SocketTcpServer
{
public:
    AutoTestService();
    virtual ~AutoTestService();

    template<class T>
    void setLatestInfo(const char* group, const char* key, const T val)
    {
        std::lock_guard<std::mutex> lck(mutex_statistic_info);
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

    Message getLatestInfo(const char* name);
    std::string getLatestInfo(const char* group, const char* name);
    Message getStatisticInfo(const char* name);
    std::string getStatisticInfo(const char* group, const char* name);

    bool startService();
    void stopService();
private:
    static void ThreadController(AutoTestService* ctx);
    virtual void onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected);
    virtual void onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size);
private:
    std::mutex mutex_latest_info;
    std::mutex mutex_statistic_info;

    std::map<std::string, Message> latest_info;
    std::map<std::string, Message> statistic_info;

    std::atomic<bool> thread_controller_running = {false};
    std::thread thread_controller;

    std::string server_name;
};

AutoTestService& GetAutoTestService();
void InitAutoTestService();
void UninitAutoTestService();

#endif /* __COM_AUTO_TEST_H__ */
