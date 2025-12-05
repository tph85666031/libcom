#ifndef __COM_BASE_H__
#define __COM_BASE_H__

#include <stdint.h>
typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned short     uint16_be;
typedef unsigned int       uint32;
typedef unsigned int       uint32_be;
typedef unsigned long long uint64;
typedef unsigned long long uint64_be;

typedef unsigned long      ulong;

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <algorithm>
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
#include <ws2tcpip.h> //getaddrinfo
#pragma comment(lib, "Ws2_32.lib")
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
#define DEPRECATED(msg) __declspec(deprecated(msg))
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
COM_EXPORT std::vector<std::string> com_string_split(const char* str, int str_size, char delim, bool keep_empty = true);
COM_EXPORT std::vector<std::string> com_string_split(const char* str, const char* delim, bool keep_empty = true);

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
COM_EXPORT bool com_string_replace(char* str, char from, char to);
COM_EXPORT bool com_string_replace(std::string& str, const char* from, const char* to);
COM_EXPORT int com_string_length_utf8(const char* str);
COM_EXPORT int com_string_length(const char* str);
COM_EXPORT int com_string_size(const char* str);
COM_EXPORT bool com_string_is_empty(const char* str);
COM_EXPORT bool com_string_is_utf8(const std::string& str);
COM_EXPORT bool com_string_is_utf8(const char* str, int len = 0);
COM_EXPORT std::string com_string_format(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
COM_EXPORT std::wstring com_wstring_format(const wchar_t* fmt, ...);

COM_EXPORT bool com_string_is_ipv4(const char* str);
COM_EXPORT bool com_string_is_ipv6(const char* str);
COM_EXPORT bool com_string_to_ipv4(const char* str, uint32_be& ipv4);
COM_EXPORT bool com_string_to_ipv6(const char* str, uint16_be ipv6[8]);
COM_EXPORT std::string com_string_from_ipv4(uint32_be ipv4);
COM_EXPORT std::string com_string_from_ipv6(uint16_be ipv6[8]);
COM_EXPORT bool com_string_to_sockaddr(const char* ip, uint16 port, struct sockaddr_storage* addr);
COM_EXPORT bool com_string_to_sockaddr(const char* ip, uint16 port, struct sockaddr_storage& addr);
COM_EXPORT std::string com_string_from_sockaddr(const struct sockaddr_storage* addr);
COM_EXPORT std::string com_string_from_sockaddr(const struct sockaddr_storage& addr);

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
COM_EXPORT void com_sem_reset(Sem* sem);
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
COM_EXPORT std::string com_user_get_name(int uid = -1);
COM_EXPORT std::string com_user_get_home(int uid = -1);
COM_EXPORT std::string com_user_get_home(const char* user = NULL);
COM_EXPORT std::string com_user_get_home(const std::string& user = std::string());
COM_EXPORT int com_user_get_uid_logined();
COM_EXPORT int com_user_get_gid_logined();
COM_EXPORT std::string com_user_get_name_logined();
COM_EXPORT std::string com_user_get_home_logined();
COM_EXPORT std::string com_user_get_display_logined();
COM_EXPORT std::string com_user_get_language();
COM_EXPORT std::string com_system_set_env(const char* name, const char* val);
COM_EXPORT std::string com_system_remove_env(const char* name);

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

class COM_EXPORT ComBytes
{
public:
    ComBytes();
    ComBytes(const char* data);
    ComBytes(const std::string& data);
    ComBytes(const char* data, int data_size);
    ComBytes(const uint8* data, int data_size);
    ComBytes(const ComBytes& bytes);
    ComBytes(ComBytes&& bytes);
    ComBytes(int reserve_size);
    virtual ~ComBytes();
    ComBytes& operator=(const ComBytes& bytes);
    ComBytes& operator=(ComBytes&& bytes);
    ComBytes& operator+(uint8 val);
    ComBytes& operator+=(uint8 val);
    bool operator==(ComBytes& bytes);
    bool operator!=(ComBytes& bytes);
    void reserve(int size);
    void clear();
    bool empty() const;
    uint8* getData();
    const uint8* getData() const;
    int getDataSize() const;
    ComBytes& append(uint8 val);
    ComBytes& append(const uint8* data, int data_size);
    ComBytes& append(const char* data);
    ComBytes& append(const std::string& data);
    ComBytes& append(const ComBytes& bytes);
    ComBytes& insert(int pos, uint8 val);
    ComBytes& insert(int pos, const uint8* data, int data_size);
    ComBytes& insert(int pos, const char* data);
    ComBytes& insert(int pos, ComBytes& bytes);
    uint8 getAt(int pos) const;
    void setAt(int pos, uint8 val);
    void removeAt(int pos);
    void removeHead(int count = 1);
    void removeTail(int count = 1);
    bool toFile(const char* file);
    std::string toString() const;
    std::string toHexString(bool upper = false) const;
    static ComBytes FromHexString(const char* hex_str);
private:
    std::vector<uint8> buf;
};
typedef ComBytes DEPRECATED("Use ComBytes instead") CPPBytes;

class COM_EXPORT ComMutex
{
public:
    ComMutex(const char* name = NULL);
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

class COM_EXPORT ComSem
{
public:
    ComSem(const char* name = NULL);
    virtual ~ComSem();
    void setName(const char* name);
    const char* getName();
    bool post();
    bool wait(int timeout_ms = 0);
    void reset();
private :
    Sem sem;
};
typedef ComSem DEPRECATED("Use ComSem instead") CPPSem;

class COM_EXPORT ComCondition
{
public:
    ComCondition(const char* name = NULL);
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
typedef ComCondition DEPRECATED("Use ComCondition instead") CPPCondition;

template<class T1, class T2>
class COM_EXPORT ComLRUMap
{
public:
    ComLRUMap(size_t max_size = 1024) : max_size(std::max(max_size, size_t(1))){};
    virtual ~ComLRUMap() = default;

    bool put(const T1& key, const T2& value)
    {
        auto it = map.find(key);
        if(it != map.end())
        {
            it->second.second = value;
            list.erase(it->second.first);
            list.push_back(key);
            it->second.first = --list.end();
            return true;
        }
        if(map.size() >= max_size)
        {
            const T1& oldest_key = list.front();
            map.erase(oldest_key);
            list.pop_front();
        }
        list.push_back(key);
        map.emplace(key, std::make_pair(--list.end(), value));
        return true;
    }
    T2& get(const T1& key)
    {
        auto it = map.find(key);
        if(it == map.end())
        {
            throw std::out_of_range("key not found");
        }
        list.erase(it->second.first);
        list.push_back(key);
        it->second.first = --list.end();

        return it->second.second;
    }
    T2 getCopy(const T1& key)
    {
        return get(key);
    }
private:
    size_t max_size = 1024;
    std::unordered_map<T1, std::pair<typename std::list<T1>::iterator, T2>> map;
    std::list<T1> list;
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
    bool operator==(const Message& msg);

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
    Message& set(const char* key, const ComBytes& bytes);
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
    ComBytes getBytes(const char* key) const;
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
    ByteStreamReader(const ComBytes& buffer);
    virtual ~ByteStreamReader();

    int64 find(const char* key, int offset = 0);
    int64 find(const uint8* key, int key_size, int offset = 0);
    int64 rfind(const char* key, int offset = 0);
    int64 rfind(const uint8* key, int key_size, int offset = 0);

    bool readLine(std::string& line);
    ComBytes read(int size);
    int read(uint8* buf, int buf_size);
    ComBytes readUntil(const char* key);
    ComBytes readUntil(const uint8* key, int key_size);
    ComBytes readUntil(std::function<bool(uint8)> func);

    int64 getPos();
    bool setPos(int64 pos);
    void stepPos(int64 count);
    bool seek(int64 offset, int whence);
    int64 size();
    void reset();
private:
    FILE* fp = NULL;
    bool fp_internal = false;
    ComBytes buffer;
    int64 buffer_pos = 0;
};

class COM_EXPORT ComOptionDesc
{
public:
    std::string key;
    std::string description;
    bool need_value = false;;
    std::string value;
};

class COM_EXPORT ComOption
{
public:
    ComOption();
    virtual ~ComOption();
    ComOption& addOption(const char* key, const char* description, bool need_value = true);
    bool keyExist(const char* key);
    bool valueExist(const char* key);
    std::string getString(const char* key, std::string default_val = std::string());
    int64 getNumber(const char* key, int64 default_val = 0);
    double getDouble(const char* key, double default_val = 0.0f);
    bool getBool(const char* key, bool default_val = false);
    bool parse(int argc, const char** argv);
    Message toMessage();
    std::string showUsage();
private:
    std::map<std::string, ComOptionDesc> params;
    std::string app_name;
};

class COM_EXPORT ComAutoClean
{
public:
    ComAutoClean(std::function<void()> cb)
    {
        this->cb = cb;
    }
    virtual ~ComAutoClean()
    {
        cb();
    }
private:
    std::function<void()> cb;
};

COM_EXPORT ComBytes com_hexstring_to_bytes(const char* str);
COM_EXPORT ComBytes com_string_utf8_to_utf16(const ComBytes& utf8);
COM_EXPORT ComBytes com_string_utf16_to_utf8(const ComBytes& utf16);
COM_EXPORT ComBytes com_string_utf8_to_utf32(const ComBytes& utf8);
COM_EXPORT ComBytes com_string_utf32_to_utf8(const ComBytes& utf32);
COM_EXPORT ComBytes com_string_utf16_to_utf32(const ComBytes& utf16);
COM_EXPORT ComBytes com_string_utf32_to_utf16(const ComBytes& utf32);
COM_EXPORT std::string com_string_ansi_to_utf8(const std::string& ansi);
COM_EXPORT std::string com_string_ansi_to_utf8(const char* ansi);
COM_EXPORT std::string com_string_utf8_to_ansi(const std::string& utf8);
COM_EXPORT std::string com_string_utf8_to_ansi(const char* utf8);
COM_EXPORT std::string com_string_utf8_to_local(const std::string& utf8);
COM_EXPORT std::string com_string_utf8_to_local(const char* s);
COM_EXPORT std::string com_string_local_to_utf8(const std::string& s);
COM_EXPORT std::string com_string_local_to_utf8(const char* s);
COM_EXPORT std::wstring com_wstring_from_utf8(const ComBytes& utf8);
COM_EXPORT std::wstring com_wstring_from_utf16(const ComBytes& utf16);
COM_EXPORT std::wstring com_wstring_from_utf32(const ComBytes& utf32);
COM_EXPORT ComBytes com_wstring_to_utf8(const wchar_t* wstr);
COM_EXPORT ComBytes com_wstring_to_utf8(const std::wstring& wstr);
COM_EXPORT ComBytes com_wstring_to_utf16(const wchar_t* wstr);
COM_EXPORT ComBytes com_wstring_to_utf16(const std::wstring& wstr);
COM_EXPORT ComBytes com_wstring_to_utf32(const wchar_t* wstr);
COM_EXPORT ComBytes com_wstring_to_utf32(const std::wstring& wstr);

#endif /* __COM_BASE_H__ */

