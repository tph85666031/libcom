#ifndef __BCP_LOG_H__
#define __BCP_LOG_H__

#include "com_stack.h"

/*
本日志系统固定只加载可执行文件所在目录的log.ini配置文件
日志系统会自动搜索可执行文件的绝对路径
日志系统优先使用配置文件中组名与当前可执行文件文件名一致的配置，否则使用[default]对应的配置
若不存在log.ini文件则日志系统不生效
所有配置均在可执行文件重启后生效

[default] <----group_name，固定为可执行文件文件名
level=1,2,4,8,16   <----日志等级
console=true  <----是否在控制台输出
format="%^[%Y-%m-%d %H:%M:%S.%e] [%L] [%t] %v%$"  <----日志格式
file="./tbox.log"   <----日志存储文件名
file_err="./tbox_err.log"  <----Warning级别及其以上的日志存储文件名
file_size=2097152  <-----日志文件最大上限
file_count=3  <-----日志覆盖回滚个数
flush_interval=10 <-----日志刷写磁盘间隔(单位秒)

*/
#define LOG_LEVEL_DEBUG       0x0001
#define LOG_LEVEL_INFO        0x0002
#define LOG_LEVEL_WARNING     0x0004
#define LOG_LEVEL_ERROR       0x0008
#define LOG_LEVEL_FATAL       0x0010

#define LOG_OPTION_DEFAULT    (LOG_LEVEL_DEBUG \
                               | LOG_LEVEL_INFO  \
                               | LOG_LEVEL_WARNING \
                               | LOG_LEVEL_ERROR \
                               | LOG_LEVEL_FATAL)

typedef void (*log_hook_fc)(int level, const char* buf);

void com_log_init();
void com_log_uninit(void);
void com_log_set_hook(log_hook_fc hook_fc);
void com_log_output(int level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

#define LOG_D(fmt,...)    com_log_output(LOG_LEVEL_DEBUG,   "(%s:%s:%s:%d) - " fmt, MODULE_NAME, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_I(fmt,...)    com_log_output(LOG_LEVEL_INFO,    "(%s:%s:%s:%d) - " fmt, MODULE_NAME, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_W(fmt,...)    com_log_output(LOG_LEVEL_WARNING, "(%s:%s:%s:%d) - " fmt, MODULE_NAME, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_E(fmt,...)    com_log_output(LOG_LEVEL_ERROR,   "(%s:%s:%s:%d) - " fmt, MODULE_NAME, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_F(fmt,...)    do{com_log_output(LOG_LEVEL_FATAL,"(%s:%s:%s:%d) - " fmt, MODULE_NAME, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__);com_stack_print();}while(0)

class LogTimeCalc final
{
public:
    LogTimeCalc(const char* func_name, int line_number);
    ~LogTimeCalc();
private:
    std::string func_name;
    int line_number;
    uint64 time_cost_ns;
};

#if __linux__ == 1
#define TIME_COST() LogTimeCalc MACRO_CONCAT(__calc__,__LINE__)(__FUNC__,__LINE__)
#else
#define TIME_COST() LogTimeCalc __calc__(__FUNC__,__LINE__)
#endif

#endif /* __BCP_LOG_H__ */

