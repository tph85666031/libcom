#include "com_thread.h"
#include "com_log.h"
#include "com_file.h"
#include "com_test.h"

class MyThreadPoll: public ComThreadPool<Message>, public ThreadRunner<std::string>
{
public:
    void threadPoolRunner(Message& msg)
    {
        LOG_D("thread poll got message, id=%u, data=%s", msg.getID(), msg.getString("data").c_str());
    };

    void threadRunner(std::string& msg)
    {
        LOG_I("called,str=%s", msg.c_str());
        ASSERT_STR_EQUAL(msg.c_str(), "threadRunner");
    };
};

static void thread_cpp_test(int val1, int val2)
{
    LOG_I("vals=%d,val2=%d", val1, val2);
    LOG_I("tid=%llu", com_thread_get_tid());
    ComProcessMutex mutex_a("/tmp/1.txt");
    LOG_I("start lock");
    mutex_a.lock();
    LOG_I("lock done");
    mutex_a.unlock();
    return;
}

static void thread_process_condition_test(int index, ComProcessCondition* cond)
{
    cond->wait();
    LOG_I("%d done", index);
    return;
}

void com_thread_unit_test_suit(void** state)
{
    std::vector<uint8> xxx;
    ComAutoClean ac([&]()
    {
        for(size_t i = 0; i < xxx.size(); i++)
        {
            LOG_I("%zu=%hu", i, xxx[i]);
        }
    });
    xxx.push_back(1);
    xxx.push_back(2);
    xxx.push_back(3);
    if(com_string_equal(getenv("CMD"), "cond"))
    {
        ComProcessCondition c1("c1");
        if(com_string_equal(getenv("OP"), "wait"))
        {
            c1.wait();
        }
        else
        {
            c1.notifyAll();
        }
        return;
    }
    else if(com_string_equal(getenv("CMD"), "mutex"))
    {
        ComProcessMutex m("m1");
        if(m.trylock() != 0)
        {
            LOG_I("try lock failed");
            m.lock();
        }

        com_sleep_s(atoi(getenv("SLEEP")));
        m.unlock();
        return;
    }
    else if(com_string_equal(getenv("CMD"), "sem"))
    {
        ComProcessSem s("s1");
        if(com_string_equal(getenv("OP"), "wait"))
        {
            s.wait();
        }
        else
        {
            s.post();
        }
        return;
    }
    LOG_I("current process path=%s", com_process_get_path().c_str());
    LOG_I("process name of %d is %s", com_process_get_pid(), com_process_get_name(com_process_get_pid()).c_str());
    LOG_I("current process id=%d", com_process_get_pid());
    LOG_I("current process ppid=%d", com_process_get_ppid());
#if defined(_WIN32) || defined(_WIN64)
    LOG_I("process id of %s,%d", "Everything.exe", com_process_get_pid("Everythi?g.exe"));
    LOG_I("process ppid of%s,%d", "Everything.exe", com_process_get_ppid(com_process_get_pid("Everything.exe")));
#else
    LOG_I("process id of %s,%d", "/usr/lib/bluetooth/obexd", com_process_get_pid("ob?xd"));
    LOG_I("process ppid of%s,%d", "/usr/lib/bluetooth/obexd", com_process_get_ppid(com_process_get_pid("/usr/lib/blu*oth/obexd")));
#endif
    com_thread_set_name("T testthread");
    LOG_I("thread name=%s", com_thread_get_name(com_thread_get_tid_posix()).c_str());

    ProcInfo info = com_process_get(com_process_get_pid());
    LOG_I("pid:%d   pgrp:%d   uid:%d,%d,%d,%d   gid:%d %d %d %d", info.pid, info.pgrp, info.uid, info.euid, info.suid, info.fsuid, info.gid, info.egid, info.sgid, info.fsgid);
    std::vector<ProcInfo> infos = com_process_get_parent_all(com_process_get_pid("Everythi?g.exe"));
    LOG_I("process count=%zu", infos.size());
    std::string ppid_str = com_string_format("%d", com_process_get_pid("Everythi?g.exe"));
    for(size_t i = 0; i < infos.size(); i++)
    {
        ppid_str += "->" + com_string_format("%d", infos[i].pid);
    }
    LOG_I("ppid_str=%s", ppid_str.c_str());
    std::vector<uint64> tids = com_thread_get_tid_all();
    for(size_t i = 0; i < tids.size(); i++)
    {
        LOG_I("pid %d:tid[%zu]=%llu", com_process_get_pid(), i, tids[i]);
    }
    int pid_test = com_process_get_pid("EndpointWatchdog");
    std::vector<ProcInfo> proc_childs = com_process_get_child_all(pid_test);
    for(size_t i = 0; i < proc_childs.size(); i++)
    {
        LOG_I("pid=%d,ppid=%d,bin=%s", proc_childs[i].pid, proc_childs[i].ppid, com_path_name(proc_childs[i].path.c_str()).c_str());
    }

    MyThreadPoll pool;
    pool.setThreadsCount(3, 5);
    pool.startThreadPool();
    for(int i = 0; i < 100; i++)
    {
        Message msg;
        msg.setID(i);
        msg.set("data", std::to_string(i));
        pool.pushPoolMessage(msg);
    }
    pool.waitAllDone();
    ComProcessMutex mutex_a("/tmp/1.txt");
    LOG_I("start lock");
    mutex_a.reset();
    mutex_a.lock();
    LOG_I("lock done");
    std::thread t1(thread_cpp_test, 1, 2);
    com_sleep_s(1);
    LOG_I("start unlock");
    mutex_a.unlock();
    LOG_I("unlock done");
    if(t1.joinable())
    {
        t1.join();
    }

    MyThreadPoll x;
    x.pushRunnerMessage("threadRunner");
    com_sleep_s(3);

    LOG_I("test share memory API");
    ComShareMemoryMap share_map_w;
    ASSERT_NOT_NULL(share_map_w.init("/tmp/1.txt", 1024));
    void* buf_w = share_map_w.getAddr();
    ASSERT_NOT_NULL(buf_w);
    memcpy(buf_w, "123456", 7);
    LOG_I("buf_w=%p,content=%s", buf_w, (char*)buf_w);

    ComShareMemoryMap share_map_r;
    ASSERT_NOT_NULL(share_map_r.init("/tmp/1.txt", 1024));
    void* buf_r = share_map_r.getAddr();
    LOG_I("buf_r=%p,content=%s", buf_r, (char*)buf_r);

    ComProcessSem process_sem_a("/tmp/1.txt");
    ComProcessSem process_sem_b("/tmp/1.txt");

    LOG_I("process_sem_a post=%d", process_sem_a.post());
    LOG_I("process_sem_b wait=%d", process_sem_b.wait());

    LOG_I("process_sem_a post=%d", process_sem_a.post());
    LOG_I("process_sem_b wait timeout=%d", process_sem_b.wait(1000));

    process_sem_b.uninit();
    LOG_I("process_sem_a wait=%d [after process_sem_b uninit]", process_sem_a.wait());
    LOG_I("process_sem_a wait timeut=%d [after process_sem_b uninit]", process_sem_a.wait(1000));
    LOG_I("process_sem_a post=%d [after process_sem_b uninit]", process_sem_a.post());

    process_sem_a.uninit();
    LOG_I("process_sem_b wait=%d [after process_sem_a uninit]", process_sem_b.wait());
    LOG_I("process_sem_b wait timeut=%d [after process_sem_a uninit]", process_sem_b.wait(1000));
    LOG_I("process_sem_b post=%d [after process_sem_a uninit]", process_sem_b.post());


    ComProcessCondition cond("process_condition");
    for(int i = 0; i < 10; i++)
    {
        std::thread t(thread_process_condition_test, i, &cond);
        t.detach();
    }
    //com_sleep_s(1);
    //cond.notifyAll();
    for(int i = 0; i < 15; i++)
    {
        com_sleep_s(1);
        LOG_I("notify one");
        cond.notifyOne();
    }
    //cond.notifyAll();
    com_sleep_s(3);
}

