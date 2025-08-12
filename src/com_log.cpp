#include "com_base.h"
#include "com_file.h"
#include "com_config.h"
#include "com_thread.h"
#include "com_stack.h"
#include "com_log.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#if __linux__ == 1
#include "spdlog/sinks/syslog_sink.h"
#endif

#define LOGGER_NAME  "default"
#define LOG_FORMAT   "%^[%Y-%m-%d %H:%M:%S.%e] [%L] [%t] %v%$"

static std::atomic<int> log_level = {LOG_LEVEL_DEFAULT};
static std::atomic<fp_log_callbcak> fc_log = {NULL};

void com_log_init(const char* file, const char* file_err, bool console_enable, bool syslog_enable,
                  int file_size_max, int file_count, int level, int flush_interval_s)
{
    std::vector<spdlog::sink_ptr> sink_list;

    if(console_enable)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern(LOG_FORMAT);
        sink_list.push_back(console_sink);
    }

#if __linux__ == 1
    if(syslog_enable)
    {
        auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(com_get_bin_name(), 0, LOG_USER, false);
        syslog_sink->set_level(spdlog::level::trace);
        syslog_sink->set_pattern(LOG_FORMAT);
        sink_list.push_back(syslog_sink);
    }
#endif

    if(file != NULL && file[0] != '\0')
    {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, file_size_max, file_count);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern(LOG_FORMAT);
        sink_list.push_back(file_sink);
    }

    if(file_err != NULL && file_err[0] != '\0')
    {
        auto file_sink_err = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file_err, file_size_max, file_count);
        file_sink_err->set_level(spdlog::level::warn);
        file_sink_err->set_pattern(LOG_FORMAT);
        sink_list.push_back(file_sink_err);
    }

    if(sink_list.empty())
    {
        spdlog::set_default_logger(NULL);
        return;
    }

    std::shared_ptr<spdlog::logger> my_logger = std::make_shared<spdlog::logger>(LOGGER_NAME, begin(sink_list), end(sink_list));
    my_logger->set_level(spdlog::level::trace);
    my_logger->flush_on(spdlog::level::warn);
    spdlog::flush_every(std::chrono::seconds(flush_interval_s));
    spdlog::drop(LOGGER_NAME);
    spdlog::register_logger(my_logger);
    spdlog::set_default_logger(my_logger);
    com_log_set_level(level);
    return;
}

void com_log_set_hook(fp_log_callbcak fc)
{
    fc_log = fc;
}

void com_log_shift_level()
{
    log_level--;
    if(log_level < LOG_LEVEL_TRACE)
    {
        log_level = LOG_LEVEL_FATAL;
    }
    auto my_logger = spdlog::default_logger();
    if(my_logger != NULL)
    {
        my_logger->warn("log level set to {}", log_level);
    }
}

void com_log_set_level(int level)
{
    log_level = level;
}

void com_log_set_level(const char* level)
{
    if(level == NULL)
    {
        return;
    }
    std::string level_str = level;
    com_string_to_upper(level_str);
    if(level_str == "TRACE")
    {
        log_level = LOG_LEVEL_TRACE;
    }
    else if(level_str == "DEBUG")
    {
        log_level = LOG_LEVEL_DEBUG;
    }
    else if(level_str == "INFO")
    {
        log_level = LOG_LEVEL_INFO;
    }
    else if(level_str == "WARN")
    {
        log_level = LOG_LEVEL_WARNING;
    }
    else if(level_str == "ERROR")
    {
        log_level = LOG_LEVEL_ERROR;
    }
    else if(level_str == "FATAL")
    {
        log_level = LOG_LEVEL_FATAL;
    }
    return;
}

int com_log_get_level()
{
    return log_level;
}

std::string com_log_get_level_string()
{
    if(log_level == LOG_LEVEL_TRACE)
    {
        return "TRACE";
    }
    if(log_level == LOG_LEVEL_DEBUG)
    {
        return "DEBUG";
    }
    if(log_level == LOG_LEVEL_INFO)
    {
        return "INFO";
    }
    if(log_level == LOG_LEVEL_WARNING)
    {
        return "WARN";
    }
    if(log_level == LOG_LEVEL_ERROR)
    {
        return "ERROR";
    }
    if(log_level == LOG_LEVEL_FATAL)
    {
        return "FATAL";
    }
    return "INFO";
}

void com_log_uninit(void)
{
    spdlog::drop_all();
}

void com_log_output(int level, const char* fmt, ...)
{
    if(fmt == NULL || level < log_level)
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

    fp_log_callbcak fc = fc_log.load();
    if(fc != NULL)
    {
        fc(level, str.c_str());
        return;
    }

    auto my_logger = spdlog::default_logger();
    if(my_logger == NULL)
    {
        return;
    }
    if(level == LOG_LEVEL_TRACE)
    {
        my_logger->trace(str);
    }
    else if(level == LOG_LEVEL_DEBUG)
    {
        my_logger->debug(str);
    }
    else if(level == LOG_LEVEL_INFO)
    {
        my_logger->info(str);
    }
    else if(level == LOG_LEVEL_WARNING)
    {
        my_logger->warn(str);
    }
    else if(level == LOG_LEVEL_ERROR)
    {
        my_logger->error(str);
    }
    else if(level == LOG_LEVEL_FATAL)
    {
        my_logger->critical(str);
        my_logger->critical(com_stack_get().c_str());
    }
}

void com_logger_create(const char* name, const char* file, bool console_enable,
                       int file_size_max, int file_count, int level, int flush_interval_s)
{
    if(name == NULL || name[0] == '\0')
    {
        return;
    }

    std::vector<spdlog::sink_ptr> sink_list;

    if(console_enable)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%v");
        sink_list.push_back(console_sink);
    }

    if(file != NULL && file[0] != '\0')
    {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, file_size_max, file_count);
        file_sink->set_pattern("%v");
        sink_list.push_back(file_sink);
    }

    if(sink_list.empty())
    {
        return;
    }

    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(name, begin(sink_list), end(sink_list));
    logger->flush_on(spdlog::level::warn);
    spdlog::flush_every(std::chrono::seconds(flush_interval_s));
    spdlog::drop(name);
    spdlog::register_logger(logger);
    com_logger_set_level(name, level);
    return;
}

void com_logger_destroy(const char* name)
{
    if(name == NULL)
    {
        return;
    }
    spdlog::drop(name);
}

void com_logger_set_level(const char* name, int level)
{
    if(name == NULL)
    {
        return;
    }
    auto logger = spdlog::get(name);
    if(logger == NULL)
    {
        return;
    }
    if(level == LOG_LEVEL_TRACE)
    {
        logger->set_level(spdlog::level::trace);
    }
    else if(level == LOG_LEVEL_DEBUG)
    {
        logger->set_level(spdlog::level::debug);
    }
    else if(level == LOG_LEVEL_INFO)
    {
        logger->set_level(spdlog::level::info);
    }
    else if(level == LOG_LEVEL_WARNING)
    {
        logger->set_level(spdlog::level::warn);
    }
    else if(level == LOG_LEVEL_ERROR)
    {
        logger->set_level(spdlog::level::err);
    }
    else if(level == LOG_LEVEL_FATAL)
    {
        logger->set_level(spdlog::level::critical);
    }
    return;
}

void com_logger_set_level(const char* name, const char* level)
{
    if(name == NULL || level == NULL)
    {
        return;
    }
    auto logger = spdlog::get(name);
    if(logger == NULL)
    {
        return;
    }
    if(com_string_equal_ignore_case(level, "trace"))
    {
        logger->set_level(spdlog::level::trace);
    }
    else if(com_string_equal_ignore_case(level, "debug"))
    {
        logger->set_level(spdlog::level::debug);
    }
    else if(com_string_equal_ignore_case(level, "info"))
    {
        logger->set_level(spdlog::level::info);
    }
    else if(com_string_equal_ignore_case(level, "warning"))
    {
        logger->set_level(spdlog::level::warn);
    }
    else if(com_string_equal_ignore_case(level, "error"))
    {
        logger->set_level(spdlog::level::err);
    }
    else if(com_string_equal_ignore_case(level, "critical"))
    {
        logger->set_level(spdlog::level::critical);
    }
    return;
}

void com_logger_set_pattern(const char* name, const char* pattern)
{
    if(name == NULL || pattern == NULL)
    {
        return;
    }
    auto logger = spdlog::get(name);
    if(logger == NULL)
    {
        return;
    }
    logger->set_pattern(pattern);
    return;
}

void com_logger_log(const char* name, int level, const char* fmt, ...)
{
    if(name == NULL || fmt == NULL)
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

    auto logger = spdlog::get(name);
    if(logger == NULL)
    {
        return;
    }
    if(level == LOG_LEVEL_TRACE)
    {
        logger->trace(str);
    }
    else if(level == LOG_LEVEL_DEBUG)
    {
        logger->debug(str);
    }
    else if(level == LOG_LEVEL_INFO)
    {
        logger->info(str);
    }
    else if(level == LOG_LEVEL_WARNING)
    {
        logger->warn(str);
    }
    else if(level == LOG_LEVEL_ERROR)
    {
        logger->error(str);
    }
    else if(level == LOG_LEVEL_FATAL)
    {
        logger->critical(str);
    }
}

LogTimeCalc::LogTimeCalc(const char* file_name, int line_number)
{
    this->line_number = line_number;
    if(file_name != NULL)
    {
        this->file_name = file_name;
    }
    time_cost_ns = com_time_cpu_ns();
}

LogTimeCalc::~LogTimeCalc()
{
    com_log_output(LOG_LEVEL_WARNING, "TIME DIFF %s[%d<---%.4fms--->RETUN]",
                   file_name.c_str(), this->line_number,
                   (double)(com_time_cpu_ns() - time_cost_ns) / (1000 * 1000));
}

void LogTimeCalc::show(int line_number)
{
    com_log_output(LOG_LEVEL_WARNING, "TIME DIFF %s[%d<---%.4fms--->%d]",
                   file_name.c_str(), this->line_number,
                   (double)(com_time_cpu_ns() - time_cost_ns) / (1000 * 1000), line_number);
    this->line_number = line_number;
    time_cost_ns = com_time_cpu_ns();
}

void LogTimeCalc::printTimeCost(const char* log_message)
{
    uint64 cost = com_time_cpu_ns() - this->time_cost_ns;
    com_log_output(LOG_LEVEL_WARNING, "%s, time cost: %.4f ms",
                   log_message, (double)cost / (1000 * 1000));
}

