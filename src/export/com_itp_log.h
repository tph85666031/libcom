#ifndef __COM_ITP_LOG_H__
#define __COM_ITP_LOG_H__

#include "com_base.h"
#include "com_serializer.h"

#define ITP_LOG_TYPE_UNKNOWN  -1
#define ITP_LOG_TYPE_DLP      0
#define ITP_LOG_TYPE_SYSTEM   1
#define ITP_LOG_TYPE_PROCESS  2
#define ITP_LOG_TYPE_WFE      3

#if defined(_WIN32) || defined(_WIN64)
typedef HANDLE itp_sem_t;
typedef HANDLE itp_share_mem_t;
#else
typedef int itp_sem_t;
typedef int itp_share_mem_t;
#endif

//ITP日志公共信息头
class COM_EXPORT LogMessageCommon
{
public:
    std::string event_type;//此ITP信息类型
    int uid = 0;//此进程启动用户的uid
    int pid = 0;//发起进程的pid
    std::string username;//此进程的启动用户
    uint32 createtime = 0;//发起进程的创建时间
    std::string procname;//进程名
    std::string imagepath;//进程全路径
    uint64 timestamp = 0;//此ITP信息创建时间

    void init(const LogMessageCommon& msg)
    {
        //this->event_type = msg.event_type;由具体IPT信息来确定
        this->uid = msg.uid;
        this->pid = msg.pid;
        this->username = msg.username;
        this->createtime = msg.createtime;
        this->procname = msg.procname;
        this->imagepath = msg.imagepath;
        this->timestamp = msg.timestamp;
    }
};

//ITP System日志
class COM_EXPORT LogMessageSystemProxyChange : public LogMessageCommon
{
public:
    std::string active;//代理zuangtai on/off
    std::string type;//表示修改的是系统的代理还是firefox的代理
    std::string mode;//代理类型htpp https ftp...
    std::string address;//代理ip
    uint16 port = 0;//代理端口
    bool islocalhost = false;//代理ip是否是本地网段ip

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           active, type, mode, address, port, islocalhost);
};

class COM_EXPORT LogMessageSystemVpnStatus : public LogMessageCommon
{
public:
    std::string active;
    std::string address;
    uint16 port = 0;
    bool islocalhost = false;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           active, address, port, islocalhost);
};

class COM_EXPORT LogMessageSystemNetworkStatics : public LogMessageCommon
{
public:
    int64 packets_in = 0;
    int64 packets_out = 0;
    int64 bytes_in = 0;
    int64 bytes_out = 0;
    std::string ethernet_name;
    std::string ethernet_address;
    std::string ethernet_info;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           packets_in, packets_out, bytes_in, bytes_out, ethernet_name, ethernet_address, ethernet_info);
};

class COM_EXPORT LogMessageSystemServerStatus : public LogMessageCommon
{
public:
    std::string info;
    int server_pid = 0;
    std::string server_procname;
    std::string server_startmode;
    std::string server_imagepath;
    std::string server_display_name;
    std::string server_status;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           info, server_pid, server_procname, server_startmode, server_imagepath, server_display_name, server_status);
};

class COM_EXPORT LogMessageSystemAccountChange : public LogMessageCommon
{
public:
    std::string type;
    std::string login_type;
    std::string _username;
    int _uid = 0;
    std::string groupName;
    int gid = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           type, login_type, _username, _uid, groupName, gid);
};

class COM_EXPORT LogMessageSystemFileShareChange : public LogMessageCommon
{
public:
    std::string active;
    std::string type;
    std::string share_name;
    std::string share_path;
    std::string change_path;
    int64 size = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           active, type, share_name, share_path, change_path, size);
};

class COM_EXPORT LogMessageSystemFirewall : public LogMessageCommon
{
public:
    std::string active;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           active);
};

class COM_EXPORT LogMessageSystemDeviceConnect : public LogMessageCommon
{
public:
    std::string type;
    std::string device_type;
    int device_id = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           type, device_type, device_id);
};

class COM_EXPORT LogMessageSystemApplicationList : public LogMessageCommon
{
public:
    std::string event_type = "SYSTEM_APPLICATION_LIST";
    std::string type;
    std::string path;
    std::string name;
    int bundle_id = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           type, path, name, bundle_id);
};

//ITP Process日志
class COM_EXPORT LogMessageProcessResourceUsage : public LogMessageCommon
{
public:
    std::string event_type = "PROCESS_RESOURCE_USAGE_USER";
    int cpu_percent = 0;
    int64 mem_size = 0;
    std::string command;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           cpu_percent, mem_size, command);
};

class COM_EXPORT NetworkProtocolStastics
{
public:
    std::string direct;
    std::string protocol;
    int count = 0;
    std::string d_addres;
    uint16 d_port = 0;

    META_J(direct, protocol, count, d_addres, d_port);
};

class COM_EXPORT LogMessageProcessResourceUsageKernelNetwork : public LogMessageCommon
{
public:
    int timestamp_begin = 0;
    int timestamp_end = 0;
    int vpn_status = 0;
    int64 in_bytes = 0;
    int64 in_packets = 0;
    int64 out_bytes = 0;
    int64 out_packets = 0;
    NetworkProtocolStastics addresses;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           timestamp_begin, timestamp_end, vpn_status, in_bytes, in_packets,
           out_bytes, out_packets, addresses);
};

class COM_EXPORT LogMessageProcessResourceUsageKernelOpen : public LogMessageCommon
{
public:
    int timestamp_begin = 0;
    int timestamp_end = 0;
    int64 count = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           timestamp_begin, timestamp_end, count);
};

class COM_EXPORT LogMessageProcessResourceUsageKernelDelete : public LogMessageProcessResourceUsageKernelOpen
{
};

class COM_EXPORT LogMessageProcessResourceUsageKernelHidden : public LogMessageCommon
{
public:
    int timestamp = 0;
    std::string path;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           timestamp, path);
};

class COM_EXPORT LogMessageProcessResourceUsageKernelRename : public LogMessageCommon
{
public:
    std::string event_type = "PROCESS_RESOURCE_USAGE_KERNEL_RENAME";
    int timestamp = 0;
    std::string from;
    std::string to;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           timestamp, from, to);
};

class COM_EXPORT LogMessageProcessResourceUsageKernelRegSet : public LogMessageCommon
{
public:
    int timestamp = 0;
    std::string path;
    std::string key;
    std::string value;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           timestamp, path, key, value);
};

class COM_EXPORT LogMessageProcessResourceUsageKernelRegDelete : public LogMessageCommon
{
public:
    int timestamp = 0;
    std::string path;
    std::string key;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           timestamp, path, key);
};

//ITP DLP日志
class COM_EXPORT LogMessageDlpImFile : public LogMessageCommon
{
public:
    std::string event_type = "DLP_IM_FILE";
    std::string file_path;
    int64 file_size = 0;
    std::string transfer_direction;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           file_path, file_size, transfer_direction);
};

class COM_EXPORT LogMessageDlpFileEntropy : public LogMessageCommon
{
public:
    std::string event_type = "DLP_FILE_ENTROPY";
    double entropy = 0.0f;
    std::string action_type;
    std::string file_path;
    std::string file_type;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           entropy, action_type, file_path, file_type);
};

class COM_EXPORT LogMessageDlpTripWire : public LogMessageCommon
{
public:
    int action_type = 0;
    std::string file_path;
    bool trigger = false;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           action_type, file_path, trigger);
};

class COM_EXPORT LogMessageDlpProcess : public LogMessageCommon
{
public:
    std::string event_type = "DLP_PROCESS";
    int type = 0;
    bool codesign_verify = false;
    std::string command;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           type, codesign_verify, command);
};

class COM_EXPORT LogMessageDlpImMessage : public LogMessageCommon
{
public:
    std::string event_type = "DLP_IM_MESSAGE";
    std::string message;
    std::string transfer_direction;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           message, transfer_direction);
};

class COM_EXPORT LogMessageDlpUsbFile : public LogMessageCommon
{
public:
    std::string event_type = "DLP_USB_FILE";
    std::string usb_name;
    std::string file_path;
    int64 file_size = 0;
    std::string transfer_direction;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           usb_name, file_path, file_size, transfer_direction);
};

class COM_EXPORT LogMessageDlpNfsFile : public LogMessageDlpImFile
{
public:
    std::string event_type = "DLP_NFS_FILE";
};

class COM_EXPORT LogMessageDlpAirdropFile : public LogMessageDlpImFile
{
public:
    std::string event_type = "DLP_AIRDROP_FILE";
};

class COM_EXPORT LogMessageDlpBluetoothFile : public LogMessageCommon
{
public:
    std::string event_type = "DLP_BLUETOOTH_FILE";
    std::string file_path;
    int64 file_size = 0;
    std::string transfer_direction;
    std::string peer_bluetooth_name;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           file_path, file_size, transfer_direction, peer_bluetooth_name);
};

class COM_EXPORT LogMessageDlpPrint : public LogMessageCommon
{
public:
    std::string event_type = "DLP_PRINT";
    std::string pdf_file_path;
    int64 file_size = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           pdf_file_path, file_size);
};

class COM_EXPORT LogMessageDlpScreenCapture : public LogMessageCommon
{
public:
    std::string event_type = "DLP_SCREENCAPTURE";
};

class COM_EXPORT LogMessageDlpPasteBoardChange : public LogMessageCommon
{
public:
    std::string event_type = "DLP_PASTEBOARD_CHANGE";
};

class COM_EXPORT LogMessageDlpAgentQuit : public LogMessageCommon
{
public:
    std::string event_type = "DLP_AGENT_QUIT";
    int exitcode = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           exitcode);
};

class COM_EXPORT LogMessageDlpHttpAccess : public LogMessageCommon
{
public:
    std::string event_type = "DLP_HTTP_ACCESS";
    std::string type;
    std::string url;
    std::string address;
    uint16 port = 0;
    std::string method;
    int64 bytes = 0;
    int direct = 0;
    int islocalhost = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           type, url, address, port, method, bytes, direct, islocalhost);
};

class COM_EXPORT LogMessageDlpMailAccess : public LogMessageCommon
{
public:
    std::string event_type = "DLP_MAIL_ACCESS";
    std::string type;
    std::string address;
    uint16 port = 0;
    int64 bytes = 0;
    int direct = 0;
    int nrecivers = 0;
    int islocalhost = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           type, address, port, bytes, direct, nrecivers, islocalhost);
};

class COM_EXPORT LogMessageDlpFtpAccess : public LogMessageCommon
{
public:
    std::string event_type = "DLP_FTP_ACCESS";
    std::string address;
    uint16 port = 0;
    std::string mode;
    int direct = 0;
    std::string file_name;
    int64 file_size = 0;
    int islocalhost = 0;

    META_J(event_type, uid, pid, username, createtime, procname, imagepath, timestamp,
           address, port, mode, direct, file_name, file_size, islocalhost);
};

class COM_EXPORT LogMessageWFE
{
public:
};

class COM_EXPORT ITPLog
{
public:
    ITPLog();
    virtual ~ITPLog();
    void setPath(const char* path);
    void enableAutoDestroy();
    void destroy();
protected:
    bool init();
    void uninit();
    itp_sem_t getSem(const char* name, bool create = false, uint8 offset = 0, int init_val = 0);
    int postSem(itp_sem_t sem);
    int waitSem(itp_sem_t sem, int timeout_ms = -1);
    void destroySem(itp_sem_t sem);

    itp_share_mem_t getShareMem(const char* name, bool create = false);
    void* attachShareMem(itp_share_mem_t sm);
    void detachShareMem(void* addr);
    void destroyShareMem(itp_share_mem_t sm);
protected:
    itp_sem_t sem_mem;
    itp_sem_t mutex_mem;
    itp_share_mem_t mem;
    void* msg = NULL;
    std::string name_a;
    std::string name_b;
    std::string path;
    bool auto_destroy = false;
};

class COM_EXPORT ITPLogWriter : public ITPLog
{
public:
    ITPLogWriter();
    virtual ~ITPLogWriter();
    void writef(int type, const char* fmt, ...);
    void write(int type, const std::string& val, bool autoPost = true);
    void write(int type, const char* val, int length = 0, bool autoPost = true);
};

class COM_EXPORT ITPLogReader : public ITPLog
{
public:
    ITPLogReader();
    virtual ~ITPLogReader();
private:
    virtual void onMessage(uint8 type, uint64 timestamp_ms, const std::string& message);
    void read(ComBytes& data, int tlv_count);
    static void ThreadReader(ITPLogReader* ctx);

private:
    std::thread thread_reader;
    std::atomic<bool> thread_reader_running = {false};
};

COM_EXPORT ITPLogWriter& GetITPLogWriter();

#define iLOG(type,str_val)  GetITPLogWriter().write(type, str_val)
#define iLOG_SYS(str_val)   GetITPLogWriter().write(ITP_LOG_TYPE_SYSTEM, str_val)
#define iLOG_PROC(str_val)  GetITPLogWriter().write(ITP_LOG_TYPE_PROCESS, str_val)
#define iLOG_DLP(str_val)   GetITPLogWriter().write(ITP_LOG_TYPE_DLP, str_val)
#define iLOG_WFE(str_val)   GetITPLogWriter().write(ITP_LOG_TYPE_WFE, str_val)

#endif /* __COM_ITP_LOG_H__ */

