#ifndef __COM_SERIALIZER_H__
#define __COM_SERIALIZER_H__

#include "com_base.h"
#include "CJsonObject.h"
#include <vector>

class COM_EXPORT Serializer
{
public:
    Serializer();
    Serializer(const uint8* data, int data_size);
    ~Serializer();
    void load(const uint8* data, int data_size);
    Serializer& append(bool val);
    Serializer& append(char val);
    Serializer& append(uint8 val);
    Serializer& append(int8 val);
    Serializer& append(uint16 val);
    Serializer& append(int16 val);
    Serializer& append(uint32 val);
    Serializer& append(int32 val);
    Serializer& append(uint64 val);
    Serializer& append(int64 val);

    Serializer& append(const std::string val, int val_size = -1);
    Serializer& append(const char* val, int val_size = -1);
    Serializer& append(const uint8* val, int val_size);
    Serializer& append(ComBytes& bytes);

    template<class T>
    Serializer& append(const std::vector<T>& list)
    {
        for(auto it = list.begin(); it != list.end(); it++)
        {
            append(*it);
        }
        return *this;
    }

    template<class T>
    Serializer& append(const std::set<T>& list)
    {
        for(auto it = list.begin(); it != list.end(); it++)
        {
            append(*it);
        }
        return *this;
    }

    template<class T>
    Serializer& append(const std::deque<T>& list)
    {
        for(auto it = list.begin(); it != list.end(); it++)
        {
            append(*it);
        }
        return *this;
    }

    template<class T>
    Serializer& append(const std::queue<T>& list)
    {
        std::queue<T> tmp = list;
        while(tmp.size() > 0)
        {
            append(tmp.front());
            tmp.pop();
        }
        return *this;
    }

    template<class T>
    Serializer& append(const std::list<T>& list)
    {
        for(auto it = list.begin(); it != list.end(); it++)
        {
            append(*it);
        }
        return *this;
    }

    template<class T1, class T2>
    Serializer& append(const std::map<T1, T2>& list)
    {
        for(auto it = list.begin(); it != list.end(); it++)
        {
            append(it->second);
        }
        return *this;
    }

#if __cplusplus >= 201101L
    template<class T1, class T2>
    Serializer& append(const std::unordered_map<T1, T2>& list)
    {
        for(auto it = list.begin(); it != list.end(); it++)
        {
            append(it->second);
        }
        return *this;
    }
#endif

    int detach(bool& val);
    int detach(char& val);
    int detach(uint8& val);
    int detach(int8& val);
    int detach(uint16& val);
    int detach(int16& val);
    int detach(uint32& val);
    int detach(int32& val);
    int detach(uint64& val);
    int detach(int64& val);

    int detach(std::string& val, int val_size = -1);
    int detach(char* val, int val_size);
    int detach(uint8* val, int val_size);
    int detach(ComBytes& bytes, int val_size);

    void clear();
    void detachReset();
    int getDetachRemainSize();
    ComBytes toBytes();

    uint8* getData();
    int getDataSize();

    Serializer& enableByteOrderModify();
    Serializer& disableByteOrderModify();

private:
    std::vector<uint8> buf;
    int pos_detach = 0;;
    bool change_byte_order = true;
};

template<typename T, size_t N>
struct __struct_tuple_serializer
{
    static void serializer(Serializer& s, const T& t)
    {
        __struct_tuple_serializer < T, N - 1 >::serializer(s, t);
        auto val = std::get < N - 1 > (t);
        s.append(val);
    }
};

template<typename T>
struct __struct_tuple_serializer<T, 1>
{
    static void serializer(Serializer& s, const T& t)
    {
        auto val = std::get<0>(t);
        s.append(val);
    }
};

template<typename... Args>
void __tuple_serializer(Serializer& s, const std::tuple<Args...>& t)
{
    __struct_tuple_serializer<decltype(t), sizeof...(Args)>::serializer(s, t);
}

template<typename T, size_t N>
struct __struct_tuple_serializer_json
{
    static void serializer(CJsonObject& j, const T& t, std::vector<std::string>& n)
    {
        __struct_tuple_serializer_json < T, N - 1 >::serializer(j, t, n);
        auto& val = std::get < N - 1 > (t);
        std::string name = n[N - 1];
        com_string_trim(name);
        j.Add(name, val);
    }
    static void deserializer(CJsonObject& j, T& t, std::vector<std::string>& n)
    {
        __struct_tuple_serializer_json < T, N - 1 >::deserializer(j, t, n);
        auto& val = std::get < N - 1 > (t);
        std::string name = n[N - 1];
        com_string_trim(name);
        j.Get(name, val);
    }
};

template<typename T>
struct __struct_tuple_serializer_json<T, 1>
{
    static void serializer(CJsonObject& j, const T& t, std::vector<std::string>& n)
    {
        auto& val = std::get<0>(t);
        std::string name = n[0];
        com_string_trim(name);
        j.Add(name, val);
    }
    static void deserializer(CJsonObject& j, T& t, std::vector<std::string>& n)
    {
        auto& val = std::get<0>(t);
        std::string name = n[0];
        com_string_trim(name);
        j.Get(name, val);
    }
};

template<typename... Args>
void __tuple_serializer_json_encode(CJsonObject& j, const std::tuple<Args...>& t, std::vector<std::string>& n)
{
    __struct_tuple_serializer_json<decltype(t), sizeof...(Args)>::serializer(j, t, n);
}

template<typename... Args>
void __tuple_serializer_json_decode(CJsonObject& j, std::tuple<Args...>& t, std::vector<std::string>& n)
{
    __struct_tuple_serializer_json<decltype(t), sizeof...(Args)>::deserializer(j, t, n);
}

#define META_B(...) \
ComBytes toBytes(){\
    auto __t = std::make_tuple(__VA_ARGS__);\
    Serializer __s;\
    __tuple_serializer(__s,__t);\
    return __s.toBytes();\
}

#define META_J(...) \
std::string toJson(bool pretty=false) const{\
    auto __t = std::forward_as_tuple(__VA_ARGS__);\
    std::vector<std::string> __n = com_string_split(#__VA_ARGS__, ",");\
    CJsonObject __j;\
    __tuple_serializer_json_encode(__j,__t,__n);\
    return pretty?__j.ToFormattedString():__j.ToString();\
}\
void fromJson(const char* json){\
    auto __t = std::forward_as_tuple(__VA_ARGS__);\
    std::vector<std::string> __n = com_string_split(#__VA_ARGS__, ",");\
    CJsonObject __j(json);\
    __tuple_serializer_json_decode(__j,__t,__n);\
}

#endif /* __COM_SERIALIZER_H__ */

