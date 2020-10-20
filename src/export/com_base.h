#ifndef __COM_BASE_H__
#define __COM_BASE_H__

#include <stdint.h>
typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <winsock.h> //ntohl
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h> //ntohl
#include <semaphore.h> //sem mutex
#endif

#define COM_PI                   (3.14159265358979324)
#define COM_PI_GPS_BD            (52.35987755982988733)
#define COM_EARTH_RADIUS_M       (6378388.0)
#define COM_EARTH_RADIUS_KM      (6378.388)
#define COM_EARTH_OBLATENESS     (0.00669342162296594323)

#define COM_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define COM_MIN(a,b)    (((a) < (b)) ? (a) : (b))

#ifdef __cplusplus
#define CAPI  extern "C"
#else
#define CAPI
#endif

#ifndef __GNUC__
#define __attribute__(x)
#endif

#ifdef __GNUC__
#define GCC_VERSION_AT_LEAST(x,y) (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#else
#define GCC_VERSION_AT_LEAST(x,y) 0
#endif

#if GCC_VERSION_AT_LEAST(3,1)
#define DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define DEPRECATED(msg) __declspec(deprecated)
#else
#define DEPRECATED(msg)
#endif

class Mutex
{
public:
    std::string name;
#if defined(_WIN32) || defined(_WIN64)
    void* handle = NULL;
#else
    pthread_mutex_t handle;
#endif
};

class Sem
{
public:
    std::string name;
#if defined(_WIN32) || defined(_WIN64)
    void* handle = NULL;
#else
    sem_t handle;
#endif
};

class Condition
{
public:
    std::string name;
#if defined(_WIN32) || defined(_WIN64)
    void* handle = NULL;
#else
    pthread_cond_t handle;
#endif
};

#ifndef htonll
#define htonll(n) (htons(0x1234)==0x1234 ? ((uint64)(n)) : (uint64)(((((uint64)htonl(n)) << 32) | (uint64)htonl((n) >> 32))))
#endif

#ifndef ntohll
#define ntohll(n) (ntohs(0x1234)==0x1234 ? ((uint64)(n)) : (uint64)(((((uint64)ntohl(n)) << 32) | (uint64)ntohl((n) >> 32))))
#endif

#ifdef __GLIBC__
#define __FUNC__  __PRETTY_FUNCTION__
#else
#define __FUNC__  __FUNCTION__
#endif

#ifndef __LINE__
#define __LINE__  ""
#endif

#ifndef __FILE__
#define __FILE__ ""
#endif

#ifndef __FILENAME__
#if defined(_WIN32) || defined(_WIN64)
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif
#endif

typedef struct
{
    double latitude;
    double longitude;
    double altitude;//米
    double direction;//度
    double speed;//米每秒
    uint32 time;//秒
} GPS;

class xstring : public std::string
{
public:
    using std::string::string;
    xstring();
    xstring(const char* str);
    xstring(const std::string& str);
    xstring(const xstring& str);
    virtual ~xstring();

    void toupper();
    void tolower();
    bool starts_with(const char* str);
    bool starts_with(const xstring& str);
    bool ends_with(const char* str);
    bool ends_with(const xstring& str);
    bool contains(const char* str);
    bool contains(const xstring& str);
    void replace(const char* from, const char* to);
    void replace(const xstring& from, const xstring& to);
    std::vector<xstring> split(const char* delim);
    std::vector<xstring> split(const xstring& delim);

    int to_int();
    long long to_long();
    double to_double();
    void trim(const char* t = " \t\n\r\f\v");
    void trim_left(const char* t = " \t\n\r\f\v");
    void trim_right(const char* t = " \t\n\r\f\v");

    static xstring format(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
};

std::string com_com_search_config_file();
bool com_run_shell(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
std::string com_run_shell_with_output(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

void com_sleep_s(uint32 val);
void com_sleep_ms(uint32 val);
uint32 com_rand(uint32 min, uint32 max);
char* com_strncpy(char* dst, const char* src, int dst_size);
bool com_strncmp_ignore_case(const char* str1, const char* str2, int size);
bool com_strncmp(const char* str1, const char* str2, int size);
bool com_string_equal(const char* a, const char* b);
bool com_string_equal_ignore_case(const char* a, const char* b);
std::vector<std::string> com_string_split(const char* str, const char* delim);

std::string& com_string_trim_left(std::string& str, const char* t = " \t\n\r\f\v");
std::string& com_string_trim_right(std::string& str, const char* t = " \t\n\r\f\v");
std::string& com_string_trim(std::string& str, const char* t = " \t\n\r\f\v");
char* com_string_trim_left(char* str);
char* com_string_trim_right(char* str);
char* com_string_trim(char* str);
bool com_string_start_with(const char* str, const char* prefix);
bool com_string_end_with(const char* str, const char* trail);
bool com_string_contain(const char* str, const char* sub);
bool com_string_match(const char* str, const char* pattern);
std::string& com_string_to_upper(std::string& str);
std::string& com_string_to_lower(std::string& str);
char* com_string_to_upper(char* str);
char* com_string_to_lower(char* str);
uint32 com_string_to_ip(const char* ip_str);
std::string com_ip_to_string(uint32 ip);
bool com_string_replace(char* str, char from, char to);
bool com_string_replace(std::string& str, const char* from, const char* to);
int com_string_len_utf8(const char* str);
int com_string_len(const char* str);
int com_string_size(const char* str);
bool com_string_is_ip(const char* ip);
std::string com_string_format(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
std::string com_string_from_wstring(const wchar_t* s);
std::string com_string_from_wstring(const std::wstring& s);
std::wstring com_string_to_wstring(const char* s);
std::wstring com_string_to_wstring(const std::string& s);
int com_snprintf(char* buf, int buf_size, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
std::string com_bytes_to_hexstring(const uint8* data, uint16 size);
int com_hexstring_to_bytes(const char* str, unsigned char* bytes, int size);

uint32 com_time_rtc_s();
uint64 com_time_rtc_ms();
uint64 com_time_rtc_us();
uint32 com_time_cpu_s();
uint64 com_time_cpu_ms();
uint64 com_time_cpu_us();
uint64 com_time_cpu_ns();
int32 com_timezone_china_s();
int32 com_timezone_get_s();
std::string com_time_to_string(uint32 time_s);
std::string com_time_to_string(uint32 time_s, const char* format) __attribute__((format(strftime, 2, 0)));
bool com_time_to_tm(uint32 time_s,  struct tm* tm_val);
bool com_time_to_bcd(uint32 time_s, uint8 bcd_time[6]);
uint32 com_time_from_bcd(uint8 bcd_time[6]);

/* date_str格式为 yyyy-mm-dd HH:MM:SS */
uint32 com_string_to_time(const char* date_str);

uint32 com_tm_to_time(struct tm* tm_val);
bool com_tm_to_bcd_date(struct tm* tm_val, uint8 bcd_date[6]);
bool com_gettimeofday(struct timeval* tp);

uint8 com_uint8_to_bcd(uint8 v);
uint8 com_bcd_to_uint8(uint8 bcd);

double com_gps_distance_m(double lon_a, double lat_a, double lon_b,
                          double lat_b);

GPS com_gps_wgs84_to_gcj02(double longitude, double latitude);
GPS com_gps_gcj02_to_wgs84(double longitude, double latitude);
GPS com_gps_gcj02_to_bd09(double longitude, double latitude);
GPS com_gps_bd09_to_gcj02(double longitude, double latitude);

double com_cycle_perimeter(double diameter);

bool com_mutex_init(Mutex* mutex, const char* name = "Unknown");//通过init来创建
bool com_mutex_uninit(Mutex* mutex);//通过init创建的由uninit删除
Mutex* com_mutex_create(const char* name = "Unknown");//通过create（malloc）来创建
bool com_mutex_destroy(Mutex* mutex);//通过create创建的由destroy删除
bool com_mutex_lock(Mutex* mutex);
bool com_mutex_unlock(Mutex* mutex);

bool com_sem_init(Sem* sem, const char* name = "Unknown");//通过init来创建
bool com_sem_uninit(Sem* sem);//通过init创建的由uninit删除
Sem* com_sem_create(const char* name = "Unknown");//通过create（malloc）来创建
bool com_sem_post(Sem* sem);
bool com_sem_wait(Sem* sem, int timeout_ms = 0);
bool com_sem_destroy(Sem* sem);//通过create创建的由destroy删除

bool com_condition_init(Condition* condition, const char* name = "Unknown");//通过init来创建
bool com_condition_uninit(Condition* condition);//通过init创建的由uninit删除
Condition* com_condition_create(const char* name = "Unknown");//通过create（malloc）来创建
bool com_condition_wait(Condition* condition, Mutex* mutex, int timeout_ms = 0);
bool com_condition_notify_one(Condition* condition);
bool com_condition_notify_all(Condition* condition);
bool com_condition_destroy(Condition* condition);//通过create创建的由destroy删除

const char* com_get_bin_name();
const char* com_get_bin_path();

uint64 com_ptr_to_number(const void* ptr);
void* com_number_to_ptr(const uint64 val);

std::string com_get_cwd();
void com_set_cwd(const char* dir);

std::string com_uuid_generator();

int com_gcd(int x, int y);
std::string com_get_login_user_name();
int com_get_login_user_id();
std::string com_get_login_user_home();

template <class... T>
int com_gcd(int x, int y, T...ns)
{
    int gcd = com_gcd(x, y);
    // *INDENT-OFF*
    for(uint32 n : {ns...})
    // *INDENT-ON*
    {
        gcd = com_gcd(gcd, n);
    }
    return gcd;
}
#if __linux__ == 1
#define __MACRO_TO_STR(x)    #x
#define MACRO_TO_STR(x)      __MACRO_TO_STR(x)

#define __MACRO_CONCAT(x,y)  x##y
#define MACRO_CONCAT(x,y)    __MACRO_CONCAT(x,y)
#else
#define MACRO_CONCAT(x,y)
#endif

#define MODULE_DEPEND ("depend_lib=" MODULE_DEPEND_LIB "\ndepend_app=" MODULE_DEPEND_APP)

#ifdef LIB_HEADER_EXPORT
#undef LIB_HEADER_EXPORT
#endif

#if __linux__ == 1
#define LIB_HEADER_EXPORT \
CAPI const char* MACRO_CONCAT(MODULE_NAME_X,_name)() {return MODULE_NAME;}\
CAPI const char* MACRO_CONCAT(MODULE_NAME_X,_ver)() {return MODULE_VER;}\
CAPI const char* MACRO_CONCAT(MODULE_NAME_X,_type)() {return MODULE_TYPE;}\
CAPI const char* MACRO_CONCAT(MODULE_NAME_X,_depend)() {return MODULE_DEPEND;}
#else
#define LIB_HEADER_EXPORT
#endif

class ByteArray
{
public:
    ByteArray();
    ByteArray(const char* data);
    ByteArray(const char* data, int data_size);
    ByteArray(const uint8* data, int data_size);
    ByteArray(int reserve_size);
    virtual ~ByteArray();
    ByteArray& operator+(uint8 val);
    ByteArray& operator+=(uint8 val);
    bool operator==(ByteArray& bytes);
    bool operator!=(ByteArray& bytes);
    void clear();
    bool empty();
    uint8* getData();
    int getDataSize();
    ByteArray& append(uint8 val);
    ByteArray& append(const uint8* data, int data_size);
    ByteArray& append(const char* data);
    ByteArray& append(ByteArray& bytes);
    uint8 getAt(int pos);
    void removeAt(int pos);
    void removeHead(int count = 1);
    void removeTail(int count = 1);
    std::string toString();
    std::string toHexString(bool upper = false);
    static ByteArray FromHexString(const char* hex_str);
private:
    std::vector<uint8> buf;
};

class CPPMutex
{
public:
    CPPMutex(const char* name = "Unknown");
    virtual ~CPPMutex();
    void setName(const char* name);
    const char* getName();
    void lock();
    void unlock();
    std::mutex* getMutex();
private :
    std::string name;
    std::mutex mutex;
};

class CPPSem
{
public:
    CPPSem(const char* name = "Unknown");
    virtual ~CPPSem();
    void setName(const char* name);
    const char* getName();
    bool post();
    bool wait(int timeout_ms = 0);
private :
    Sem sem;
};

class CPPCondition
{
public:
    CPPCondition(const char* name = "Unknown");
    virtual ~CPPCondition();
    void setName(const char* name);
    const char* getName();
    bool notifyOne();
    bool notifyAll();
    bool wait(int timeout_ms = 0);
private :
    std::string name;
    std::mutex mutex_cv;
    std::condition_variable condition;
};

class AutoMutex
{
public:
    AutoMutex(CPPMutex& mutex);
    AutoMutex(Mutex* mutex);
    AutoMutex(std::mutex* mutex);
    virtual ~AutoMutex();
private:
    Mutex* mutex_c = NULL;
    std::mutex* mutex_cpp = NULL;
};

class Message
{
public:
    Message();
    Message(uint32 id);
    virtual ~Message();

    void reset();

    uint32 getID();
    Message& setID(uint32 id);

    bool isKeyExist(const char* key);
    bool isEmpty();

    template<class T>
    Message& set(const char* key, const T val)
    {
        if (key != NULL)
        {
            datas[key] = std::to_string(val);
        }
        return *this;
    };

    Message& set(const char* key, const std::string& val);
    Message& set(const char* key, const char* val);
    Message& set(const char* key, const uint8* val, int val_size);
    Message& set(const char* key, ByteArray bytes);

    void remove(const char* key);
    void removeAll();

    bool getBool(const char* key, bool default_val = false);
    char getChar(const char* key, char default_val = '\0');
    float getFloat(const char* key, float default_val = 0.0f);
    double getDouble(const char* key, double default_val = 0.0f);

    int8 getInt8(const char* key, int8 default_val = 0);
    int16 getInt16(const char* key, int16 default_val = 0);
    int32 getInt32(const char* key, int32 default_val = 0);
    int64 getInt64(const char* key, int64 default_val = 0);
    uint8 getUInt8(const char* key, uint8 default_val = 0);
    uint16 getUInt16(const char* key, uint16 default_val = 0);
    uint32 getUInt32(const char* key, uint32 default_val = 0);
    uint64 getUInt64(const char* key, uint64 default_val = 0);

    std::string getString(const char* key, std::string default_val = std::string());
    ByteArray getBytes(const char* key);

    std::string toJSON();
    static Message FromJSON(std::string json);

private:
    uint32 id;
    std::map<std::string, std::string> datas;
};

class ComException : public std::exception
{
public:
    ComException(const char* message, int code)
    {
        if (message != NULL)
        {
            this->message = message;
        }
        this->__code = code;
    };
    virtual ~ComException() throw ()
    {
    };
    const char* what() const throw()
    {
        return this->message.c_str();
    }
    int code() const throw()
    {
        return this->__code;
    }
private:
    std::string message;
    int __code;
};

ByteArray com_hexstring_to_bytes(const char* str);

#endif /* __COM_BASE_H__ */

