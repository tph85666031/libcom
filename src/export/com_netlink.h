#ifndef __COM_NETLINK_H__
#define __COM_NETLINK_H__

#include "com_base.h"

class COM_EXPORT ComNetLink
{
public:
    ComNetLink();
    virtual ~ComNetLink();
    int openLink(int local_id, int group, int protocol_type);
    void closeLink();
    /* will ALSO send to group if group not 0 */
    int sendMessage(int remote_id, int group, const void* data, int data_size);
private:
    static void ThreadRx(ComNetLink* ctx);
    virtual void onMessage(int sender_id, const uint8* data, int data_size);

private:
    int sd = 0;
    int buf_size = 8192;
    int local_id;
    int group;
    std::atomic<bool> thread_rx_running = {false};
    std::thread thread_rx;
};

class COM_EXPORT ComNetLinkGeneric
{
public:
    ComNetLinkGeneric();
    ComNetLinkGeneric(const char* name);
    virtual ~ComNetLinkGeneric();
    int openLink(const char* name, int id = 0);
    void closeLink();
    bool sendMessage(const void* data, int data_size);
    bool sendMessage(int remote_id, const void* data, int data_size);
private:
    int getFamilyID();
    bool sendMessage(int remote_id, int type, int cmd, int data_type, const void* data, int data_size);
    ComBytes recvMessage(int timeout_ms);
    static void ThreadRx(ComNetLinkGeneric* ctx);
    virtual void onMessage(int sender_id, const uint8* data, int data_size);

private:
    int sock = 0;
    int id = 0;
    uint8* buf;
    int buf_size = 8192;
    int family_id;
    std::string name;
    std::atomic<bool> thread_rx_running = {false};
    std::thread thread_rx;
};

#endif /* __COM_NETLINK_H__ */

