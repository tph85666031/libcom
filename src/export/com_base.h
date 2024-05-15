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
#include <unordered_map>
#include <set>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h> //ntohl
#include <windows.h>
#ifdef __GNUC__
#define COM_EXPORT __attribute__((dllexport)) extern
#else
#define COM_EXPORT __declspec(dllexport)
#endif
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#include <sys/time.h>
#define COM_EXPORT __attribute__((visibility ("default")))
#else
#include <arpa/inet.h> //ntohl
#include <semaphore.h> //sem mutex
#define COM_EXPORT __attribute__((visibility ("default")))
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

class COM_EXPORT Sem
{
public:
    std::string name;
#if defined(_WIN32) || defined(_WIN64)
    void* handle = NULL;
#elif __linux__ == 1
    sem_t handle;
#elif defined(__APPLE__)
    dispatch_semaphore_t handle = NULL;
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

typedef struct
{
    std::string host;
    uint16 port;
    int clientfd;
} SOCKET_CLIENT_DES;

COM_EXPORT int xfnmatch(const char* pattern, const char* string, int flags);
COM_EXPORT int com_run_shell(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
COM_EXPORT std::string com_run_shell_with_output(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
COM_EXPORT bool com_run_shell_in_thread(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

COM_EXPORT void com_sleep_s(uint32 val);
COM_EXPORT void com_sleep_ms(uint32 val);
COM_EXPORT uint32 com_rand(uint32 min, uint32 max);//[min-max]
COM_EXPORT char* com_strncpy(char* dst, const char* src, int dst_size);
COM_EXPORT bool com_strncmp_ignore_case(const char* str1, const char* str2, int size);
COM_EXPORT bool com_strncmp(const char* str1, const char* str2, int size);
COM_EXPORT bool com_string_equal(const char* a, const char* b);
COM_EXPORT bool com_string_equal_ignore_case(const char* a, const char* b);
COM_EXPORT std::vector<std::string> com_string_split(const char* str, const char* delim);

COM_EXPORT std::string& com_string_trim_left(std::string& str, const char* t = " \t\n\r\f\v");
COM_EXPORT std::string& com_string_trim_right(std::string& str, const char* t = " \t\n\r\f\v");
COM_EXPORT std::string& com_string_trim(std::string& str, const char* t = " \t\n\r\f\v");
COM_EXPORT char* com_string_trim_left(char* str, const char* t = " \t\n\r\f\v");
COM_EXPORT char* com_string_trim_right(char* str, const char* t = " \t\n\r\f\v");
COM_EXPORT char* com_string_trim(char* str);
COM_EXPORT bool com_string_start_with(const char* str, const char* prefix);
COM_EXPORT bool com_string_end_with(const char* str, const char* trail);
COM_EXPORT bool com_string_contain(const char* str, const char* sub);
COM_EXPORT bool com_string_match(const char* str, const char* pattern, bool is_path = false);
COM_EXPORT std::string& com_string_to_upper(std::string& str);
COM_EXPORT std::string& com_string_to_lower(std::string& str);
COM_EXPORT char* com_string_to_upper(char* str);
COM_EXPORT char* com_string_to_lower(char* str);
COM_EXPORT uint32 com_string_to_ip(const char* ip_str);
COM_EXPORT std::string com_ip_to_string(uint32 ip);
COM_EXPORT bool com_string_replace(char* str, char from, char to);
COM_EXPORT bool com_string_replace(std::string& str, const char* from, const char* to);
COM_EXPORT int com_string_len_utf8(const char* str);
COM_EXPORT int com_string_len(const char* str);
COM_EXPORT int com_string_size(const char* str);
COM_EXPORT bool com_string_is_empty(const char* str);
COM_EXPORT bool com_string_is_ip(const char* ip);
COM_EXPORT bool com_string_is_utf8(const std::string& str);
COM_EXPORT bool com_string_is_utf8(const char* str);
COM_EXPORT std::string com_string_format(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
COM_EXPORT std::wstring com_wstring_format(const wchar_t* fmt, ...);

COM_EXPORT int com_snprintf(char* buf, int buf_size, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
COM_EXPORT std::string com_bytes_to_hexstring(const uint8* data, int size);
COM_EXPORT int com_hexstring_to_bytes(const char* str, unsigned char* bytes, int size);
COM_EXPORT uint64 com_time_rtc_s();
COM_EXPORT uint64 com_time_rtc_ms();
COM_EXPORT uint64 com_time_rtc_us();
COM_EXPORT uint64 com_time_cpu_s();
COM_EXPORT uint64 com_time_cpu_ms();
COM_EXPORT uint64 com_time_cpu_us();
COM_EXPORT uint64 com_time_cpu_ns();
COM_EXPORT int64 com_timezone_china_s();
COM_EXPORT int64 com_timezone_get_s();
COM_EXPORT std::string com_time_to_string(uint64 time_s, const char* format = "%Y-%m-%d %H:%M:%S") __attribute__((format(strftime, 2, 0)));
COM_EXPORT uint64 com_time_from_string(const char* date_str, const char* format = "%d-%d-%d %d:%d:%d");

COM_EXPORT bool com_time_to_tm(uint64 time_s,  struct tm* tm_val);
COM_EXPORT uint64 com_time_from_tm(struct tm* tm_val);
COM_EXPORT bool com_gettimeofday(struct timeval* tp);

COM_EXPORT uint8 com_uint8_to_bcd(uint8 v);
COM_EXPORT uint8 com_bcd_to_uint8(uint8 bcd);

COM_EXPORT double com_gps_distance_m(double lon_a, double lat_a, double lon_b,
                                     double lat_b);

COM_EXPORT GPS com_gps_wgs84_to_gcj02(double longitude, double latitude);
COM_EXPORT GPS com_gps_gcj02_to_wgs84(double longitude, double latitude);
COM_EXPORT GPS com_gps_gcj02_to_bd09(double longitude, double latitude);
COM_EXPORT GPS com_gps_bd09_to_gcj02(double longitude, double latitude);

COM_EXPORT double com_cycle_perimeter(double diameter);

COM_EXPORT bool com_sem_init(Sem* sem, const char* name = "Unknown");//通过init来创建
COM_EXPORT bool com_sem_uninit(Sem* sem);//通过init创建的由uninit删除
COM_EXPORT Sem* com_sem_create(const char* name = "Unknown");//通过create（malloc）来创建
COM_EXPORT bool com_sem_post(Sem* sem);
COM_EXPORT bool com_sem_wait(Sem* sem, int timeout_ms = 0);
COM_EXPORT bool com_sem_destroy(Sem* sem);//通过create创建的由destroy删除

COM_EXPORT std::string com_get_bin_name();
COM_EXPORT std::string com_get_bin_path();

COM_EXPORT uint64 com_ptr_to_number(const void* ptr);
COM_EXPORT void* com_number_to_ptr(const uint64 val);

COM_EXPORT std::string com_get_cwd();
COM_EXPORT void com_set_cwd(const char* dir);

COM_EXPORT std::string com_uuid_generator();
COM_EXPORT int com_gcd(int x, int y);

COM_EXPORT int com_user_get_uid(const char* user = NULL);
COM_EXPORT int com_user_get_gid(const char* user = NULL);
COM_EXPORT std::string com_user_get_name(int uid);
COM_EXPORT std::string com_user_get_home(const char* user);
COM_EXPORT std::string com_user_get_home(const std::string& user);
COM_EXPORT int com_user_get_uid_logined();
COM_EXPORT int com_user_get_gid_logined();
COM_EXPORT std::string com_user_get_name_logined();
COM_EXPORT std::string com_user_get_home_logined();
COM_EXPORT std::string com_user_get_display_logined();
COM_EXPORT std::string com_user_get_language();

template <class... T>
COM_EXPORT int com_gcd(int x, int y, T...ns)
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

class COM_EXPORT CPPBytes
{
public:
    CPPBytes();
    CPPBytes(const char* data);
    CPPBytes(const std::string& data);
    CPPBytes(const char* data, int data_size);
    CPPBytes(const uint8* data, int data_size);
    CPPBytes(const CPPBytes& bytes);
    CPPBytes(CPPBytes&& bytes);
    CPPBytes(int reserve_size);
    virtual ~CPPBytes();
    CPPBytes& operator=(const CPPBytes& bytes);
    CPPBytes& operator=(CPPBytes&& bytes);
    CPPBytes& operator+(uint8 val);
    CPPBytes& operator+=(uint8 val);
    bool operator==(CPPBytes& bytes);
    bool operator!=(CPPBytes& bytes);
    void reserve(int size);
    void clear();
    bool empty() const;
    uint8* getData();
    const uint8* getData() const;
    int getDataSize() const;
    CPPBytes& append(uint8 val);
    CPPBytes& append(const uint8* data, int data_size);
    CPPBytes& append(const char* data);
    CPPBytes& append(const std::string& data);
    CPPBytes& append(const CPPBytes& bytes);
    CPPBytes& insert(int pos, uint8 val);
    CPPBytes& insert(int pos, const uint8* data, int data_size);
    CPPBytes& insert(int pos, const char* data);
    CPPBytes& insert(int pos, CPPBytes& bytes);
    uint8 getAt(int pos) const;
    void setAt(int pos, uint8 val);
    void removeAt(int pos);
    void removeHead(int count = 1);
    void removeTail(int count = 1);
    bool toFile(const char* file);
    std::string toString() const;
    std::string toHexString(bool upper = false) const;
    static CPPBytes FromHexString(const char* hex_str);
private:
    std::vector<uint8> buf;
};

class COM_EXPORT ComMutex
{
public:
    ComMutex(const char* name = "Unknown");
    virtual ~ComMutex();
    void setName(const char* name);
    const char* getName();
    void lock();
    void unlock();
    void lock_shared();
    void unlock_shared();
public :
    std::string name;
    std::atomic<uint64> flag;
};

class COM_EXPORT CPPSem
{
public:
    ComSem(const char* name = "Unknown");
    virtual ~ComSem();
    void setName(const char* name);
    const char* getName();
    bool post();
    bool wait(int timeout_ms = 0);
private :
    Sem sem;
};

class COM_EXPORT CPPCondition
{
public:
    ComCondition(const char* name = "Unknown");
    virtual ~ComCondition();
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

class COM_EXPORT Message
{
public:
    Message();
    Message(uint32 id);

    Message(const Message& msg);
    Message(Message&& msg);
    virtual ~Message();

    Message& operator=(const Message& bytes);
    Message& operator=(Message&& bytes);
    Message& operator+=(const Message& msg);

    void reset();

    uint32 getID() const;
    Message& setID(uint32 id);

    bool isKeyExist(const char* key) const;
    bool isEmpty() const;

    template<class T>
    Message& set(const char* key, const T val)
    {
        if(key != NULL)
        {
            datas[key] = std::to_string(val);
        }
        return *this;
    };

    Message& set(const char* key, const std::string& val);
    Message& set(const char* key, const char* val);
    Message& set(const char* key, const uint8* val, int val_size);
    Message& set(const char* key, const CPPBytes& bytes);
    Message& set(const char* key, const std::vector<std::string>& array);
    Message& set(const char* key, const Message& msg);

    Message& setPtr(const char* key, const void* ptr);

    void remove(const char* key);
    void removeAll();

    std::vector<std::string> getAllKeys() const;

    bool getBool(const char* key, bool default_val = false) const;
    char getChar(const char* key, char default_val = '\0') const;
    float getFloat(const char* key, float default_val = 0.0f) const;
    double getDouble(const char* key, double default_val = 0.0f) const;

    int8 getInt8(const char* key, int8 default_val = 0) const;
    int16 getInt16(const char* key, int16 default_val = 0) const;
    int32 getInt32(const char* key, int32 default_val = 0) const;
    int64 getInt64(const char* key, int64 default_val = 0) const;
    uint8 getUInt8(const char* key, uint8 default_val = 0) const;
    uint16 getUInt16(const char* key, uint16 default_val = 0) const;
    uint32 getUInt32(const char* key, uint32 default_val = 0) const;
    uint64 getUInt64(const char* key, uint64 default_val = 0) const;
    void* getPtr(const char* key, void* default_val = NULL) const;
    std::vector<std::string> getStringArray(const char* key) const;
    Message getMessage(const char* key) const;
    std::string getString(const char* key, std::string default_val = std::string()) const;
    CPPBytes getBytes(const char* key) const;
    uint8* getBytes(const char* key, int& size);

    std::string toJSON(bool pretty_style = false) const;

    static Message FromJSON(const char* json);
    static Message FromJSON(const std::string& json);

private:
    uint32 id;
    std::map<std::string, std::string> datas;
};

class ComException : public std::exception
{
public:
    ComException(const char* message, int code)
    {
        if(message != NULL)
        {
            this->message = message;
        }
        this->__code = code;
    };
    virtual ~ComException() throw()
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

class TimeCost final
{
public:
    TimeCost()
    {
        time_begin_ms = com_time_cpu_ms();
    };
    TimeCost(std::function<void(int64 cost_ms)> fc)
    {
        this->fc = fc;
        time_begin_ms = com_time_cpu_ms();
    };
    virtual ~TimeCost()
    {
        if(fc != NULL)
        {
            fc(com_time_cpu_ms() - time_begin_ms);
        }
    };
    int64 get()
    {
        return (com_time_cpu_ms() - time_begin_ms);
    };
private:
    std::function<void(int64 cost_ms)> fc = NULL;
    uint64 time_begin_ms = 0;
};

class COM_EXPORT ByteStreamReader
{
public:
    ByteStreamReader(FILE* fp);
    ByteStreamReader(const char* file, bool is_file);
    ByteStreamReader(const char* buffer);
    ByteStreamReader(const std::string& buffer);
    ByteStreamReader(const uint8* buffer, int buffer_size);
    ByteStreamReader(const CPPBytes& buffer);
    virtual ~ByteStreamReader();

    int64 find(const char* key, int offset = 0);
    int64 find(const uint8* key, int key_size, int offset = 0);
    int64 rfind(const char* key, int offset = 0);
    int64 rfind(const uint8* key, int key_size, int offset = 0);

    bool readLine(std::string& line);
    CPPBytes read(int size);
    int read(uint8* buf, int buf_size);
    CPPBytes readUntil(const char* key);
    CPPBytes readUntil(const uint8* key, int key_size);
    CPPBytes readUntil(std::function<bool(uint8)> func);

    int64 getPos();
    void setPos(int64 pos);
    void stepPos(int count);
    void reset();
private:
    FILE* fp = NULL;
    bool fp_internal = false;
    CPPBytes buffer;
    int64 buffer_pos = 0;
};

COM_EXPORT CPPBytes com_hexstring_to_bytes(const char* str);
COM_EXPORT CPPBytes com_string_utf8_to_utf16(const CPPBytes& utf8);
COM_EXPORT CPPBytes com_string_utf16_to_utf8(const CPPBytes& utf16);
COM_EXPORT CPPBytes com_string_utf8_to_utf32(const CPPBytes& utf8);
COM_EXPORT CPPBytes com_string_utf32_to_utf8(const CPPBytes& utf32);
COM_EXPORT CPPBytes com_string_utf16_to_utf32(const CPPBytes& utf16);
COM_EXPORT CPPBytes com_string_utf32_to_utf16(const CPPBytes& utf32);
COM_EXPORT std::string com_string_ansi_to_utf8(const std::string& ansi);
COM_EXPORT std::string com_string_ansi_to_utf8(const char* ansi);
COM_EXPORT std::string com_string_utf8_to_ansi(const std::string& utf8);
COM_EXPORT std::string com_string_utf8_to_ansi(const char* utf8);
COM_EXPORT std::string com_string_utf8_to_local(const std::string& utf8);
COM_EXPORT std::string com_string_utf8_to_local(const char* s);
COM_EXPORT std::string com_string_local_to_utf8(const std::string& s);
COM_EXPORT std::string com_string_local_to_utf8(const char* s);
COM_EXPORT std::wstring com_wstring_from_utf8(const CPPBytes& utf8);
COM_EXPORT std::wstring com_wstring_from_utf16(const CPPBytes& utf16);
COM_EXPORT std::wstring com_wstring_from_utf32(const CPPBytes& utf32);
COM_EXPORT CPPBytes com_wstring_to_utf8(const wchar_t* wstr);
COM_EXPORT CPPBytes com_wstring_to_utf8(const std::wstring& wstr);
COM_EXPORT CPPBytes com_wstring_to_utf16(const wchar_t* wstr);
COM_EXPORT CPPBytes com_wstring_to_utf16(const std::wstring& wstr);
COM_EXPORT CPPBytes com_wstring_to_utf32(const wchar_t* wstr);
COM_EXPORT CPPBytes com_wstring_to_utf32(const std::wstring& wstr);

#endif /* __COM_BASE_H__ */

