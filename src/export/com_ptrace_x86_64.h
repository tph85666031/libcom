#include "com_base.h"
#include "com_ptrace.h"

class PTrace_x86_64 : public PTrace
{
public:
    PTrace_x86_64(ProcessID pid);
    virtual ~PTrace_x86_64();
    bool attach();
    bool detach();
    void* getEntryAddrBin();
    void* getFreexAddr();
    void* getEntryAddrLib(ProcessID pid, const char* file_lib);
    void* getSymbolAddr(const char* file_lib, const void* addr_func);
    bool getRegs(USER_REGS* regs);
    bool setRegs(USER_REGS* regs);
    bool psyscall();
    bool pcontinue();
    int pwait();
    bool pstep();
    bool pwrite(void* addr, const void* data, int data_size);
    bool pread(void* addr, void* buf, int buf_size);
    std::string preadString(void* addr);
    unsigned long pcall(void* addr_func, unsigned long* params, int params_count);
    void* pmalloc(int size);
    void pfree(void* val);
    void* pdlopen(const char* file_lib);
    void* pdlsym(void* handle, const char* symbol);
    void pdlclose(void* handle);
    bool loadElf();
    void* getGotAddr(const char* sym_name);
    void* getSymbolAddr(const char* sym_name);
    void showSym();
    bool replace(const char* func_name, const char* func_name_new);
    bool replace(const char* func_name, Elf64_Addr addr_new);
private:

};

