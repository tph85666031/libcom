#ifndef __BCP_JT808_SERIALIZER_H__
#define __BCP_JT808_SERIALIZER_H__

#include "com_base.h"
#include <vector>

class Serializer
{
public:
    Serializer();
    Serializer(uint8* data, int data_size);
    ~Serializer();
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

    Serializer& append(const std::string val);
    Serializer& append(const char* val);
    Serializer& append(const char* val, int val_size);
    Serializer& append(const uint8* val, int val_size);
    Serializer& append(ByteArray& bytes);

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
    int detach(ByteArray& bytes, int val_size);

    void clear();
    void detachReset();
    int getDetachRemainSize();
    ByteArray toBytes();

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
        __struct_tuple_serializer<T, N-1>::serializer(s, t);
        auto val = std::get<N-1> (t);
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
ByteArray toBytes(){\
    auto t = std::make_tuple(__VA_ARGS__);\
    Serializer s;\
    __tuple_serializer(s,t);\
    return s.toBytes();\
}

#endif /* __BCP_JT808_SERIALIZER_H__ */

