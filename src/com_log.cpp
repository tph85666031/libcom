#include "com_base.h"
#include "com_file.h"
#include "com_config.h"
#include "com_thread.h"
#include "com_log.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#define LOGGER_NAME      "default"

static std::atomic<log_hook_fc> log_hook = {NULL};
#define LOG_FORMAT "%^[%Y-%m-%d %H:%M:%S.%e] [%L] [%t] %v%$"

static spdlog::level::level_enum level_convert(int level)
{
    if(level == LOG_LEVEL_TRACE)
    {
        return spdlog::level::trace;
    }
    else if(level == LOG_LEVEL_DEBUG)
    {
        return spdlog::level::debug;
    }
    else if(level == LOG_LEVEL_INFO)
    {
        return spdlog::level::info;
    }
    else if(level == LOG_LEVEL_WARNING)
    {
        return spdlog::level::warn;
    }
    else if(level == LOG_LEVEL_ERROR)
    {
        return spdlog::level::err;
    }
    else if(level == LOG_LEVEL_FATAL)
    {
        return spdlog::level::critical;
    }
    else
    {
        return spdlog::level::off;
    }
}

void com_log_set_hook(log_hook_fc hook_fc)
{
    log_hook = hook_fc;
}

void com_log_init(const char* file, const char* file_err, bool console_enable,
                  int file_size_max, int file_count, int level, int flush_interval_s)
{
    std::vector<spdlog::sink_ptr> sink_list;

    if(console_enable)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(level_convert(level));
        console_sink->set_pattern(LOG_FORMAT);
        sink_list.push_back(console_sink);
    }

    if(file != NULL)
    {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, file_size_max, file_count);
        file_sink->set_level(level_convert(level));
        file_sink->set_pattern(LOG_FORMAT);
        sink_list.push_back(file_sink);
    }

    if(file_err != NULL)
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
    my_logger->set_level(level_convert(level));
    my_logger->flush_on(spdlog::level::warn);
    spdlog::flush_every(std::chrono::seconds(flush_interval_s));
    spdlog::drop(LOGGER_NAME);
    spdlog::register_logger(my_logger);
    spdlog::set_default_logger(my_logger);
    return;
}

void com_log_uninit(void)
{
    spdlog::drop_all();
}

void com_log_output(int level, const char* fmt, ...)
{
    auto my_logger = spdlog::default_logger();
    if(my_logger == NULL)
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

    if(level & LOG_LEVEL_TRACE)
    {
        my_logger->trace(str);
    }
    else if(level & LOG_LEVEL_DEBUG)
    {
        my_logger->debug(str);
    }
    else if(level & LOG_LEVEL_INFO)
    {
        my_logger->info(str);
    }
    else if(level & LOG_LEVEL_WARNING)
    {
        my_logger->warn(str);
    }
    else if(level & LOG_LEVEL_ERROR)
    {
        my_logger->error(str);
    }
    else if(level & LOG_LEVEL_FATAL)
    {
        my_logger->critical(str);
    }
    else
    {
    }

    log_hook_fc hook = log_hook;
    if(hook != NULL)
    {
        hook(level, str.c_str());
    }
}

LogTimeCalc::LogTimeCalc(const char* func_name, int line_number)
{
    this->line_number = line_number;
    if(func_name != NULL)
    {
        this->func_name = func_name;
    }
    time_cost_ns = com_time_cpu_ns();
}

LogTimeCalc::~LogTimeCalc()
{
    time_cost_ns = com_time_cpu_ns() - time_cost_ns;
    LOG_I("time cost for [%s:%d] is %.4f ms",
          func_name.c_str(), line_number,
          (double)time_cost_ns / (1000 * 1000));
}

