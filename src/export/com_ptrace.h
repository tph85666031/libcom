#ifndef __COM_PTRACE_H__
#define __COM_PTRACE_H__

#if __linux==1
#include <elf.h>

#include "com_base.h"
#include "com_thread.h"

#if __x86_32==1
typedef struct user_regs_struct USER_REGS;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Sym  Elf_Sym;
typedef Elf32_Rela Elf_Rela;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Phdr Elf_Phdr;
typedef Elf32_Dyn  Elf_Dyn;
typedef Elf32_Rela Elf_Rela;
#define ELF_R_SYM  ELF32_R_SYM
#elif __x86_64==1
typedef struct user_regs_struct USER_REGS;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Sym  Elf_Sym;
typedef Elf64_Rela Elf_Rela;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
typedef Elf64_Dyn  Elf_Dyn;
typedef Elf64_Rela Elf_Rela;
#define ELF_R_SYM  ELF64_R_SYM
#elif __mips64==1
typedef struct user_regs_struct USER_REGS;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Sym  Elf_Sym;
typedef Elf64_Rela Elf_Rela;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
typedef Elf64_Dyn  Elf_Dyn;
typedef Elf64_Rela Elf_Rela;
#define ELF_R_SYM  ELF64_R_SYM
#elif __arm64==1
typedef struct user_regs_struct USER_REGS;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Sym  Elf_Sym;
typedef Elf64_Rela Elf_Rela;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
typedef Elf64_Dyn  Elf_Dyn;
typedef Elf64_Rela Elf_Rela;
#define ELF_R_SYM  ELF64_R_SYM
#else
#error "not support"
#endif
class PTrace
{
public:
    PTrace(pid_t pid)
    {
        this->pid = pid;
    };
    virtual ~PTrace() {};
    virtual bool attach() = 0;
    virtual bool detach() = 0;
    virtual void* getEntryAddrBin() = 0;
    virtual void* getFreexAddr() = 0;
    virtual void* getEntryAddrLib(pid_t pid, const char* file_lib) = 0;
    virtual void* getSymbolAddr(const char* file_lib, const void* addr_func) = 0;
    virtual bool getRegs(USER_REGS* regs) = 0;
    virtual bool setRegs(USER_REGS* regs) = 0;
    virtual bool psyscall() = 0;
    virtual bool pcontinue() = 0;
    virtual int pwait() = 0;
    virtual bool pstep() = 0;
    virtual bool pwrite(void* addr, const void* data, int data_size) = 0;
    virtual bool pread(void* addr, void* buf, int buf_size) = 0;
    virtual std::string preadString(void* addr) = 0;
    virtual unsigned long pcall(void* addr_func, unsigned long* params, int params_count) = 0;
    virtual void* pmalloc(int size) = 0;
    virtual void pfree(void* val) = 0;
    virtual void* pdlopen(const char* file_lib) = 0;
    virtual void* pdlsym(void* handle, const char* symbol) = 0;
    virtual void pdlclose(void* handle) = 0;
    virtual bool loadElf() = 0;
    virtual void* getGotAddr(const char* sym_name) = 0;
    virtual void* getSymbolAddr(const char* sym_name) = 0;
    virtual void showSym() = 0;
    virtual bool replace(const char* func_name, const char* func_name_new) = 0;
    virtual bool replace(const char* func_name, Elf_Addr addr_new) = 0;
protected:
    ProcessID pid = 0;
    void* addr_entry = NULL;
    void* addr_dynamic = NULL;
    void* addr_gotplt = NULL;
    Elf_Sym* addr_symtab = NULL;
    char* addr_strtab = NULL;
    void* addr_hash = NULL;
    void* addr_debug = NULL;
    void* addr_link_map = NULL;
    Elf_Rela* addr_rel = NULL;
    Elf_Rela* addr_plt_rel = NULL;
    int plt_rel_count = 0;
    int rel_count = 0;
};
#endif

#endif /* __COM_PTRACE_H__ */
