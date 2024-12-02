#if __linux__==1
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h> //close
#endif
#include "com_log.h"
#include "com_thread.h"
#include "com_netlink.h"

ComNetLink::ComNetLink()
{
}

ComNetLink::~ComNetLink()
{
    closeLink();
}

int ComNetLink::openLink(int local_id, int group, int protocol_type)
{
#if __linux__==1
    sd = socket(AF_NETLINK, SOCK_DGRAM, protocol_type);
    if(sd <= 0)
    {
        LOG_E("failed to open netlink socket,ret=%d", sd);
        return -1;
    }
    this->local_id = local_id;
    this->group = group;

    struct sockaddr_nl addr_local;
    addr_local.nl_family = AF_NETLINK;
    addr_local.nl_groups = group;
    addr_local.nl_pid = local_id;

    int ret = bind(sd, (struct sockaddr*)&addr_local, sizeof(sockaddr_nl));
    if(ret < 0)
    {
        LOG_E("bind failed,ret=%d", ret);
        return -2;
    }
    thread_rx_running = true;
    thread_rx = std::thread(ThreadRx, this);
    return 0;
#else
    return -1;
#endif
}

void ComNetLink::closeLink()
{
#if __linux__==1
    if(sd > 0)
    {
        close(sd);
        sd = 0;
    }
    thread_rx_running = false;
    if(thread_rx.joinable())
    {
        thread_rx.join();
    }
#endif
}

int ComNetLink::sendMessage(int remote_id, int group, const void* data, int data_size)
{
#if __linux__==1
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect,data=%p,data_size=%d", data, data_size);
        return -1;
    }
    int total_size = NLMSG_SPACE(data_size);
    uint8* buf = new uint8[total_size]();
    struct nlmsghdr* nl_header = (struct nlmsghdr*)buf;
    nl_header->nlmsg_len = NLMSG_LENGTH(data_size);
    nl_header->nlmsg_type = NLMSG_DONE;
    nl_header->nlmsg_flags = 0;
    nl_header->nlmsg_pid = local_id;
    nl_header->nlmsg_seq = 0;
    memcpy(NLMSG_DATA(nl_header), data, data_size);

    struct sockaddr_nl addr_remote;
    addr_remote.nl_family = AF_NETLINK;
    addr_remote.nl_groups = group;
    addr_remote.nl_pid = remote_id;

    int size_sent = 0;
    do
    {
        int ret = sendto(sd, buf + size_sent, total_size - size_sent, 0, (struct sockaddr*)&addr_remote, sizeof(addr_remote));
        if(ret <= 0)
        {
            break;
        }
        size_sent += ret;
    }
    while(size_sent < total_size);

    delete[] buf;
    return size_sent;
#else
    return -1;
#endif
}

void ComNetLink::onMessage(int sender_id, const uint8* data, int data_size)
{
}

void ComNetLink::ThreadRx(ComNetLink* ctx)
{
#if __linux__==1
    if(ctx == NULL)
    {
        return;
    }

    uint8* buf = new uint8[ctx->buf_size]();

    int timeout_ms = 1000;
    while(ctx->thread_rx_running)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(ctx->sd, &readfds);
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        int select_rtn = select(ctx->sd + 1, &readfds, NULL, NULL, &tv);
        //如果对端关闭了连接,select的返回值可能为1,且errno为2
        if(select_rtn < 0)
        {
            LOG_W("select() failed,ret=%d,errno=%d:%s", select_rtn, errno, strerror(errno));
            break;
        }
        if(select_rtn == 0)
        {
            LOG_T("select() timeout");
            continue;
        }
        if(FD_ISSET(ctx->sd, &readfds) == 0)
        {
            LOG_D("select() no data");
            continue;
        }

        int size = 0;
        int ret = 0;
        while(ctx->thread_rx_running
                && (ret = recvfrom(ctx->sd, buf + size, ctx->buf_size - size, MSG_DONTWAIT, NULL, NULL)) > 0)
        {
            size += ret;
        }

        if(size >= NLMSG_HDRLEN)
        {
            ctx->onMessage(((struct nlmsghdr*)buf)->nlmsg_pid, (uint8*)NLMSG_DATA((struct nlmsghdr*)buf), ((struct nlmsghdr*)buf)->nlmsg_len - NLMSG_HDRLEN);
        }
    }
    delete[] buf;
#endif
}


