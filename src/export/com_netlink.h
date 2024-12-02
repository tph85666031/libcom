#ifndef __COM_NETLINK_H__
#define __COM_NETLINK_H__

#include "com_base.h"

class ComNetLink
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


#endif /* __COM_NETLINK_H__ */

