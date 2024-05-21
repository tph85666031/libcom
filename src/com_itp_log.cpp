#if __linux__ == 1
#include <sys/shm.h>
#include <sys/sem.h>
union semun
{
    int              val;    /* Value for SETVAL */
    struct semid_ds* buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short*  array;  /* Array for GETALL, SETALL */
    struct seminfo*  __buf;  /* Buffer for IPC_INFO (Linux-specific) */
};
#endif

#if defined(__APPLE__)
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/sysctl.h>
#endif

#include "com_log.h"
#include "com_crc.h"
#include "com_itp_log.h"
#include "com_file.h"

#if defined(_WIN32) || defined(_WIN64)
#define ITP_SEM_VALID(x)      (x!=NULL)
#define ITP_MEM_VALID(x)      (x!=NULL)
#define ITP_SEM_DEFAULT_VALUE (NULL)
#define ITP_MEM_DEFAULT_VALUE (NULL)
#else
#define ITP_SEM_VALID(x)      (x>=0)
#define ITP_MEM_VALID(x)      (x>=0)
#define ITP_SEM_DEFAULT_VALUE (-1)
#define ITP_MEM_DEFAULT_VALUE (-1)
#endif

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint8 type;
    uint64 timestamp_ms;
    int len;
    uint8 data[0];
} ITP_TLV;

typedef struct
{
    bool invalid;
    uint8 rev[64];
    int tlv_count;
    int total_size;
    uint8 data[0];
} ITP_MSG;
#pragma pack(pop)

#define ITP_SHARE_MEM_SIZE_MAX (2*1024*1024)

int GetMaxSharedMemSize()
{
#if defined(__APPLE__)
    unsigned long maxShmSize = 0;
    size_t len = sizeof(maxShmSize);
    if(sysctlbyname("kern.sysv.shmmax", &maxShmSize, &len, NULL, 0) == 0)
    {
        unsigned long halfSize = maxShmSize / 2;
        return halfSize > ITP_SHARE_MEM_SIZE_MAX ? ITP_SHARE_MEM_SIZE_MAX : (int)halfSize;
    }
    else
    {
        return ITP_SHARE_MEM_SIZE_MAX;
    }
#else
    return ITP_SHARE_MEM_SIZE_MAX;
#endif
}

#define ITP_DATA_SIZE_MAX (GetMaxSharedMemSize() - sizeof(ITP_MSG))

ITPLogWriter& GetITPLogWriter()
{
    static ITPLogWriter writer;
    return writer;
}

ITPLog::ITPLog()
{
    sem_mem = ITP_SEM_DEFAULT_VALUE;
    mutex_mem = ITP_SEM_DEFAULT_VALUE;
    mem = ITP_MEM_DEFAULT_VALUE;
    msg = NULL;
}

ITPLog::~ITPLog()
{
    if(auto_destroy)
    {
        destroy();
    }
    else
    {
        uninit();
    }
}

void ITPLog::setPath(const char* path)
{
    if(path != NULL)
    {
        this->path = path;
    }
}

void ITPLog::enableAutoDestroy()
{
    this->auto_destroy = true;
}

bool ITPLog::init()
{
    if(path.empty())
    {
        return false;
    }
    if(ITP_MEM_VALID(mem) == false)
    {
        mem = getShareMem(path.c_str(), true);
        if(ITP_MEM_VALID(mem) == false)
        {
            LOG_E("failed to create share mem:0x%x", mem);
            return false;
        }
        msg = attachShareMem(mem);
        if(msg == NULL)
        {
            LOG_E("failed to attach share mem:0x%x", mem);
            return false;
        }
    }

    if(ITP_SEM_VALID(mutex_mem) == false)
    {
        mutex_mem = getSem(path.c_str(), true, 0, 1);
        if(ITP_SEM_VALID(mutex_mem) == false)
        {
            LOG_E("failed to create sem:0x%x", mutex_mem);
            return false;
        }
    }

    if(ITP_SEM_VALID(sem_mem) == false)
    {
        sem_mem = getSem(path.c_str(), true, 1);
        if(ITP_SEM_VALID(sem_mem) == false)
        {
            LOG_E("failed to create sem:0x%x", sem_mem);
            return false;
        }
    }

    return true;
}

void ITPLog::uninit()
{
    detachShareMem(msg);
    msg = NULL;
#if defined(_WIN32) || defined(_WIN64)
    destroyShareMem(mem);
    destroySem(mutex_mem);
    destroySem(sem_mem);
#endif
    mem = ITP_MEM_DEFAULT_VALUE;
    mutex_mem = ITP_SEM_DEFAULT_VALUE;
    sem_mem = ITP_SEM_DEFAULT_VALUE;
}

void ITPLog::destroy()
{
    waitSem(mutex_mem);
    if(msg != NULL)
    {
        ((ITP_MSG*)msg)->invalid = true;
        LOG_W("mark share mem destroied");
    }
    postSem(mutex_mem);

    detachShareMem(msg);
    msg = NULL;

    destroyShareMem(mem);
    mem = ITP_MEM_DEFAULT_VALUE;

    destroySem(mutex_mem);
    mutex_mem = ITP_SEM_DEFAULT_VALUE;

    destroySem(sem_mem);
    sem_mem = ITP_SEM_DEFAULT_VALUE;
}

itp_sem_t ITPLog::getSem(const char* name, bool create, uint8 offset, int init_val)
{
#if __linux__ == 1 || defined(__APPLE__)
    key_t key = ftok(name, offset);
    if(key == -1)
    {
        key = (com_crc32((uint8*)name, strlen(name)) + offset) & 0x7FFFFFFF;
        LOG_W("failed to create key, use crc32 value instead,key=0x%x", key);
    }
    int flag = 0666;
    itp_sem_t sem_id = semget(key, 1, flag);
    if(sem_id >= 0)
    {
        return sem_id;
    }
    else if(create)
    {
        flag |= IPC_CREAT;
        sem_id = semget(key, 1, flag);
        if(init_val > 0)
        {
            union semun arg;
            arg.val = init_val;
            semctl(sem_id, 0, SETVAL, arg);
        }
    }
    return sem_id;
#elif defined(_WIN32) || defined(_WIN64)
    if(name == NULL)
    {
        return NULL;
    }
    std::string str_name = com_string_format("%s_%d", name, offset);
    itp_share_mem_t handle = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, true, str_name.c_str());
    if(handle == NULL && create)
    {
        handle = CreateSemaphoreA(NULL, init_val, 1, str_name.c_str());
    }
    return handle;
#endif
}

int ITPLog::postSem(itp_sem_t sem)
{
    if(ITP_SEM_VALID(sem) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    int ret = semop(sem, &buf, 1);
    if(ret == 0)
    {
        return 0;
    }
    else if(errno == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(ReleaseSemaphore(sem, 1, NULL) == false)
    {
        return -1;
    }
    return 0;
#endif
}

int ITPLog::waitSem(itp_sem_t sem, int timeout_ms)
{
    if(ITP_SEM_VALID(sem) == false)
    {
        return -1;
    }
#if __linux__ == 1 || defined(__APPLE__)
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
#if defined(__APPLE__)
    buf.sem_flg = SEM_UNDO | IPC_NOWAIT;
#else
    buf.sem_flg = SEM_UNDO;
#endif

    int ret = 0;
    int err_no = 0;
    if(timeout_ms <= 0)
    {
        ret = semop(sem, &buf, 1);
        err_no = errno;
    }
#if __linux__ == 1
    else
    {
        //semtimedop使用相对时间
        struct timespec ts;
        memset(&ts, 0, sizeof(struct timespec));
        ts.tv_nsec = ((int64)timeout_ms % 1000) * 1000 * 1000;
        ts.tv_sec = timeout_ms / 1000;
        ret = semtimedop(sem, &buf, 1, &ts);
        err_no = errno;
    }
#endif
#if defined(__APPLE__)
    else
    {
        while(timeout_ms > 0)
        {

            ret = semop(sem, &buf, 1);
            err_no = errno;

            if(ret == 0 || err_no != EAGAIN)
            {
                break;
            }

            int wait_ms = 100;
            // 会修改errno 为 timeout
            com_sleep_ms(wait_ms);
            timeout_ms -= wait_ms;
        }
    }
#endif

    if(ret == 0)
    {
        return 0;
    }
    else if(err_no == EAGAIN)
    {
        return -2;
    }
    else if(err_no == EIDRM)
    {
        return -3;
    }
    else
    {
        return -1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(timeout_ms <= 0)
    {
        timeout_ms = INFINITE;
    }
    int ret = WaitForSingleObject(sem, timeout_ms);
    if(ret == WAIT_OBJECT_0)
    {
        return 0;
    }
    else if(ret == WAIT_TIMEOUT)
    {
        return -2;
    }
    else
    {
        return GetLastError();
    }
#endif
}

void ITPLog::destroySem(itp_sem_t sem)
{
    if(ITP_SEM_VALID(sem) == false)
    {
        return;
    }
#if __linux__ == 1 || defined(__APPLE__)
    semctl(sem, 0, IPC_RMID);
#elif defined(_WIN32) || defined(_WIN64)
    CloseHandle(sem);
#endif
}

itp_share_mem_t ITPLog::getShareMem(const char* name, bool create)
{
#if __linux__ == 1 || defined(__APPLE__)
    key_t key = ftok(name, 1);
    if(key == -1)
    {
        key = com_crc32((uint8*)name, strlen(name)) & 0x7FFFFFFF;
        LOG_W("failed to create key, use crc32 value instead:0x%x", key);
    }
    int flag = 0666;
    if(create)
    {
        flag |= IPC_CREAT;
    }
    return shmget(key, GetMaxSharedMemSize(), flag);
#elif defined(_WIN32) || defined(_WIN64)
    itp_share_mem_t handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, name);
    if(handle == NULL && create)
    {
        handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
                                   PAGE_READWRITE, 0, GetMaxSharedMemSize(), name);
    }
    return handle;
#endif
}

void* ITPLog::attachShareMem(itp_share_mem_t sm)
{
    if(ITP_MEM_VALID(sm) == false)
    {
        return NULL;
    }
#if __linux__ == 1 || defined(__APPLE__)
    return shmat(sm, NULL, 0);
#elif defined(_WIN32) || defined(_WIN64)
    return MapViewOfFile(sm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
}

void ITPLog::detachShareMem(void* addr)
{
    if(addr == NULL)
    {
        return;
    }
#if __linux__ == 1 || defined(__APPLE__)
    shmdt(addr);
#elif defined(_WIN32) || defined(_WIN64)
    UnmapViewOfFile(addr);
#endif
}

void ITPLog::destroyShareMem(itp_share_mem_t sm)
{
    if(ITP_MEM_VALID(sm) == false)
    {
        return;
    }
#if __linux__ == 1 || defined(__APPLE__)
    shmctl(sm, IPC_RMID, NULL);
#elif defined(_WIN32) || defined(_WIN64)
    CloseHandle(sm);
#endif
}

ITPLogWriter::ITPLogWriter()
{
}

ITPLogWriter::~ITPLogWriter()
{
}

void ITPLogWriter::writef(int type, const char* fmt, ...)
{
    if(fmt == NULL)
    {
        return;
    }

    std::string str;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len > 0)
    {
        len += 1;  //上面返回的长度不包含\0，这里加上
        std::vector<char> buf(len);
        va_start(args, fmt);
        vsnprintf(buf.data(), len, fmt, args);
        va_end(args);
        str.assign(buf.data());
    }

    write(type, str.c_str(), str.size());
    return;
}

void ITPLogWriter::write(int type, const std::string& val, bool autoPost)
{
    write(type, val.c_str(), val.size(), autoPost);
}

void ITPLogWriter::write(int type, const char* val, int length, bool autoPost)
{
    if(val == NULL || val[0] == '\0')
    {
        LOG_W("write share memory, log val is null");
        return;
    }

    if(init() == false)
    {
        LOG_W("write share memory, init false, discard itp msg");
        return;
    }

    int ret = waitSem(mutex_mem, 1000);
    if(ret < 0)
    {
        if(ret != -2)
        {
            LOG_W("sem destroied, reinit later,ret=%d", ret);
            uninit();
        }
        LOG_W("wait sem return < 0, discard itp msg, ret=%d", ret);
        return;
    }

    ITP_MSG* p_msg = (ITP_MSG*)msg;
    if(p_msg->invalid)
    {
        LOG_W("share mem destroied, reinit later, discard itp msg");
        uninit();
        postSem(mutex_mem);
        return;
    }

    if(length <= 0)
    {
        length = strlen(val);
    }

    if(p_msg->total_size + sizeof(ITP_TLV) + length + 1 <= ITP_DATA_SIZE_MAX)
    {
        ITP_TLV* tlv = (ITP_TLV*)(p_msg->data + p_msg->total_size);
        tlv->type = type;
        tlv->timestamp_ms = com_time_rtc_ms();
        tlv->len =  length + 1;
        memcpy(tlv->data, val, length + 1);//包含'\0'
        p_msg->total_size += sizeof(ITP_TLV) + tlv->len;
        p_msg->tlv_count++;
        if(!autoPost || (p_msg->total_size >= (int)ITP_DATA_SIZE_MAX / 2))
        {
            //消息过半后通知Reader来读取,Reader同时也会定时1秒读取消息
            int ret = postSem(sem_mem);//通知Reader来读
            LOG_D("post sem auto:%d, notify reader to read, ret:%d", autoPost, ret);
        }
    }
    else
    {
        LOG_D("write share memory, memory is overflow, and discard itp msg");
    }
    postSem(mutex_mem);
    return;
}

ITPLogReader::ITPLogReader()
{
    thread_reader_running = true;
    thread_reader = std::thread(ThreadReader, this);
}

ITPLogReader::~ITPLogReader()
{
    thread_reader_running = false;
    if(thread_reader.joinable())
    {
        thread_reader.join();
    }
}

void ITPLogReader::read(ComBytes& data, int tlv_count)
{
    if(data.empty())
    {
        return;
    }

    int size = 0;
    ITP_TLV* tlv = (ITP_TLV*)data.getData();
    for(int i = 0; i < tlv_count && size <= data.getDataSize(); i++)
    {

        std::string message((char*)tlv->data, tlv->len);
        onMessage(tlv->type, tlv->timestamp_ms, message);
        tlv = (ITP_TLV*)(tlv->data + tlv->len);
        size += sizeof(ITP_TLV) + tlv->len;
    }
}

void ITPLogReader::onMessage(uint8 type, uint64 timestamp_ms, const std::string& message)
{
    LOG_I("type=%d,%llu,data=%s", type, timestamp_ms, message.c_str());
}

void ITPLogReader::ThreadReader(ITPLogReader* ctx)
{
    if(ctx == NULL)
    {
        LOG_W("log reader initialize failure with null context");
        return;
    }

    ComBytes data;
    int tlv_count = 0;
    while(ctx->thread_reader_running)
    {
        if(ctx->init() == false)
        {
            LOG_W("log reader context init failure");
            com_sleep_s(1);
            continue;
        }
        //等待消息
        int ret = ctx->waitSem(ctx->sem_mem, 1000);
        if(ret < 0)
        {
            if(ret != -2)
            {
                LOG_W("sem destroied, reinit later,ret=%d", ret);
                ctx->uninit();
                continue;
            }
        }

        //加锁访问共享内存
        ret = ctx->waitSem(ctx->mutex_mem, 5000);
        if(ret < 0)
        {
            if(ret != -2)
            {
                LOG_W("sem destroied, reinit later,ret=%d", ret);
                ctx->uninit();
            }
            continue;
        }

        ITP_MSG* p_msg = (ITP_MSG*)ctx->msg;
        if(p_msg->invalid)
        {
            LOG_W("share mem destroied, reinit later");
            ctx->uninit();
            ctx->postSem(ctx->mutex_mem);
            continue;
        }

        if(p_msg->tlv_count > 0)
        {
            data.append(p_msg->data, p_msg->total_size);
            tlv_count = p_msg->tlv_count;
            p_msg->total_size = 0;
            p_msg->tlv_count = 0;
        }
        //释放锁
        ctx->postSem(ctx->mutex_mem);

        //解析数据
        ctx->read(data, tlv_count);
        data.clear();
        tlv_count = 0;
    }
}

