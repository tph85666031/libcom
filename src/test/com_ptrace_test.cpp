#include "com_ptrace_x86_64.h"
#include "com_log.h"

void com_ptrace_unit_test_suit(void** state)
{
    std::string result = com_run_shell_with_output("pidof a.out");
    pid_t pid = strtoul(result.c_str(), NULL, 10);
    std::string file_lib = "/data/T1/lib_text/out/linux_x64/libtest.so";

    PTrace_x86_64 trace(pid);

    trace.attach();
    void* handle = trace.pdlopen(file_lib.c_str());
    LOG_I("handle=%p", handle);
    //ptrace_dlsym(pid,handle,"loadMsg");
    void* sym_addr = trace.getSymbolAddr("test_yy");
    LOG_D("sym_addr=%p", sym_addr);

    trace.replace("calloc", "test_yy");
    trace.detach();
}

