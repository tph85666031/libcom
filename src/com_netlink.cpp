#if __linux__==1
#include <linux/netlink.h>
#include <linux/genetlink.h>
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

ComNetLinkGeneric::ComNetLinkGeneric()
{
    buf = new uint8[buf_size]();
}

ComNetLinkGeneric::~ComNetLinkGeneric()
{
    closeLink();
    delete[] buf;
}

int ComNetLinkGeneric::getFamilyID()
{
#if __linux__==1
    //向内核的GENL_ID_CTRL family查询name对应的familyID
    if(sendMessage(0, GENL_ID_CTRL, CTRL_CMD_GETFAMILY, CTRL_ATTR_FAMILY_NAME, name.c_str(), name.length() + 1) == false)
    {
        LOG_E("failed");
        return -1;
    }
    ComBytes reply = recvMessage(3000);
    struct nlmsghdr* nl_header = (struct nlmsghdr*)reply.getData();
    if(NLMSG_OK(nl_header, reply.getDataSize()) == false)
    {
        LOG_E("reply data check failed,size=%d", reply.getDataSize());
        return -2;
    }

    if(nl_header->nlmsg_type != GENL_ID_CTRL)
    {
        LOG_E("reply data type check failed,type=%d", nl_header->nlmsg_type);
        return -3;
    }

    int nl_size = reply.getDataSize();
    struct nlmsghdr* nl_header_cur = nl_header;
    for(nl_header_cur = nl_header; NLMSG_OK(nl_header_cur, nl_size); nl_header_cur = NLMSG_NEXT(nl_header_cur, nl_size))
    {
        if(nl_header_cur->nlmsg_type == NLMSG_DONE
                || nl_header_cur->nlmsg_type == NLMSG_ERROR)
        {
            break;
        }

        struct genlmsghdr* gnl_header = (struct genlmsghdr*)NLMSG_DATA(nl_header_cur);
        struct nlattr* nla = (struct nlattr*)((char*)gnl_header + GENL_HDRLEN);
        int nla_total_size = nl_header_cur->nlmsg_len - NLMSG_HDRLEN - GENL_HDRLEN;
        while(nla_total_size >= NLA_ALIGN(nla->nla_len))
        {
            if(nla->nla_type == CTRL_ATTR_FAMILY_ID)
            {
                return *((int16*)((char*)nla + NLA_HDRLEN));
            }
            nla_total_size -= NLA_ALIGN(nla->nla_len);
            nla = (struct nlattr*)((char*)nla + NLA_ALIGN(nla->nla_len));
        }
    }

#endif
    return -1;
}

int ComNetLinkGeneric::openLink(const char* name, int id)
{
    if(name == NULL)
    {
        return -1;
    }
    this->name = name;
    this->id = (id <= 0 ? com_process_get_pid() : id);

#if __linux__==1
    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if(sock <= 0)
    {
        LOG_E("failed to open netlink socket,ret=%d", sock);
        return -1;
    }

    struct sockaddr_nl addr_local;
    memset(&addr_local, 0, sizeof(struct sockaddr_nl));
    addr_local.nl_family = AF_NETLINK;
    addr_local.nl_groups = 0;
    addr_local.nl_pid = this->id;

    int ret = bind(sock, (struct sockaddr*)&addr_local, sizeof(sockaddr_nl));
    if(ret < 0)
    {
        LOG_E("bind failed,ret=%d", ret);
        return -2;
    }
    family_id = getFamilyID();
    if(family_id < 0)
    {
        LOG_E("failed to get family id,ret=%d", family_id);
        return -3;
    }
    LOG_I("family id=%d", family_id);
    thread_rx_running = true;
    thread_rx = std::thread(ThreadRx, this);
    return 0;
#else
    return -1;
#endif

}

void ComNetLinkGeneric::closeLink()
{
#if __linux__==1
    if(sock > 0)
    {
        close(sock);
        sock = 0;
    }
    thread_rx_running = false;
    if(thread_rx.joinable())
    {
        thread_rx.join();
    }
#endif
}

bool ComNetLinkGeneric::sendMessage(int remote_id, int family_id, int cmd,
                                    int data_type, const void* data, int data_size)
{
#if __linux__==1
    int gnl_data_size = NLA_ALIGN(NLA_HDRLEN + data_size + GENL_HDRLEN) ;
    int total_size = NLMSG_SPACE(gnl_data_size);
    uint8* buf = new uint8[total_size]();
    struct nlmsghdr* nl_header = (struct nlmsghdr*)buf;
    nl_header->nlmsg_len = NLMSG_LENGTH(gnl_data_size);
    nl_header->nlmsg_type = family_id;
    nl_header->nlmsg_flags = NLM_F_REQUEST;
    nl_header->nlmsg_pid = this->id;
    nl_header->nlmsg_seq = 0;

    struct genlmsghdr* gnl_header = (struct genlmsghdr*)NLMSG_DATA(nl_header);
    gnl_header->cmd = cmd;
    gnl_header->version = 1;

    struct nlattr* gnl_attr = (struct nlattr*)((char*)gnl_header + GENL_HDRLEN);
    gnl_attr->nla_type = data_type;
    gnl_attr->nla_len = NLA_HDRLEN + data_size;
    memcpy((char*)gnl_attr + NLA_HDRLEN, data, data_size);

    struct sockaddr_nl addr_remote;
    memset(&addr_remote, 0, sizeof(struct sockaddr_nl));
    addr_remote.nl_family = AF_NETLINK;
    addr_remote.nl_pid = remote_id;
    addr_remote.nl_groups = 0;

    int size_sent = 0;
    do
    {
        int ret = sendto(sock, buf + size_sent, total_size - size_sent, 0, (struct sockaddr*)&addr_remote, sizeof(addr_remote));
        if(ret <= 0)
        {
            delete[] buf;
            return false;
        }
        size_sent += ret;
    }
    while(size_sent < total_size);
    delete[] buf;

    return true;
#else
    return false;
#endif
}

bool ComNetLinkGeneric::sendMessage(const void* data, int data_size)
{
    return sendMessage(0, family_id, 1, 1, data, data_size);
}

bool ComNetLinkGeneric::sendMessage(int remote_id, const void* data, int data_size)
{
    //本实现不使用Generic Netlink内置的cmd和data_type，完全由上层用户在data中自行定义
    return sendMessage(remote_id, family_id, 1, 1, data, data_size);
}

ComBytes ComNetLinkGeneric::recvMessage(int timeout_ms)
{
#if __linux__==1
    if(timeout_ms <= 0)
    {
        timeout_ms = 1000;
    }
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int select_rtn = select(sock + 1, &readfds, NULL, NULL, &tv);
    //如果对端关闭了连接,select的返回值可能为1,且errno为2
    if(select_rtn < 0)
    {
        LOG_W("select() failed,ret=%d,errno=%d:%s", select_rtn, errno, strerror(errno));
        return ComBytes();
    }
    if(select_rtn == 0)
    {
        LOG_T("select() timeout");
        return ComBytes();
    }
    if(FD_ISSET(sock, &readfds) == 0)
    {
        LOG_D("select() no data");
        return ComBytes();
    }

    int size = 0;
    int ret = 0;
    while((ret = recvfrom(sock, buf + size, buf_size - size, MSG_DONTWAIT, NULL, NULL)) > 0)
    {
        size += ret;
    }
    return ComBytes(buf, size);
#else
    return ComBytes();
#endif
}

void ComNetLinkGeneric::onMessage(int sender_id, const uint8* data, int data_size)
{
    LOG_I("sender=%d,data_size=%d", sender_id, data_size);
}

void ComNetLinkGeneric::ThreadRx(ComNetLinkGeneric* ctx)
{
#if __linux__==1
    if(ctx == NULL)
    {
        return;
    }

    while(ctx->thread_rx_running)
    {
        ComBytes result = ctx->recvMessage(1000);
        uint8* data = result.getData();
        int data_size = result.getDataSize();
        LOG_I("data=%p,data_size=%d", data, data_size);
        if(data == NULL || data_size <= (int)(NLMSG_HDRLEN + GENL_HDRLEN + NLA_HDRLEN))
        {
            continue;
        }
        do
        {
            struct nlmsghdr* nl_header = (struct nlmsghdr*)data;
            if(nl_header == NULL || nl_header->nlmsg_len <= NLMSG_HDRLEN)
            {
                LOG_E("failed");
                break;
            }
            struct genlmsghdr* gnl_header = (struct genlmsghdr*)NLMSG_DATA(nl_header);
            if(gnl_header == NULL)
            {
                LOG_E("failed");
                break;
            }

            struct nlattr* nla = (struct nlattr*)((char*)gnl_header + GENL_HDRLEN);
            if(nla == NULL || nla->nla_len < NLA_HDRLEN)
            {
                LOG_E("failed");
                break;
            }
            ctx->onMessage(nl_header->nlmsg_pid, (uint8*)nla + NLA_HDRLEN, nla->nla_len - NLA_HDRLEN);
            data_size -= nl_header->nlmsg_len;
            data += nl_header->nlmsg_len;
        }
        while(data_size > 0);
    }
#endif
}

