#ifndef __COM_LOG_H__
#define __COM_LOG_H__

#include "com_stack.h"

#define LOG_LEVEL_TRACE       0x0000
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

void com_log_init(const char* file, const char* file_err, bool console_enable,
                  int file_size_max = 10 * 1024 * 1024, int file_count = 3, int level = LOG_LEVEL_INFO, int flush_interval_s = 5);
void com_log_uninit(void);
void com_log_use_log4xx();
void com_log_set_hook(log_hook_fc hook_fc);
void com_log_output(int level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

#define LOG_T(fmt,...)    com_log_output(LOG_LEVEL_TRACE,   "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_D(fmt,...)    com_log_output(LOG_LEVEL_DEBUG,   "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_I(fmt,...)    com_log_output(LOG_LEVEL_INFO,    "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_W(fmt,...)    com_log_output(LOG_LEVEL_WARNING, "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_E(fmt,...)    com_log_output(LOG_LEVEL_ERROR,   "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_F(fmt,...)    do{com_log_output(LOG_LEVEL_FATAL,"(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__);com_stack_print();}while(0)

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

#endif /* __COM_LOG_H__ */

