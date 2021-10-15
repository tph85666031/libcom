#ifndef __COM_SERIALIZER_H__
#define __COM_SERIALIZER_H__

#include "com_base.h"
#include <vector>

class Serializer
{
public:
    Serializer();
    Serializer(uint8_t* data, int data_size);
    ~Serializer();
    Serializer& append(bool val);
    Serializer& append(char val);
    Serializer& append(uint8_t val);
    Serializer& append(int8_t val);
    Serializer& append(uint16_t val);
    Serializer& append(int16_t val);
    Serializer& append(uint32_t val);
    Serializer& append(int32_t val);
    Serializer& append(uint64_t val);
    Serializer& append(int64_t val);

    Serializer& append(const std::string val, int val_size = -1);
    Serializer& append(const char* val, int val_size = -1);
    Serializer& append(const uint8_t* val, int val_size);
    Serializer& append(CPPBytes& bytes);

    int detach(bool& val);
    int detach(char& val);
    int detach(uint8_t& val);
    int detach(int8_t& val);
    int detach(uint16_t& val);
    int detach(int16_t& val);
    int detach(uint32_t& val);
    int detach(int32_t& val);
    int detach(uint64_t& val);
    int detach(int64_t& val);

    int detach(std::string& val, int val_size = -1);
    int detach(char* val, int val_size);
    int detach(uint8_t* val, int val_size);
    int detach(CPPBytes& bytes, int val_size);

    void clear();
    void detachReset();
    int getDetachRemainSize();
    CPPBytes toBytes();

    uint8_t* getData();
    int getDataSize();

    Serializer& enableByteOrderModify();
    Serializer& disableByteOrderModify();

private:
    std::vector<uint8_t> buf;
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

#define META(...) \
CPPBytes toBytes(){\
    auto t = std::make_tuple(__VA_ARGS__);\
    Serializer s;\
    __tuple_serializer(s,t);\
    return s.toBytes();\
}

#endif /* __COM_SERIALIZER_H__ */

