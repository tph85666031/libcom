#if __mips64==1
#include <assert.h>
#include <arch_regs.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <syscall.h>
#include <string.h>
#include <dlfcn.h>
#include <link.h>
#include "com_ptrace_mips_64.h"
#include "com_log.h"
#include "com_file.h"

void __asm_call_start_mips_64__()
{
    asm
    (
        "callq *%r12 \n"
        "int $3"
    );
}

void __asm_call_end_mips_64__()
{
}

PTrace_mips_64::PTrace_mips_64(pid_t pid) : PTrace(pid)
{
    std::string file_map = com_string_format("/proc/%d/maps", pid);
    FILE* file = com_file_open(file_map.c_str(), "r");
    if(file == NULL)
    {
        LOG_E("failed to open bin mem maps file");
    }
    char buf[4096];
    while(com_file_readline(file, buf, sizeof(buf)))
    {
        std::vector<std::string> items = com_string_split(buf, "-");
        if(items.size() <= 1)
        {
            continue;
        }
        uint64 addr = strtoull(items[0].c_str(), NULL, 16);
        if(addr == 0x8000)
        {
            addr = 0;
        }
        addr_entry = (void*)addr;
        break;
    }
    com_file_close(file);
    if(addr_entry == NULL)
    {
        LOG_E("failed to get bin entry address");
    }
    LOG_D("entry address is %p", addr_entry);
}

PTrace_mips_64::~PTrace_mips_64()
{
    detach();
}

bool PTrace_mips_64::attach()
{
    if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
    {
        LOG_E("failed attach to target process");
        return false;
    }
    pwait();

    if(loadElf() == false)
    {
        detach();
        LOG_E("failed to load elf info from memory");
        return false;
    }
    return true;
}

bool PTrace_mips_64::detach()
{
    return (ptrace(PTRACE_DETACH, pid, NULL, NULL) >= 0);
}

void* PTrace_mips_64::getEntryAddrBin()
{
    return addr_entry;
}

void* PTrace_mips_64::getFreexAddr()
{
    std::string file_map = "/proc/self/maps";
    if(pid > 0)
    {
        file_map = com_string_format("/proc/%d/maps", pid);
    }
    FILE* file = com_file_open(file_map.c_str(), "r");
    if(file == NULL)
    {
        LOG_E("failed");
        return NULL;
    }
    char buf[4096];
    uint64 addr = 0;
    while(com_file_readline(file, buf, sizeof(buf)))
    {
        if(com_string_contain(buf, "xp") == false)
        {
            continue;
        }
        std::vector<std::string> items = com_string_split(buf, "-");
        if(items.size() <= 1)
        {
            continue;
        }
        addr = strtoull(items[0].c_str(), NULL, 16);
        if(addr == 0x8000)
        {
            addr = 0;
        }
        break;
    }
    com_file_close(file);
    return (void*)(addr + 8);
}

void* PTrace_mips_64::getEntryAddrLib(pid_t pid, const char* file_lib)
{
    std::string file_map = "/proc/self/maps";
    if(pid > 0)
    {
        file_map = com_string_format("/proc/%d/maps", pid);
    }
    FILE* file = com_file_open(file_map.c_str(), "r");
    if(file == NULL)
    {
        LOG_E("failed");
        return NULL;
    }
    char buf[4096];
    uint64 addr = 0;
    while(com_file_readline(file, buf, sizeof(buf)))
    {
        if(com_string_contain(buf, file_lib) == false)
        {
            continue;
        }
        if(com_string_contain(buf, "xp") == false)
        {
            continue;
        }
        std::vector<std::string> items = com_string_split(buf, "-");
        if(items.size() <= 1)
        {
            continue;
        }
        addr = strtoull(items[0].c_str(), NULL, 16);
        if(addr == 0x8000)
        {
            addr = 0;
        }
        break;
    }
    com_file_close(file);
    return (void*)addr;
}

void* PTrace_mips_64::getSymbolAddr(const char* file_lib, const void* addr_func)
{
    void* remote_addr_base = getEntryAddrLib(pid, file_lib);
    void* local_addr_base = getEntryAddrLib(0, file_lib);
    if(remote_addr_base == NULL || local_addr_base == NULL)
    {
        LOG_E("failed,remote_addr_base=%p,local_addr_base=%p",
              remote_addr_base, local_addr_base);
        return NULL;
    }
    return (void*)((uint64)remote_addr_base + ((uint64)addr_func - (uint64)local_addr_base));
}

void* PTrace_mips_64::getSymbolAddr(const char* sym_name)
{
    Elf_Dyn dyn;
    Elf_Sym sym;
    struct link_map map;
    if(addr_link_map == NULL)
    {
        return NULL;
    }
    void* addr_link_map_tmp = addr_link_map;
    do
    {
        pread(addr_link_map_tmp, &map, sizeof(map));
        std::string name = preadString(map.l_name);
        if(com_string_len(name.c_str()) == 0)//略过app本身的符号表只查询依赖的lib库
        {
            addr_link_map_tmp = map.l_next;
            continue;
        }
        LOG_D("lib name=%s,l_addr=0x%zx,l_ld=%p", name.c_str(), map.l_addr, map.l_ld);
        int index = 0;
        Elf_Sym* addr_symtab = NULL;
        char* addr_strtab = NULL;
        do
        {
            pread((void*)((unsigned long)map.l_ld + index * sizeof(dyn)), &dyn, sizeof(dyn));
            index++;
            if(dyn.d_tag == DT_SYMTAB)
            {
                addr_symtab = (Elf_Sym*)dyn.d_un.d_val;
            }
            else if(dyn.d_tag == DT_STRTAB)
            {
                addr_strtab = (char*)dyn.d_un.d_val;
            }
        }
        while(dyn.d_tag != DT_NULL);

        for(int i = 0; i < 10000; i++)
        {
            pread(addr_symtab + i, &sym, sizeof(sym));
            if(sym.st_name == 0 && sym.st_value == 0 && sym.st_size == 0)
            {
                continue;
            }
            char* addr_name = addr_strtab + sym.st_name;
            std::string name = preadString(addr_name);
            if(name.empty() == false && name[0] != '\0' && (name[0] < 32 || name[0] >= 127))
            {
                break;
            }
            if(com_string_equal(sym_name, name.c_str()))
            {
                return (void*)(map.l_addr + sym.st_value);
            }
        }
        addr_link_map_tmp = map.l_next;
    }
    while(map.l_next != NULL);
    return NULL;
}

void* PTrace_mips_64::getGotAddr(const char* sym_name)
{
    if(sym_name == NULL)
    {
        return NULL;
    }

    Elf_Rela rel;
    Elf_Sym sym;
    for(int i = 0; i < rel_count; i++)
    {
        pread(addr_rel + i, &rel, sizeof(rel));
        if(ELF_R_SYM(rel.r_info) == 0)
        {
            continue;
        }
        pread((void*)((Elf_Addr)addr_symtab + ELF_R_SYM(rel.r_info)*sizeof(Elf_Sym)), &sym, sizeof(sym));
        std::string name = preadString(addr_strtab + sym.st_name);
        if(com_string_equal(name.c_str(), sym_name))
        {
            //获取指向got对应位置的指针
            return (void*)((Elf_Addr)addr_entry + rel.r_offset);
        }
    }
    for(int i = 0; i < plt_rel_count; i++)
    {
        pread(addr_plt_rel + i, &rel, sizeof(rel));
        pread((void*)((Elf_Addr)addr_symtab + ELF_R_SYM(rel.r_info)*sizeof(Elf_Sym)), &sym, sizeof(sym));
        std::string name = preadString(addr_strtab + sym.st_name);
        if(com_string_equal(name.c_str(), sym_name))
        {
            //获取指向got对应位置的指针
            return (void*)((Elf_Addr)addr_entry + rel.r_offset);
        }
    }
    return NULL;
}

bool PTrace_mips_64::getRegs(USER_REGS* regs)
{
    if(ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0)
    {
        return false;
    }

    return true;
}

bool PTrace_mips_64::setRegs(USER_REGS* regs)
{
    if(ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0)
    {
        return false;
    }

    return true;
}

bool PTrace_mips_64::psyscall()
{
    if(ptrace(PTRACE_SYSCALL, pid, NULL, 0) < 0)
    {
        return false;
    }

    return true;
}

bool PTrace_mips_64::pcontinue()
{
    if(ptrace(PTRACE_CONT, pid, NULL, 0) < 0)
    {
        return false;
    }

    return true;
}

int PTrace_mips_64::pwait()
{
    int status = 0;
    waitpid(pid, &status, 0);
    return status;
}

bool PTrace_mips_64::pstep()
{
    if(ptrace(PTRACE_SINGLESTEP, pid, NULL, 0) < 0)
    {
        return false;
    }

    return true;
}

bool PTrace_mips_64::pwrite(void* addr, const void* data, int data_size)
{
    union
    {
        long val;
        char data[sizeof(long)];
    } item;

    uint8* ptr_src = (uint8*)data;
    uint8* ptr_dst = (uint8*)addr;
    for(int i = 0; i < data_size / sizeof(long); i++)
    {
        memcpy(item.data, ptr_src, sizeof(item.data));
        if(ptrace(PTRACE_POKETEXT, pid, ptr_dst, item.val) == -1)
        {
            return false;
        }
        ptr_src += sizeof(long);
        ptr_dst += sizeof(long);
    }

    int remain = data_size % sizeof(long);
    if(remain > 0)
    {
        item.val = 0;
        memcpy(item.data, ptr_src, remain);
        if(ptrace(PTRACE_POKETEXT, pid, ptr_dst, item.val) == -1)
        {
            return false;
        }
    }

    return true;
}

bool PTrace_mips_64::pread(void* addr, void* buf, int buf_size)
{
    union
    {
        long val;
        char data[sizeof(long)];
    } item;

    uint8* ptr_src = (uint8*)addr;
    uint8* ptr_dst = (uint8*)buf;
    for(int i = 0; i < buf_size / sizeof(long); i++)
    {
        item.val = ptrace(PTRACE_PEEKTEXT, pid, ptr_src, 0);
        memcpy(ptr_dst, item.data, sizeof(item.data));
        ptr_src += sizeof(long);
        ptr_dst += sizeof(long);
    }

    int remain = buf_size % sizeof(long);
    if(remain > 0)
    {
        item.val = ptrace(PTRACE_PEEKTEXT, pid, ptr_src, 0);
        memcpy(ptr_dst, item.data, remain);
    }

    return true;
}

std::string PTrace_mips_64::preadString(void* addr)
{
    union
    {
        long val;
        char data[sizeof(long)];
    } item;

    std::string result;
    uint8* ptr_src = (uint8*)addr;

    int i = 0;
    do
    {
        item.val = ptrace(PTRACE_PEEKTEXT, pid, ptr_src, 0);
        ptr_src += sizeof(long);
        int pos = -1;
        for(int i = 0; i < sizeof(item.data); i++)
        {
            if(item.data[i] == '\0')
            {
                pos = i;
                break;
            }
        }
        if(pos == -1)
        {
            result.append(item.data, sizeof(item.data));
        }
        else
        {
            result.append(item.data, pos + 1);
            break;
        }
        i++;
    }
    while(i < 128);

    return result;
}

unsigned long PTrace_mips_64::pcall(void* addr_func, unsigned long* params, int params_count)
{
    struct user_regs_struct regs;
    struct user_regs_struct regs_bak;
    getRegs(&regs);
    memcpy(&regs_bak, &regs, sizeof(regs_bak));

    for(int i = 0; i < params_count; i++)
    {
        if(i == 0)
        {
            regs.rdi = params[0];
        }
        else if(i == 1)
        {
            regs.rsi = params[1];
        }
        else if(i == 2)
        {
            regs.rdx = params[2];
        }
        else if(i == 3)
        {
            regs.rcx = params[3];
        }
        else if(i == 4)
        {
            regs.r8 = params[4];
        }
        else if(i == 5)
        {
            regs.r9 = params[5];
        }
    }

    void* freex_addr = getFreexAddr();
    int shellcode_size = (unsigned long)__asm_call_end_mips_64__ - (unsigned long)__asm_call_start_mips_64__;
    char buf[shellcode_size];
    pread(freex_addr, buf, sizeof(buf));
    pwrite(freex_addr, (void*)__asm_call_start_mips_64__, shellcode_size);

    regs.r12 = (unsigned long)addr_func;
    regs.rip = (unsigned long)freex_addr + 2;

    setRegs(&regs);
    pcontinue();
    pwait();
    getRegs(&regs);

    //恢复数据
    setRegs(&regs_bak);
    pwrite(freex_addr, buf, sizeof(buf));

    return regs.rax;
}

void* PTrace_mips_64::pmalloc(int size)
{
    void* addr_malloc = getSymbolAddr("libc", (void*)malloc);
    unsigned long params[] = {(unsigned long)size};
    unsigned long val = pcall(addr_malloc, params, 1);
    if(val < 0x1000)
    {
        val = 0;
    }
    return (void*)val;
}

void PTrace_mips_64::pfree(void* val)
{
    void* addr_free = getSymbolAddr("libc", (void*)free);
    unsigned long params[] = {(unsigned long)val};
    pcall(addr_free, params, 1);
}

void* PTrace_mips_64::pdlopen(const char* file_lib)
{
    void* addr_dlopen = getSymbolAddr("libdl", (void*)dlopen);
    LOG_D("addr_dlopen=%p,file=%s", addr_dlopen, file_lib);
    if(addr_dlopen == NULL)
    {
        LOG_E("failed");
        return NULL;
    }

    void* addr_param1 = pmalloc(128);
    LOG_D("addr_param1=%p", addr_param1);
    if(addr_param1 == NULL)
    {
        LOG_E("failed");
        return NULL;
    }
    pwrite(addr_param1, file_lib, strlen(file_lib) + 1);
    unsigned long params[] = {(unsigned long)addr_param1, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE};
    void* handle = (void*)pcall(addr_dlopen, params, 2);
    pfree(addr_param1);
    return handle;
}

void* PTrace_mips_64::pdlsym(void* handle, const char* symbol)
{
    void* addr_dlsym = getSymbolAddr("libdl", (void*)dlsym);
    if(addr_dlsym == NULL)
    {
        LOG_E("failed");
        return NULL;
    }

    void* addr_param1 = pmalloc(128);
    if(addr_param1 == NULL)
    {
        LOG_E("failed");
        return NULL;
    }
    pwrite(addr_param1, symbol, strlen(symbol) + 1);
    unsigned long params[] = {(unsigned long)handle, (unsigned long)addr_param1};
    void* func = (void*)pcall(addr_dlsym, params, 2);
    pfree(addr_param1);
    return func;
}

void PTrace_mips_64::pdlclose(void* handle)
{
    void* addr_dlclose = getSymbolAddr("libdl", (void*)dlclose);
    if(addr_dlclose == NULL)
    {
        LOG_E("failed");
        return;
    }

    unsigned long params[] = {(unsigned long)handle};
    pcall(addr_dlclose, params, 2);
}

bool PTrace_mips_64::loadElf()
{
    Elf_Ehdr ehdr;
    Elf_Phdr phdr;
    Elf_Dyn dyn;

    memset(&ehdr, 0, sizeof(ehdr));
    memset(&phdr, 0, sizeof(phdr));
    memset(&dyn, 0, sizeof(dyn));

    pread(addr_entry, &ehdr, sizeof(ehdr));
    Elf_Addr* phdr_addr = (Elf_Addr*)((unsigned long)addr_entry + ehdr.e_phoff);
    // 遍历program headers table，找到.dynamic
    LOG_D("count=%d,%s,0x%zx", ehdr.e_phnum, ehdr.e_ident, ehdr.e_entry);
    for(int i = 0; i < ehdr.e_phnum; i++)
    {
        pread((unsigned char*)phdr_addr + i * sizeof(Elf_Phdr), &phdr, sizeof(phdr));
        assert(phdr.p_align == 0 || (phdr.p_vaddr % phdr.p_align) == (phdr.p_offset % phdr.p_align));
        if(phdr.p_type == PT_DYNAMIC)
        {
            addr_dynamic = (void*)((Elf_Addr)addr_entry + phdr.p_vaddr);
            break;
        }
    }

    if(addr_dynamic == NULL)
    {
        LOG_E("cannot find the address of .dynamin");
        return false;
    }

    LOG_D("addr_dynamic=%p", addr_dynamic);
    // 遍历.dynamic，找到.got.plt
    int index = 0;
    int rel_size = 0;
    int pltrel_size = 0;
    int relent_size = 0;
    do
    {
        pread((void*)((Elf_Addr)addr_dynamic + index * sizeof(Elf_Dyn)), &dyn, sizeof(Elf_Dyn));
        //LOG_I("dtype type=%zd", dyn.d_tag);
        if(dyn.d_tag == DT_PLTGOT)
        {
            addr_gotplt = (void*)dyn.d_un.d_val;
            LOG_D("addr_gotplt=%p", addr_gotplt);
        }
        else if(dyn.d_tag == DT_SYMTAB)
        {
            addr_symtab = (Elf_Sym*)dyn.d_un.d_val;
            LOG_D("addr_symtab=%p", addr_symtab);
        }
        else if(dyn.d_tag == DT_STRTAB)
        {
            addr_strtab = (char*)dyn.d_un.d_val;
            LOG_D("addr_strtab=%p", addr_strtab);
        }
        else if(dyn.d_tag == DT_GNU_HASH)
        {
            addr_hash = (void*)dyn.d_un.d_val;
            LOG_D("addr_hash=%p", addr_hash);
        }
        else if(dyn.d_tag == DT_DEBUG)
        {
            addr_debug = (void*)dyn.d_un.d_val;
            LOG_D("addr_debug=%p", addr_debug);
        }
        else if(dyn.d_tag == DT_REL)
        {
            addr_rel = (Elf_Rela*)dyn.d_un.d_val;
            LOG_D("addr_rel=%p", addr_rel);
        }
        else if(dyn.d_tag == DT_JMPREL)
        {
            addr_plt_rel = (Elf_Rela*)dyn.d_un.d_val;
            LOG_D("addr_plt_rel=%p", addr_plt_rel);
        }
        else if(dyn.d_tag == DT_RELSZ)
        {
            rel_size = dyn.d_un.d_val;
            LOG_D("rel_size=%d", rel_size);
        }
        else if(dyn.d_tag == DT_PLTRELSZ)
        {
            pltrel_size = dyn.d_un.d_val;
            LOG_D("pltrel_size=%d", pltrel_size);
        }
        else if(dyn.d_tag == DT_RELENT || dyn.d_tag == DT_RELAENT)
        {
            relent_size = dyn.d_un.d_val;
            LOG_D("relent_size=%d", relent_size);
        }
        index++;
    }
    while(dyn.d_tag != DT_NULL);

    if((addr_gotplt == NULL && addr_debug == NULL) || relent_size == 0)
    {
        LOG_E("cannot find the address of .got.plt or .debug");
        return false;
    }

    plt_rel_count = pltrel_size / relent_size;
    rel_count = rel_size / relent_size;

    // 获取link_map地址
    //GOT[0]存放.dnamic相对地址
    //GOT[1]存放linkmap相对地址
    //GOT[2]存放dl_runtime_resolve函数地址
    //GOT[3]....存放实际lib函数地址，与PLT[1]开始一一对应
    pread((void*)((Elf_Addr)addr_gotplt + sizeof(Elf_Addr)), &addr_link_map, sizeof(void*));
    LOG_D("link_map=%p", addr_link_map);
    if(addr_link_map == 0)
    {
        pread((void*)((Elf_Addr)addr_debug + sizeof(Elf_Addr)), &addr_link_map, sizeof(void*));
        LOG_D("link_map=%p", addr_link_map);
    }
    return true;
}

void PTrace_mips_64::showSym()
{
    Elf_Dyn dyn;
    Elf_Sym sym;
    struct link_map map;
    void* addr_link_map_tmp = addr_link_map;
    do
    {
        pread(addr_link_map_tmp, &map, sizeof(map));
        std::string name = preadString(map.l_name);
        //LOG_I("lib name=%s,l_addr=0x%zx,l_ld=%p", name.c_str(), map.l_addr, map.l_ld);
        int index = 0;
        Elf_Sym* addr_symtab = NULL;
        char* addr_strtab = NULL;
        do
        {
            pread((void*)((unsigned long)map.l_ld + index * sizeof(dyn)), &dyn, sizeof(dyn));
            index++;
            if(dyn.d_tag == DT_SYMTAB)
            {
                addr_symtab = (Elf_Sym*)dyn.d_un.d_val;
            }
            else if(dyn.d_tag == DT_STRTAB)
            {
                addr_strtab = (char*)dyn.d_un.d_val;
            }
        }
        while(dyn.d_tag != DT_NULL);

        for(int i = 0; i < 10000; i++)
        {
            pread(addr_symtab + i, &sym, sizeof(sym));
            if(sym.st_name == 0 && sym.st_value == 0 && sym.st_size == 0)
            {
                continue;
            }
            char* addr_name = addr_strtab + sym.st_name;
            std::string name = preadString(addr_name);
            LOG_D("sym name=%s,addr=%p", name.c_str(), (void*)(map.l_addr + sym.st_value));
            if(name.empty() == false && name[0] != '\0' && (name[0] < 32 || name[0] >= 127))
            {
                break;
            }
        }
        addr_link_map_tmp = map.l_next;
    }
    while(map.l_next != NULL);
}

bool PTrace_mips_64::replace(const char* func_name, const char* func_name_new)
{
    if(func_name == NULL || func_name_new == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    void* addr_plt = getGotAddr(func_name);
    if(addr_plt == NULL)
    {
        LOG_E("failed");
        return false;
    }

    Elf_Addr addr_new = (Elf_Addr)getSymbolAddr(func_name_new);

    return pwrite(addr_plt, &addr_new, sizeof(addr_new));
}

bool PTrace_mips_64::replace(const char* func_name, Elf_Addr addr_new)
{
    if(func_name == NULL || addr_new == 0)
    {
        LOG_E("arg incorrect");
        return false;
    }
    void* addr_plt = getGotAddr(func_name);
    if(addr_plt == NULL)
    {
        LOG_E("failed");
        return false;
    }

    return pwrite(addr_plt, &addr_new, sizeof(addr_new));
}

#endif

