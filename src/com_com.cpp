#include <cctype>
#include <algorithm>
#if defined(_WIN32) || defined(_WIN64)
#include <time.h>
#else
#include <fnmatch.h>
#include <unistd.h> //usleep readlink
#include <sys/time.h>
#include <netdb.h> //gethostbyname
#endif

#include "CJsonObject.h"
#include "com_com.h"
#include "com_md5.h"
#include "com_file.h"
#include "com_thread.h"
#include "com_log.h"

#define FILE_COM_CONFIG  "com.ini"

std::string com_com_search_config_file()
{
    std::vector<std::string> config_files;

    const char* bin_path = com_get_bin_path();
    const char* bin_name = com_get_bin_name();

    config_files.push_back(com_string_format("./config/%s_com.ini", bin_name));
    config_files.push_back(com_string_format("./config/%s", FILE_COM_CONFIG));
    config_files.push_back(com_string_format("./%s_com.ini", bin_name));
    config_files.push_back(com_string_format("./%s", FILE_COM_CONFIG));
    config_files.push_back(com_string_format("%s/config/%s_com.ini", bin_path, bin_name));
    config_files.push_back(com_string_format("%s/config/%s", bin_path, FILE_COM_CONFIG));
    config_files.push_back(com_string_format("%s/%s_com.ini", bin_path, bin_name));
    config_files.push_back(com_string_format("%s/%s", bin_path, FILE_COM_CONFIG));
    config_files.push_back(com_string_format("./tmp/%s/depend/%s", HOST_TYPE, FILE_COM_CONFIG));

    for (int i = 0; i < config_files.size(); i++)
    {
        if (com_file_type(config_files[i].c_str()) == FILE_TYPE_FILE)
        {
            //printf("Search COM Config File: %s ... FOUND\n", config_files[i].c_str());
            return config_files[i];
        }
        //printf("Search COM Config File: %s ... NOT FOUND, try next one\n", config_files[i].c_str());
    }
    return std::string();
}

bool com_run_shell(const char* fmt, ...)
{
#if __linux__ == 1
    std::string cmd;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len <= 0)
    {
        return false;
    }
    len += 1;  //上面返回的长度不包含\0，这里加上
    std::vector<char> cmd_buf(len);
    va_start(args, fmt);
    len = vsnprintf(cmd_buf.data(), len, fmt, args);
    va_end(args);
    if (len <= 0)
    {
        return false;
    }
    cmd.assign(cmd_buf.data(), len);
    system(cmd.c_str());
    return true;
#else
    return false;
#endif
}

std::string com_run_shell_with_output(const char* fmt, ...)
{
    std::string result;
#if __linux__ == 1
    if (fmt == NULL)
    {
        //LOG_E("failed");
        return result;
    }
    std::string cmd;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len <= 0)
    {
        return result;
    }
    len += 1;  //上面返回的长度不包含\0，这里加上
    std::vector<char> cmd_buf(len);
    va_start(args, fmt);
    len = vsnprintf(cmd_buf.data(), len, fmt, args);
    va_end(args);
    if (len <= 0)
    {
        return result;
    }
    cmd.assign(cmd_buf.data(), len);

    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == NULL)
    {
        //LOG_E("failed, cmd=%s", cmd.c_str());
        return result;
    }
    char* output_buf = new char[8192]();
    if (fgets(output_buf, 8192, fp) == NULL)
    {
        delete[] output_buf;
        //("failed, cmd=%s", cmd.c_str());
        return result;
    }
    pclose(fp);
    output_buf[8191] = '\0';
    //LOG_D("run shell succeed, cmd=%s", cmd.c_str());
    result.assign(output_buf);
    delete[] output_buf;
#endif
    return result;
}

std::vector<std::string> com_string_split(const char* str, const char* delim)
{
    std::vector<std::string> vals;
    if (str != NULL && delim != NULL)
    {
        std::string orgin = str;
        int delim_len = strlen(delim);
        int pos = 0;
        int pos_pre = 0;
        while (true)
        {
            pos = orgin.find_first_of(delim, pos_pre);
            if (pos == std::string::npos)
            {
                vals.push_back(orgin.substr(pos_pre));
                break;
            }
            vals.push_back(orgin.substr(pos_pre, pos - pos_pre));
            pos_pre = pos + delim_len;
        }
    }
    return vals;
}

char* com_strncpy(char* dst, const char* src, int dst_size)
{
    if (dst == NULL || src == NULL || dst_size <= 0)
    {
        return NULL;
    }
    strncpy(dst, src, dst_size);
    dst[dst_size - 1] = '\0';
    return dst;
}

bool com_strncmp_ignore_case(const char* str1, const char* str2, int size)
{
    if (str1 == NULL || str2 == NULL || size <= 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (_strnicmp(str1, str2, size) == 0)
#else
    if (strncasecmp(str1, str2, size) == 0)
#endif
    {
        return true;
    }
    return false;
}

bool com_strncmp(const char* str1, const char* str2, int size)
{
    if (str1 == NULL || str2 == NULL || size <= 0)
    {
        return false;
    }
    if (strncmp(str1, str2, size) == 0)
    {
        return true;
    }
    return false;
}

bool com_string_equal(const char* a, const char* b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }
    if (strcmp(a, b) == 0)
    {
        return true;
    }
    return false;
}

bool com_string_equal_ignore_case(const char* a, const char* b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (_stricmp(a, b) == 0)
#else
    if (strcasecmp(a, b) == 0)
#endif
    {
        return true;
    }
    return false;
}

std::string& com_string_trim_left(std::string& str, const char* t)
{
    if (t == NULL)
    {
        t = " \t\n\r\f\v";
    }
    str.erase(0, str.find_first_not_of(t));
    return str;
}

std::string& com_string_trim_right(std::string& str, const char* t)
{
    if (t == NULL)
    {
        t = " \t\n\r\f\v";
    }
    str.erase(str.find_last_not_of(t) + 1);
    return str;
}

std::string& com_string_trim(std::string& str, const char* t)
{
    return com_string_trim_left(com_string_trim_right(str, t), t);
}

char* com_string_trim_left(char* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    char* p1 = str;
    char* p2 = str;
    while (isspace(*p1))
    {
        p1++;
    }
    while (*p1 != '\0')
    {
        *p2 = *p1;
        p1++;
        p2++;
    }
    *p2 = '\0';
    return str;
}

char* com_string_trim_right(char* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    int len = strlen(str);
    int i = 0;
    for (i = len - 1; i >= 0; i--)
    {
        if (isspace(str[i]) == false)
        {
            break;
        }
    }
    str[i + 1] = '\0';
    return str;
}

char* com_string_trim(char* str)
{
    com_string_trim_right(str);
    com_string_trim_left(str);
    return str;
}

bool com_string_start_with(const char* str, const char* prefix)
{
    if (str == NULL || prefix == NULL)
    {
        return false;
    }
    if (strncmp(str, prefix, strlen(prefix)) == 0)
    {
        return true;
    }
    return false;
}

bool com_string_end_with(const char* str, const char* tail)
{
    if (str == NULL || tail == NULL)
    {
        return false;
    }
    int l1 = com_string_len(str);
    int l2 = com_string_len(tail);
    if (l1 >= l2)
    {
        if (com_string_equal(str + l1 - l2, tail))
        {
            return true;
        }
    }
    return false;
}

bool com_string_contain(const char* str, const char* sub)
{
    if (str == NULL || sub == NULL)
    {
        return false;
    }
    if (strstr(str, sub))
    {
        return true;
    }
    return false;
}

std::string& com_string_to_upper(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string& com_string_to_lower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

char* com_string_to_upper(char* str)
{
    if (str == NULL)
    {
        return str;
    }
    char* orignal = str;
    for (; *str != '\0'; str++)
    {
        *str = toupper(*str);
    }
    return orignal;
}

char* com_string_to_lower(char* str)
{
    if (str == NULL)
    {
        return str;
    }
    char* orignal = str;
    for (; *str != '\0'; str++)
    {
        *str = tolower(*str);
    }
    return orignal;
}

std::string com_string_format(const char* fmt, ...)
{
    std::string str;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len > 0)
    {
        len += 1;  //上面返回的长度不包含\0，这里加上
        std::vector<char> buf(len);
        va_start(args, fmt);
        len = vsnprintf(buf.data(), len, fmt, args);
        va_end(args);
        if (len > 0)
        {
            str.assign(buf.data(), len);
        }
    }
    return str;
}

/* 匹配 * ？通配符 */
bool com_string_match(const char* str, const char* pattern)
{
#if __linux__ == 0
    return com_string_equal(str, pattern);
#else
    return (fnmatch(pattern, str, 0) == 0);
#endif
}

std::string com_bytes_to_hexstring(const uint8* data, uint16 size)
{
    char buf[8];
    std::string result;
    if (data != NULL && size > 0)
    {
        for (int i = 0; i < size; i++)
        {
            snprintf(buf, sizeof(buf), "%02x", data[i]);
            result.append(buf, 2);
        }
    }
    return result;
}

ByteArray com_hexstring_to_bytes(const char* str)
{
    ByteArray bytes;
    if (str == NULL)
    {
        return bytes;
    }
    std::string val = str;
    com_string_replace(val, " ", "");

    char tmp[4];
    memset(tmp, 0, sizeof(tmp));
    for (int i = 0; i < val.length(); i = i + 2)
    {
        memcpy(tmp, val.data() + i, 2);
        bytes.append(strtoul(tmp, NULL, 16));
    }

    return bytes;
}

int com_hexstring_to_bytes(const char* str, unsigned char* bytes, int size)
{
    char* strEnd;
    int m = 0;
    int len = strlen(str) / 2;
    int i = 0;
    char tmp[4] = {0};
    if (size < len)
    {
        return -1;
    }
    for (i = 0; i < len; i++)
    {
        m = i * 2;
        memcpy(tmp, str + m, 2);
        bytes[i] = strtol(tmp, &strEnd, 16);
    }
    return i;
}

bool com_string_replace(char* str, char from, char to)
{
    int len = com_string_len(str);
    if (len <= 0)
    {
        return false;
    }
    for (int i = 0; i < len; i++)
    {
        if (str[i] == from)
        {
            str[i] = to;
        }
    }
    return true;
}

bool com_string_replace(std::string& str, const char* from, const char* to)
{
    if (from == NULL || to == NULL)
    {
        return false;
    }

    int pos = 0;
    int from_len = com_string_len(from);
    int to_len = com_string_len(to);
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from_len, to);
        pos += to_len;
    }
    return true;
}

int com_string_len(const char* str)
{
    if (str == NULL)
    {
        return 0;
    }
    return strlen(str);
}

int com_string_size(const char* str)
{
    if (str == NULL)
    {
        return 0;
    }
    return com_string_len(str) + 1;
}

bool com_string_is_ip(const char* ip)
{
    if (ip == NULL)
    {
        return false;
    }
    int ip_val[4];
    if (sscanf(ip, "%d.%d.%d.%d", ip_val, ip_val + 1, ip_val + 2, ip_val + 3) != 4)
    {
        return false;
    }

    for (int i = 0; i < 4; i++)
    {
        if (ip_val[i] < 0 || ip_val[i] > 255)
        {
            return false;
        }
    }
    return true;
}

//返回值为已写入buf的长度，不包括末尾的\0
int com_snprintf(char* buf, int buf_size, const char* fmt, ...)
{
    if (buf == NULL || buf_size <= 0 || fmt == NULL)
    {
        return 0;
    }
    va_list list;
    va_start(list, fmt);
    int ret = vsnprintf(buf, buf_size, fmt, list);
    va_end(list);
    if (ret < 0)
    {
        ret = 0;
    }
    else if (ret >= buf_size)
    {
        ret = buf_size - 1;
    }
    buf[buf_size - 1] = '\0';
    return ret;
}

void com_sleep_ms(uint32 val)
{
    if (val == 0)
    {
        return;
    }
#if defined(_WIN32) || defined(_WIN64)
    Sleep(val);
#else
#if 1
    usleep((uint64)val * 1000);
#else
    struct timeval time;
    time.tv_sec = val / 1000;//seconds
    time.tv_usec = val % 1000 * 1000;//microsecond
    select(0, NULL, NULL, NULL, &time);
#endif
#endif
}

void com_sleep_s(uint32 val)
{
    com_sleep_ms((uint64)val * 1000);
}

uint32 com_rand(uint32 min, uint32 max)
{
    static uint32 seed = 0;
    srand(com_time_cpu_ms() + (seed++));
    return (uint32)(rand() % ((uint64)max - min + 1) + min);
}

bool com_gettimeofday(struct timeval* tp)
{
    if (tp == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    time_t clock;
    struct tm tm = {0};
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year   = wtm.wYear - 1900;
    tm.tm_mon   = wtm.wMonth - 1;
    tm.tm_mday   = wtm.wDay;
    tm.tm_hour   = wtm.wHour;
    tm.tm_min   = wtm.wMinute;
    tm.tm_sec   = wtm.wSecond;
    tm.tm_isdst  = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return true;
#else
    return gettimeofday(tp, NULL) == 0;
#endif
}

uint32 com_time_cpu_s()
{
#if defined(_WIN32) || defined(_WIN64)
    return GetTickCount() / 1000;
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return tss.tv_sec;
#endif
}

uint64 com_time_cpu_ms()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetTickCount();
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return (tss.tv_sec * 1000ULL + tss.tv_nsec / 1000000);
#endif
}

uint64 com_time_cpu_us()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetTickCount() * 1000;
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return (tss.tv_sec * 1000 * 1000ULL + tss.tv_nsec / 1000);
#endif
}

uint64 com_time_cpu_ns()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetTickCount() * 1000 * 1000;
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return (tss.tv_sec * 1000 * 1000 * 1000ULL + tss.tv_nsec);
#endif
}

//UTC seconds form 1970-1-1 0-0-0
uint32 com_time_rtc_s()
{
    struct timeval tv;
    com_gettimeofday(&tv);
    return tv.tv_sec;
}

//UTC milliseconds form 1970-1-1 0-0-0
uint64 com_time_rtc_ms()
{
    struct timeval tv;
    com_gettimeofday(&tv);
    return (tv.tv_sec * 1000ULL + tv.tv_usec / 1000);
}

//UTC macroseconds form 1970-1-1 0-0-0
uint64 com_time_rtc_us()
{
    struct timeval tv;
    com_gettimeofday(&tv);
    return (tv.tv_sec * 1000 * 1000ULL + tv.tv_usec);
}

uint64 com_time_diff_ms(uint64 start, uint64 end)
{
    if (end < start)
    {
        LOG_W("incorrect timestamp, start=%llu, end=%llu", start, end);
        return 0;
    }
    return end - start;
}

//获取与utc时间的秒速差值
int32 com_timezone_get_s()
{
    time_t t1, t2;
    time(&t1);
    t2 = t1;
    int32 timezone_s = 0;
    struct tm* tm_local = NULL;
    struct tm* tm_utc = NULL;
    tm_local = localtime(&t1);
    t1 = mktime(tm_local) ;
    tm_utc = gmtime(&t2);
    t2 = mktime(tm_utc);
    timezone_s = (t1 - t2);
    return timezone_s;
}

int32 com_timezone_china_s()
{
    return 8 * 3600;
}

/* UTC
%a 星期几的简写
%A 星期几的全称
%b 月份的简写
%B 月份的全称
%c 标准的日期的时间串
%C 年份的前两位数字
%d 十进制表示的每月的第几天
%D 月/天/年
%e 在两字符域中，十进制表示的每月的第几天
%F 年-月-日
%g 年份的后两位数字，使用基于周的年
%G 年份，使用基于周的年
%h 简写的月份名
%H 24小时制的小时
%I 12小时制的小时
%j 十进制表示的每年的第几天
%m 十进制表示的月份
%M 十时制表示的分钟数
%n 新行符
%p 本地的AM或PM的等价显示
%r 12小时的时间
%R 显示小时和分钟：hh:mm
%S 十进制的秒数
%t 水平制表符
%T 显示时分秒：hh:mm:ss
%u 每周的第几天，星期一为第一天 （值从1到7，星期一为1）
%U 第年的第几周，把星期日作为第一天（值从0到53）
%V 每年的第几周，使用基于周的年
%w 十进制表示的星期几（值从0到6，星期天为0）
%W 每年的第几周，把星期一做为第一天（值从0到53）
%x 标准的日期串
%X 标准的时间串
%y 不带世纪的十进制年份（值从0到99）
%Y 带世纪部分的十制年份
%z，%Z 时区名称，如果不能得到时区名称则返回空字符
%% 百分号
*/
std::string com_time_to_string(uint32 time_s, const char* format)
{
    std::string str;
    if (format == NULL)
    {
        return str;
    }
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(struct tm));
    if (com_time_to_tm(time_s, &tm_val) == false)
    {
        return str;
    }
    char buf[128];
    memset(buf, 0, sizeof(buf));
    if (strftime(buf, sizeof(buf), format, &tm_val) > 0)
    {
        str = buf;
    }
    return str;
}

std::string com_time_to_string(uint32 time_s)
{
    return com_time_to_string(time_s, "%Y-%m-%d %H:%M:%S");
}

//UTC
bool com_time_to_tm(uint32 time_s,  struct tm* tm_val)
{
    if (tm_val == NULL)
    {
        return false;
    }
    time_t val = time_s;
#if defined(_WIN32) || defined(_WIN64)
    if (gmtime_s(tm_val, &val) != 0)
#else
    if (gmtime_r(&val, tm_val) == NULL)
#endif
    {
        return false;
    }
    return true;
}

//UTC
bool com_time_to_bcd(uint32 time_s, uint8 bcd_time[6])
{
    if (bcd_time == NULL)
    {
        return false;
    }
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(struct tm));
    if (com_time_to_tm(time_s, &tm_val) == false)
    {
        return false;
    }
    return com_tm_to_bcd_date(&tm_val, bcd_time);
}

uint32 com_time_from_bcd(uint8 bcd_date[6])
{
    if (bcd_date == NULL)
    {
        return 0;
    }
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(struct tm));
    tm_val.tm_year = 2000 + com_bcd_to_uint8(bcd_date[0]) - 1900;
    tm_val.tm_mon = com_bcd_to_uint8(bcd_date[1]) - 1;
    tm_val.tm_mday = com_bcd_to_uint8(bcd_date[2]);

    tm_val.tm_hour = com_bcd_to_uint8(bcd_date[3]);
    tm_val.tm_min = com_bcd_to_uint8(bcd_date[4]);
    tm_val.tm_sec = com_bcd_to_uint8(bcd_date[5]);

    return com_tm_to_time(&tm_val);
}

//UTC
uint32 com_tm_to_time(struct tm* tm_val)
{
    if (tm_val == NULL)
    {
        return 0;
    }
#if defined(_WIN32) || defined(_WIN64)
#else
    tm_val->tm_gmtoff = 0;
    tm_val->tm_zone = NULL;
#endif
    //mktime默认会扣除当前系统配置的时区秒数,由于我们传入的参数默认为UTC时间，因此需要再加上时区秒数
    return (uint32)(mktime(tm_val) + com_timezone_get_s());
}

//UTC
bool com_tm_to_bcd_date(struct tm* tm_val, uint8 bcd_date[6])
{
    if (tm_val == NULL || bcd_date == NULL)
    {
        return false;
    }
    if (tm_val->tm_year > 100)
    {
        bcd_date[0] = com_uint8_to_bcd(tm_val->tm_year - 100);
    }
    else
    {
        bcd_date[0] = 100 + tm_val->tm_year;
    }
    bcd_date[1] = com_uint8_to_bcd(tm_val->tm_mon + 1);
    bcd_date[2] = com_uint8_to_bcd(tm_val->tm_mday);
    bcd_date[3] = com_uint8_to_bcd(tm_val->tm_hour);
    bcd_date[4] = com_uint8_to_bcd(tm_val->tm_min);
    bcd_date[5] = com_uint8_to_bcd(tm_val->tm_sec);
    return true;
}

/* UTC date_str格式为 yyyy-mm-dd HH:MM:SS */
uint32 com_string_to_time(const char* date_str)
{
    if (date_str == NULL)
    {
        return 0;
    }
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(struct tm));
    int count = sscanf(date_str, "%d-%d-%d %d:%d:%d",
                       &tm_val.tm_year, &tm_val.tm_mon, &tm_val.tm_mday,
                       &tm_val.tm_hour, &tm_val.tm_min, &tm_val.tm_sec);
    if (count != 6)
    {
        return false;
    }
    tm_val.tm_year -= 1900;
    tm_val.tm_mon -= 1;
    return com_tm_to_time(&tm_val);
}

uint8 com_uint8_to_bcd(uint8 v)
{
    return ((v / 10) << 4 | v % 10);
}

uint8 com_bcd_to_uint8(uint8 bcd)
{
    uint8 tmp = 0;
    tmp = ((bcd & 0xF0) >> 0x4) * 10;
    return (tmp + (bcd & 0x0F));
}

bool com_sem_init(Sem* sem, const char* name)
{
    if (sem == NULL)
    {
        return false;
    }
    if (name == NULL)
    {
        name = "Unknown";
    }
    sem->name = name;
#if defined(_WIN32) || defined(_WIN64)
    sem->handle = CreateEvent(
                      NULL,               // default security attributes
                      FALSE,              // manual-reset event?
                      FALSE,              // initial state is nonsignaled
                      NULL                // object name
                  );
#else
    if (sem_init(&sem->handle, 0, 0) != 0)
    {
        LOG_E("failed");
        return false;
    }
#endif
    return true;
}

bool com_sem_uninit(Sem* sem)
{
    if (sem == NULL)
    {
        LOG_E("failed");
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (sem->handle == NULL)
    {
        LOG_E("failed");
        return false;
    }
    CloseHandle(sem->handle);
#else
    sem_destroy(&sem->handle);
#endif
    return true;
}

Sem* com_sem_create(const char* name)
{
    if (name == NULL)
    {
        name = "Unknown";
    }
    Sem* sem = new Sem();
    sem->name = name;
#if defined(_WIN32) || defined(_WIN64)
    sem->handle = CreateEvent(
                      NULL,               // default security attributes
                      FALSE,              // manual-reset event?
                      FALSE,              // initial state is nonsignaled
                      NULL                // object name
                  );
#else
    if (sem_init(&sem->handle, 0, 0) != 0)
    {
        LOG_E("failed");
        delete sem;
        return NULL;
    }
#endif
    return sem;
}

bool com_sem_post(Sem* sem)
{
    if (sem == NULL)
    {
        LOG_E("failed");
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (sem->handle == NULL)
    {
        return false;
    }
    return SetEvent(sem->handle);
#else
    if (sem_post(&sem->handle) != 0)
    {
        LOG_E("failed");
        return false;
    }
    return true;
#endif
}

bool com_sem_wait(Sem* sem, int timeout_ms)
{
    if (sem == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (sem->handle == NULL)
    {
        LOG_E("failed");
        return false;
    }
    if (timeout_ms <= 0)
    {
        timeout_ms = INFINITE;
    }
    int rc = WaitForSingleObject(sem->handle, timeout_ms);
    if (rc != WAIT_OBJECT_0)
    {
        LOG_E("failed");
        return false;
    }
#else
    if (timeout_ms > 0)
    {
        struct timespec ts;
        memset(&ts, 0, sizeof(struct timespec));

        clock_gettime(CLOCK_REALTIME, &ts);
        uint64 tmp = ts.tv_nsec + (int64)timeout_ms * 1000 * 1000;
        ts.tv_nsec = tmp % (1000 * 1000 * 1000);
        ts.tv_sec += tmp / (1000 * 1000 * 1000);

        int ret = sem_timedwait(&sem->handle, &ts);
        if (ret != 0)
        {
            if (errno == ETIMEDOUT)
            {
                //LOG_W("timeout:%s:%d", sem->name.c_str(), timeout_ms);
            }
            else
            {
                LOG_E("failed,ret=%d,errno=%d", ret, errno);
            }
            return false;
        }
    }
    else
    {
        if (sem_wait(&sem->handle) != 0)
        {
            LOG_E("failed");
            return false;
        }
    }
#endif
    return true;
}

bool com_sem_destroy(Sem* sem)
{
    if (sem == NULL)
    {
        LOG_E("failed");
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (sem->handle == NULL)
    {
        LOG_E("failed");
        return false;
    }
    CloseHandle(sem->handle);
#else
    sem_destroy(&sem->handle);
#endif
    delete sem;
    return true;
}

bool com_condition_init(Condition* condition, const char* name)
{
    if (condition == NULL)
    {
        return false;
    }
    if (name == NULL)
    {
        name = "Unknown";
    }
    condition->name = name;
#if defined(_WIN32) || defined(_WIN64)
#else
    if (pthread_cond_init(&condition->handle, NULL) != 0)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#endif
    return true;
}

bool com_condition_uninit(Condition* condition)
{
    if (condition == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    pthread_cond_destroy(&condition->handle);
#endif
    return true;
}

Condition* com_condition_create(const char* name)
{
    if (name == NULL)
    {
        name = "Unknown";
    }
    Condition* condition = new Condition();
    condition->name = name;
#if defined(_WIN32) || defined(_WIN64)
#else
    if (pthread_cond_init(&condition->handle, NULL) != 0)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        delete condition;
        return NULL;
    }
#endif
    return condition;
}

bool com_condition_wait(Condition* condition, Mutex* mutex, int timeout_ms)
{
    if (condition == NULL || mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    if (timeout_ms > 0)
    {
        struct timespec ts;
        memset(&ts, 0, sizeof(struct timespec));
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64 tmp = ts.tv_nsec + (int64)timeout_ms * 1000 * 1000;
        ts.tv_nsec = tmp % (1000 * 1000 * 1000);
        ts.tv_sec += tmp / (1000 * 1000 * 1000);
        if (pthread_cond_timedwait(&condition->handle, &mutex->handle, &ts) != 0)
        {
            //LOG_I("%s:%d timeout or failed", __FUNC__, __LINE__);
            return false;
        }
    }
    else
    {
        if (pthread_cond_wait(&condition->handle, &mutex->handle) != 0)
        {
            LOG_E("%s:%d failed", __FUNC__, __LINE__);
            return false;
        }
    }
    return true;
#endif
}

bool com_condition_notify_one(Condition* condition)
{
    if (condition == NULL)
    {
        LOG_E("%s:%d arg incorrect", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    pthread_cond_signal(&condition->handle);
#endif
    return true;
}

bool com_condition_notify_all(Condition* condition)
{
    if (condition == NULL)
    {
        LOG_E("%s:%d arg incorrect", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    pthread_cond_broadcast(&condition->handle);
#endif
    return true;
}

bool com_condition_destroy(Condition* condition)
{
    if (condition == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    pthread_cond_destroy(&condition->handle);
#endif
    delete condition;
    return true;
}

bool com_mutex_init(Mutex* mutex, const char* name)
{
    if (mutex == NULL)
    {
        return false;
    }
    if (name == NULL)
    {
        name = "Unknown";
    }
    mutex->name = name;
#if defined(_WIN32) || defined(_WIN64)
    mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
#else
    if (pthread_mutex_init(&mutex->handle, NULL) != 0)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#endif
    return true;
}

bool com_mutex_uninit(Mutex* mutex)
{
    if (mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (mutex->handle != NULL)
    {
        CloseHandle(mutex->handle);
    }
#else
    pthread_mutex_destroy(&mutex->handle);
#endif
    return true;
}

Mutex* com_mutex_create(const char* name)
{
    if (name == NULL)
    {
        name = "Unknown";
    }
    Mutex* mutex = new Mutex();
    mutex->name = name;
#if defined(_WIN32) || defined(_WIN64)
    mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
#else
    if (pthread_mutex_init(&mutex->handle, NULL) != 0)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        delete mutex;
        mutex = NULL;
    }
#endif
    return mutex;
}

bool com_mutex_destroy(Mutex* mutex)
{
    if (mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (mutex->handle != NULL)
    {
        CloseHandle(mutex->handle);
    }
#else
    pthread_mutex_destroy(&mutex->handle);
#endif
    delete mutex;
    return true;
}

bool com_mutex_lock(Mutex* mutex)
{
    if (mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
    int ret = -1;
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s LOCKED", mutex->name, com_thread_get_name().c_str());
#endif
#if defined(_WIN32) || defined(_WIN64)
    if (mutex->handle == NULL)
    {
        LOG_E("mutex->handle is NULL");
        return false;
    }
    ret = WaitForSingleObject(mutex->handle, INFINITE);
#else
    ret = pthread_mutex_lock(&mutex->handle);
#endif
    if (ret != 0)
    {
        LOG_E("pthread_mutex_lock lock failed,ret=%d", ret);
        return false;
    }
    return true;
}

bool com_mutex_unlock(Mutex* mutex)
{
    if (mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
    int ret = -1;
#if defined(_WIN32) || defined(_WIN64)
    if (mutex->handle == NULL)
    {
        LOG_E("mutex->handle is NULL");
        return false;
    }
    ret = ReleaseSemaphore(mutex->handle, 1, NULL);
#else
    ret = pthread_mutex_unlock(&mutex->handle);
#endif
    if (ret != 0)
    {
        LOG_E("pthread_mutex_lock lock failed, ret=%d", ret);
        return false;
    }
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s UNLOCKED",
          mutex->name, com_thread_get_name().c_str());
#endif
    return true;
}

double com_cycle_perimeter(double diameter)
{
    return diameter * BCP_PI / 180.0;
}

double com_gps_distance_m(double lon_a, double lat_a, double lon_b,
                          double lat_b)
{
    double radLat1 = com_cycle_perimeter(lat_a);
    double radLat2 = com_cycle_perimeter(lat_b);
    double a = radLat1 - radLat2;
    double b = com_cycle_perimeter(lon_a) - com_cycle_perimeter(lon_b);
    double distance_km = 2 * asin(sqrt(pow(sin(a / 2), 2) +
                                       cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2)));
    distance_km = distance_km * BCP_EARTH_RADIUS_KM;
    double distance_m = (round(distance_km * 10000) / 10000) * 1000;
    return distance_m;
}

static bool com_gps_is_outsize_china(double longitude, double latitude)
{
    if (longitude < 72.004 || longitude > 137.8347)
    {
        return true;
    }
    if (latitude < 0.8293 || latitude > 55.8271)
    {
        return true;
    }
    return false;
}

static double gps_transform_latitude(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 *
                 sqrt(x > 0 ? x : -x);
    ret += (20.0 * sin(6.0 * x * BCP_PI) + 20.0 * sin(2.0 * x * BCP_PI)) * 2.0 /
           3.0;
    ret += (20.0 * sin(y * BCP_PI) + 40.0 * sin(y / 3.0 * BCP_PI)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * BCP_PI) + 320 * sin(y * BCP_PI / 30.0)) * 2.0 /
           3.0;
    return ret;
}

static double gps_transform_longitude(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(
                     x > 0 ? x : -x);
    ret += (20.0 * sin(6.0 * x * BCP_PI) + 20.0 * sin(2.0 * x * BCP_PI)) * 2.0 /
           3.0;
    ret += (20.0 * sin(x * BCP_PI) + 40.0 * sin(x / 3.0 * BCP_PI)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * BCP_PI) + 300.0 * sin(x / 30.0 * BCP_PI)) * 2.0 /
           3.0;
    return ret;
}

GPS com_gps_wgs84_to_gcj02(double longitude, double latitude)
{
    GPS gps;
    memset(&gps, 0, sizeof(GPS));
    if (com_gps_is_outsize_china(longitude, latitude))
    {
        gps.latitude = latitude;
        gps.longitude = longitude;
        return gps;
    }
    double dLat = gps_transform_latitude(longitude - 105.0, latitude - 35.0);
    double dLon = gps_transform_longitude(longitude - 105.0, latitude - 35.0);
    double radLat = latitude / 180.0 * BCP_PI;
    double magic = sin(radLat);
    magic = 1 - BCP_EARTH_OBLATENESS * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((BCP_EARTH_RADIUS_M * (1 - BCP_EARTH_OBLATENESS)) /
                             (magic * sqrtMagic) * BCP_PI);
    dLon = (dLon * 180.0) / (BCP_EARTH_RADIUS_M / sqrtMagic * cos(radLat) * BCP_PI);
    gps.latitude = latitude + dLat;
    gps.longitude = longitude + dLon;
    return gps;
}

GPS com_gps_gcj02_to_wgs84(double longitude, double latitude)
{
    GPS gps;
    GPS gps_temp;
    GPS gps_shift;
    memset(&gps, 0, sizeof(GPS));
    gps.latitude = latitude;
    gps.longitude = longitude;
    memset(&gps_temp, 0, sizeof(GPS));
    memset(&gps_shift, 0, sizeof(GPS));
    while (true)
    {
        gps_temp = com_gps_wgs84_to_gcj02(gps.longitude, gps.latitude);
        gps_shift.latitude = latitude - gps_temp.latitude;
        gps_shift.longitude = longitude - gps_temp.longitude;
        // 1e-7 ~ centimeter level accuracy
        if (fabs(gps_shift.latitude) < 1e-7 && fabs(gps_shift.longitude) < 1e-7)
        {
            // Result of experiment:
            //   Most of the time 2 iterations would be enough
            //   for an 1e-8 accuracy (milimeter level).
            //
            return gps;
        }
        gps.latitude += gps_shift.latitude;
        gps.longitude += gps_shift.longitude;
    }
    return gps;
}

GPS com_gps_bd09_to_gcj02(double longitude, double latitude)
{
    double x = longitude - 0.0065, y = latitude - 0.006;
    double z = sqrt(x * x + y * y) - 0.00002 * sin(y * BCP_PI_GPS_BD);
    double theta = atan2(y, x) - 0.000003 * cos(x * BCP_PI_GPS_BD);
    GPS gps;
    memset(&gps, 0, sizeof(GPS));
    gps.latitude = z * sin(theta);
    gps.longitude = z * cos(theta);
    return gps;
}

GPS com_gps_gcj02_to_bd09(double longitude, double latitude)
{
    double x = longitude;
    double y = latitude;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * BCP_PI_GPS_BD);
    double theta = atan2(y, x) + 0.000003 * cos(x * BCP_PI_GPS_BD);
    GPS gps;
    memset(&gps, 0, sizeof(GPS));
    gps.latitude = z * sin(theta) + 0.006;
    gps.longitude = z * cos(theta) + 0.0065;
    return gps;
}

//ip是大端结构
uint32 com_string_to_ip(const char* ip_str)
{
    uint32 ip[4];
    int ret = sscanf(ip_str, "%u.%u.%u.%u",
                     &ip[0], &ip[1], &ip[2],  &ip[3]);
    if (ret != 4)
    {
        return 0;
    }
    uint32 val = 0;
    val = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
    return val;
}

std::string com_ip_to_string(uint32 ip)
{
    static char str[64];
    snprintf(str, sizeof(str),
             "%u.%u.%u.%u", (ip) & 0xFF, ((ip) >> 8) & 0xFF,
             ((ip) >> 16) & 0xFF, ((ip) >> 24) & 0xFF);
    return str;
}

const char* com_get_bin_name()
{
    static std::string name;
#if __linux__ == 1
    if (name.empty())
    {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        int ret = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (ret < 0 || (ret >= sizeof(buf) - 1))
        {
            return NULL;
        }
        for (int i = ret; i >= 0; i--)
        {
            if (buf[i] == '/')
            {
                name = buf + i + 1;
                break;
            }
        }
    }
#endif
    return name.c_str();
}

const char* com_get_bin_path()
{
    static std::string path;
#if __linux__ == 1
    if (path.empty())
    {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        int ret = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (ret < 0 || (ret >= sizeof(buf) - 1))
        {
            return NULL;
        }
        for (int i = ret; i >= 0; i--)
        {
            if (buf[i] == '/')
            {
                buf[i + 1] = '\0';
                path = buf;
                break;
            }
        }
    }
#endif
    return path.c_str();
}

std::string com_get_cwd()
{
    std::string dir;
    char tmp[1024];
#if defined(_WIN32) || defined(_WIN64)
    if (_getcwd(tmp, sizeof(tmp)) != NULL)
#else
    if (getcwd(tmp, sizeof(tmp)) != NULL)
#endif
    {
        dir = tmp;
    }
    return dir;
}

void com_set_cwd(const char* dir)
{
    if (dir != NULL)
    {
#ifdef _WIN32
        _chdir(dir);
#else
        chdir(dir);
#endif
    }
}

uint64 com_ptr_to_number(const void* ptr)
{
    uint64 val = 0;
    memcpy(&val, &ptr, sizeof(void*));
    return val;
}

void* com_number_to_ptr(const uint64 val)
{
    void* ptr = NULL;
    memcpy(&ptr, &val, sizeof(void*));
    return ptr;
}

std::string com_uuid_generator()
{
    std::string val = com_string_format("%llu-%llu-%llu-%llu-%u",
                                        com_thread_get_pid(),
                                        com_thread_get_tid(),
                                        com_time_rtc_us(),
                                        com_time_cpu_us(),
                                        com_rand(0, 0xFFFFFFFF));
    CPPMD5 md5;
    md5.append((const uint8*)val.data(), val.size());
    return md5.finish().toHexString(false);
}

//计算最大公约数
int com_gcd(int x, int y)
{
    return y ? com_gcd(y, x % y) : x;
}

ByteArray::ByteArray()
{
    buf.clear();
}

ByteArray::ByteArray(int reserve_size)
{
    if (reserve_size > 0)
    {
        buf.reserve(reserve_size);
    }
}

ByteArray::ByteArray(const uint8* data, int data_size)
{
    if (data_size > 0 && data != NULL)
    {
        for (int i = 0; i < data_size; i++)
        {
            buf.push_back(data[i]);
        }
    }
}

ByteArray::ByteArray(const char* data)
{
    if (data != NULL)
    {
        for (int i = 0; i < strlen(data); i++)
        {
            buf.push_back((uint8)data[i]);
        }
    }
}

ByteArray::ByteArray(const char* data, int data_size)
{
    if (data_size > 0 && data != NULL)
    {
        for (int i = 0; i < data_size; i++)
        {
            buf.push_back((uint8)data[i]);
        }
    }
}

ByteArray::~ByteArray()
{
    buf.clear();
}

ByteArray& ByteArray::operator+(uint8 val)
{
    buf.push_back(val);
    return *this;
}

ByteArray& ByteArray::operator+=(uint8 val)
{
    buf.push_back(val);
    return *this;
}

bool ByteArray::operator==(ByteArray& bytes)
{
    if (this == &bytes)
    {
        return true;
    }
    if (bytes.getDataSize() != this->getDataSize())
    {
        return false;
    }
    for (int i = 0; i < bytes.getDataSize(); i++)
    {
        if (bytes.getData()[i] != this->getData()[i])
        {
            return false;
        }
    }
    return true;
}

bool ByteArray::operator!=(ByteArray& bytes)
{
    if (this == &bytes)
    {
        return false;
    }
    if (bytes.getDataSize() != this->getDataSize())
    {
        return true;
    }
    for (int i = 0; i < bytes.getDataSize(); i++)
    {
        if (bytes.getData()[i] != this->getData()[i])
        {
            return true;
        }
    }
    return false;
}

void ByteArray::clear()
{
    buf.clear();
}

uint8* ByteArray::getData()
{
    if (empty())
    {
        return NULL;
    }
    return &buf[0];
}

int ByteArray::getDataSize()
{
    return buf.size();
}

bool ByteArray::empty()
{
    return (buf.size() <= 0);
}

ByteArray& ByteArray::append(uint8 val)
{
    buf.push_back(val);
    return *this;
}

ByteArray& ByteArray::append(const uint8* data, int data_size)
{
    if (data_size > 0 && data != NULL)
    {
        for (int i = 0; i < data_size; i++)
        {
            buf.push_back(data[i]);
        }
    }
    return *this;
}

ByteArray& ByteArray::append(const char* data)
{
    if (data != NULL)
    {
        for (int i = 0; i < strlen(data); i++)
        {
            buf.push_back(data[i]);
        }
    }
    return *this;
}

ByteArray& ByteArray::append(ByteArray& bytes)
{
    if (this == &bytes)
    {
        ByteArray tmp = bytes;
        for (int i = 0; i < tmp.getDataSize(); i++)
        {
            buf.push_back(tmp.getData()[i]);
        }
    }
    else
    {
        for (int i = 0; i < bytes.getDataSize(); i++)
        {
            buf.push_back(bytes.getData()[i]);
        }
    }
    return *this;
}

uint8 ByteArray::getAt(int pos)
{
    if (pos < 0 || pos >= buf.size())
    {
        return 0;
    }
    return buf[pos];
}

void ByteArray::removeAt(int pos)
{
    if (pos < 0 || pos >= buf.size())
    {
        return;
    }
    buf.erase(buf.begin() + pos);
}

void ByteArray::removeHead(int count)
{
    if (count <= 0)
    {
        return;
    }
    if (count > buf.size())
    {
        count = buf.size();
    }
    buf.erase(buf.begin(), buf.begin() + count);
}

void ByteArray::removeTail(int count)
{
    if (count <= 0)
    {
        return;
    }
    while (count > 0 && buf.size() > 0)
    {
        buf.pop_back();
        count--;
    }
}

std::string ByteArray::toString()
{
    std::string str;
    if (getDataSize() > 0)
    {
        str.append((char*)getData(), getDataSize());
    }
    return str;
}

std::string ByteArray::toHexString(bool upper)
{
    std::string str = com_bytes_to_hexstring(getData(), getDataSize());
    if (upper)
    {
        com_string_to_upper(str);
    }
    return str;
}

ByteArray ByteArray::FromHexString(const char* hex_str)
{
    return com_hexstring_to_bytes(hex_str);
}

CPPMutex::CPPMutex(const char* name)
{
    if (name != NULL)
    {
        this->name = name;
    }
}

CPPMutex::~CPPMutex()
{
}

void CPPMutex::setName(const char* name)
{
    if (name != NULL)
    {
        this->name = name;
    }
}

const char* CPPMutex::getName()
{
    return name.c_str();
}

void CPPMutex::lock()
{
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s LOCKED", name.c_str(), com_thread_get_name().c_str());
#endif
    mutex.lock();
}

void CPPMutex::unlock()
{
    mutex.unlock();
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s UNLOCKED", name.c_str(), com_thread_get_name().c_str());
#endif
}

std::mutex* CPPMutex::getMutex()
{
    return &mutex;
}

AutoMutex::AutoMutex(Mutex* mutex)
{
    this->mutex_cpp = NULL;
    this->mutex_c = mutex;
    com_mutex_lock(this->mutex_c);
}

AutoMutex::AutoMutex(std::mutex* mutex)
{
    this->mutex_c = NULL;
    this->mutex_cpp = mutex;
    if (this->mutex_cpp != NULL)
    {
        this->mutex_cpp->lock();
    }
}

AutoMutex::AutoMutex(CPPMutex& mutex)
{
    this->mutex_c = NULL;
    this->mutex_cpp = mutex.getMutex();
    if (this->mutex_cpp != NULL)
    {
        this->mutex_cpp->lock();
    }
}

AutoMutex::~AutoMutex()
{
    if (this->mutex_c != NULL)
    {
        com_mutex_unlock(this->mutex_c);
    }
    if (this->mutex_cpp != NULL)
    {
        mutex_cpp->unlock();
    }
}

CPPSem::CPPSem(const char* name)
{
    com_sem_init(&sem, name);
}

CPPSem::~CPPSem()
{
    com_sem_uninit(&sem);
}

void CPPSem::setName(const char* name)
{
    if (name == NULL)
    {
        return;
    }
    sem.name = name;
}

const char* CPPSem::getName()
{
    return sem.name.c_str();
}

bool CPPSem::post()
{
    return com_sem_post(&sem);
}

bool CPPSem::wait(int timeout_ms)
{
    return com_sem_wait(&sem, timeout_ms);
}

CPPCondition::CPPCondition(const char* name)
{
    if (name != NULL)
    {
        this->name = name;
    }
}

CPPCondition::~CPPCondition()
{
}

void CPPCondition::setName(const char* name)
{
    this->name = name;
}

const char* CPPCondition::getName()
{
    return name.c_str();
}

bool CPPCondition::notifyOne()
{
    std::unique_lock<std::mutex> lock(mutex_cv);
    condition.notify_one();
    return true;
}

bool CPPCondition::notifyAll()
{
    std::unique_lock<std::mutex> lock(mutex_cv);
    condition.notify_all();
    return true;
}

bool CPPCondition::wait(int timeout_ms)
{
    std::unique_lock<std::mutex> lock(mutex_cv);
    if (timeout_ms > 0)
    {
        if (condition.wait_for(lock, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout)
        {
            return false;
        }
    }
    else
    {
        condition.wait(lock);
    }
    return true;
}

Message::Message()
{
    id = 0;
}

Message::Message(uint32 id)
{
    this->id = id;
}

Message::~Message()
{
}

void Message::reset()
{
    id = 0;
    datas.clear();
}

uint32 Message::getID()
{
    return id;
}

Message& Message::setID(uint32 id)
{
    this->id = id;
    return *this;
}

bool Message::isKeyExist(const char* key)
{
    if (key == NULL || datas.count(key) == 0)
    {
        return false;
    }
    return true;
}

bool Message::isEmpty()
{
    return datas.empty();
}

Message& Message::set(const char* key, const std::string& val)
{
    if (key != NULL)
    {
        datas[key] = val;
    }
    return *this;
}

Message& Message::set(const char* key, const char* val)
{
    if (key != NULL && val != NULL)
    {
        datas[key] = val;
    }
    return *this;
}

Message& Message::set(const char* key, const uint8* val, int val_size)
{
    if (key != NULL && val != NULL && val_size > 0)
    {
        std::string data;
        data.assign((char*)val, val_size);
        datas[key] = data;
    }
    return *this;
}

Message& Message::set(const char* key, ByteArray bytes)
{
    if (key != NULL && bytes.empty() == false)
    {
        std::string data;
        data.assign((char*)bytes.getData(), bytes.getDataSize());
        datas[key] = data;
    }
    return *this;
}

void Message::remove(const char* key)
{
    std::map<std::string, std::string>::iterator it = datas.find(key);
    if (it != datas.end())
    {
        datas.erase(it);
    }
}

void Message::removeAll()
{
    datas.clear();
}

bool Message::getBool(const char* key, bool default_val)
{
    if (isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas[key];
    return (strtol(val.c_str(), NULL, 10) == 1);
}

char Message::getChar(const char* key, char default_val)
{
    if (isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas[key];
    if (val.empty())
    {
        return default_val;
    }
    return val[0];
}

float Message::getFloat(const char* key, float default_val)
{
    if (isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas[key];
    return strtof(val.c_str(), NULL);
}

double Message::getDouble(const char* key, double default_val)
{
    if (isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas[key];
    return strtod(val.c_str(), NULL);
}

int8 Message::getInt8(const char* key, int8 default_val)
{
    return (int8)getInt64(key, default_val);
}

int16 Message::getInt16(const char* key, int16 default_val)
{
    return (int16)getInt64(key, default_val);
}

int32 Message::getInt32(const char* key, int32 default_val)
{
    return (int32)getInt64(key, default_val);
}

int64 Message::getInt64(const char* key, int64 default_val)
{
    if (isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas[key];
    return strtoll(val.c_str(), NULL, 10);
}

uint8 Message::getUInt8(const char* key, uint8 default_val)
{
    return (uint8)getUInt64(key, default_val);
}

uint16 Message::getUInt16(const char* key, uint16 default_val)
{
    return (uint16)getUInt64(key, default_val);
}

uint32 Message::getUInt32(const char* key, uint32 default_val)
{
    return (uint32)getUInt64(key, default_val);
}

uint64 Message::getUInt64(const char* key, uint64 default_val)
{
    if (isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas[key];
    return strtoull(val.c_str(), NULL, 10);
}

std::string Message::getString(const char* key, std::string default_val)
{
    if (isKeyExist(key) == false)
    {
        return default_val;
    }
    return datas[key];
}

ByteArray Message::getBytes(const char* key)
{
    ByteArray bytes;
    if (isKeyExist(key) == false)
    {
        return bytes;
    }
    std::string val = datas[key];
    return ByteArray((uint8*)val.data(), val.length());
}

std::string Message::toJSON()
{
    CJsonObject cjson;

    std::map<std::string, std::string>::iterator it;
    for (it = datas.begin(); it != datas.end(); it++)
    {
        cjson.Add(it->first, it->second);
    }
    return cjson.ToString();
}

Message Message::FromJSON(std::string json)
{
    CJsonObject cjson(json);
    Message msg;
    std::string key;
    std::string value;
    while (cjson.GetKey(key))
    {
        if (cjson.Get(key, value))
        {
            msg.set(key.c_str(), value);
        }
    }
    return msg;
}

xstring::xstring() : std::string()
{
}

xstring::xstring(const char* str) : std::string(str == NULL ? "" : str)
{
}

xstring::xstring(const std::string& str) : std::string(str)
{

}

xstring::xstring(const xstring& str) : std::string(str.c_str())
{

}

void xstring::toupper()
{
    std::transform(begin(), end(), begin(), ::toupper);
}

void xstring::tolower()
{
    std::transform(begin(), end(), begin(), ::tolower);
}

bool xstring::starts_with(const char* str)
{
    if (str == NULL)
    {
        return false;
    }
    return (strncmp(c_str(), str, strlen(str)) == 0);
}

bool xstring::starts_with(const xstring& str)
{
    return (strncmp(c_str(), str.c_str(), str.length()) == 0);
}

bool xstring::ends_with(const char* str)
{
    if (str == NULL)
    {
        return false;
    }
    int str_len = strlen(str);
    if (length() < str_len)
    {
        return false;
    }
    return (strncmp(c_str() + (length() - str_len), str, str_len) == 0);
}

bool xstring::ends_with(const xstring& str)
{
    if (length() < str.length())
    {
        return false;
    }
    return (strncmp(c_str() + (length() - str.length()), str.c_str(), str.length()) == 0);
}

bool xstring::contains(const char* str)
{
    if (str == NULL)
    {
        return false;
    }
    return (find(str) != std::string::npos);
}

bool xstring::contains(const xstring& str)
{
    return (find(str) != std::string::npos);
}

void xstring::replace(const char* from, const char* to)
{
    if (from == NULL || to == NULL)
    {
        return;
    }

    int pos = 0;
    int from_len = com_string_len(from);
    int to_len = com_string_len(to);
    while ((pos = find(from, pos)) != std::string::npos)
    {
        std::string::replace(pos, from_len, to);
        pos += to_len;
    }
    return;
}

void xstring::replace(const xstring& from, const xstring& to)
{
    replace(from.c_str(), to.c_str());
}

std::vector<xstring> xstring::split(const char* delim)
{
    std::vector<xstring> vals;
    if (delim == NULL)
    {
        return vals;
    }
    int pos = 0;
    int pos_pre = 0;
    int delim_len = strlen(delim);
    while (true)
    {
        pos = find_first_of(delim, pos_pre);
        if (pos == std::string::npos)
        {
            vals.push_back(substr(pos_pre));
            break;
        }
        vals.push_back(substr(pos_pre, pos - pos_pre));
        pos_pre = pos + delim_len;
    }
    return vals;
}

std::vector<xstring> xstring::split(const xstring& delim)
{
    std::vector<xstring> vals;
    int pos = 0;
    int pos_pre = 0;
    while (true)
    {
        pos = find_first_of(delim, pos_pre);
        if (pos == std::string::npos)
        {
            vals.push_back(substr(pos_pre));
            break;
        }
        vals.push_back(substr(pos_pre, pos - pos_pre));
        pos_pre = pos + delim.length();
    }
    return vals;
}

int xstring::to_int()
{
    return strtol(c_str(), NULL, 10);
}

long long xstring::to_long()
{
    return strtoll(c_str(), NULL, 10);
}

double xstring::to_double()
{
    return strtod(c_str(), NULL);
}

void xstring::trim(const char* t)
{
    trim_right(t);
    trim_left(t);
}

void xstring::trim_left(const char* t)
{
    if (t == NULL)
    {
        t = " \t\n\r\f\v";
    }
    erase(0, find_first_not_of(t));
}

void xstring::trim_right(const char* t)
{
    if (t == NULL)
    {
        t = " \t\n\r\f\v";
    }
    erase(find_last_not_of(t) + 1);
}

xstring xstring::format(const char* fmt, ...)
{
    xstring str;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len > 0)
    {
        len += 1;  //上面返回的长度不包含\0，这里加上
        std::vector<char> buf(len);
        va_start(args, fmt);
        len = vsnprintf(buf.data(), len, fmt, args);
        va_end(args);
        if (len > 0)
        {
            str.assign(buf.data(), len);
        }
    }
    return str;
}
