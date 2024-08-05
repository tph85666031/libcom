#ifndef __COM_LOG_H__
#define __COM_LOG_H__

#include "com_base.h"

#define LOG_LEVEL_TRACE       1
#define LOG_LEVEL_DEBUG       2
#define LOG_LEVEL_INFO        3
#define LOG_LEVEL_WARNING     4
#define LOG_LEVEL_ERROR       5
#define LOG_LEVEL_FATAL       6

#define LOG_LEVEL_DEFAULT     LOG_LEVEL_INFO

typedef void (*fp_log_callbcak)(int level, const char* message);

//全局日志系统
COM_EXPORT void com_log_init(const char* file = NULL, const char* file_err = NULL, bool console_enable = true, bool syslog_enable = false,
                             int file_size_max = 10 * 1024 * 1024, int file_count = 3, int level = LOG_LEVEL_DEFAULT, int flush_interval_s = 5);
COM_EXPORT void com_log_set_hook(fp_log_callbcak fc);
COM_EXPORT void com_log_shift_level();
COM_EXPORT void com_log_set_level(int level);
COM_EXPORT void com_log_set_level(const char* level);
COM_EXPORT int com_log_get_level();
COM_EXPORT std::string com_log_get_level_string();
COM_EXPORT void com_log_uninit(void);
COM_EXPORT void com_log_output(int level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

//自定义日志
COM_EXPORT void com_logger_create(const char* name, const char* file, bool console_enable = false,
                                  int file_size_max = 10 * 1024 * 1024, int file_count = 3, int level = LOG_LEVEL_TRACE, int flush_interval_s = 5);
COM_EXPORT void com_logger_destroy(const char* name);
COM_EXPORT void com_logger_set_level(const char* name, int level);
COM_EXPORT void com_logger_set_level(const char* name, const char* level);
COM_EXPORT void com_logger_set_pattern(const char* name, const char* pattern);
COM_EXPORT void com_logger_log(const char* name, int level, const char* fmt, ...);

#define LOG_T(fmt,...)    com_log_output(LOG_LEVEL_TRACE,   "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_D(fmt,...)    com_log_output(LOG_LEVEL_DEBUG,   "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_I(fmt,...)    com_log_output(LOG_LEVEL_INFO,    "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_W(fmt,...)    com_log_output(LOG_LEVEL_WARNING, "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_E(fmt,...)    com_log_output(LOG_LEVEL_ERROR,   "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_F(fmt,...)    com_log_output(LOG_LEVEL_FATAL,   "(%s:%s:%d) - " fmt, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

class COM_EXPORT LogTimeCalc final
{
public:
    LogTimeCalc(const char* file_name, int line_number);
    ~LogTimeCalc();
    void show(int line_number);
    void printTimeCost(const char* log_message);
private:
    std::string file_name;
    int line_number;
    uint64 time_cost_ns;
};

#define TIME_COST()      LogTimeCalc __calc__(__FILENAME__,__LINE__)
#define TIME_COST_SHOW() __calc__.show(__LINE__)
#define PRINT_TIME_COST(msg) __calc__.printTimeCost(msg)

#endif /* __COM_LOG_H__ */

