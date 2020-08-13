#ifndef __BCP_BASE64_H__
#define __BCP_BASE64_H__

#include "com_com.h"
#include "com_serializer.h"

class Base64 final
{
public:
    Base64();
    ~Base64();
    static std::string Encode(ByteArray& bytes);
    static std::string Encode(const uint8* data, int data_size);
    static ByteArray Decode(const char* base64_data);
private:
    static bool isBase64(char c);
};

#endif /* __BCP_BASE64_H__ */

