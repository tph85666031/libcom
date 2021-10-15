#ifndef __COM_BASE64_H__
#define __COM_BASE64_H__

#include "com_base.h"
#include "com_serializer.h"

class Base64
{
public:
    Base64();
    virtual ~Base64();
    static std::string Encode(CPPBytes& bytes);
    static std::string Encode(const uint8_t* data, int data_size);
    static CPPBytes Decode(const char* base64_data);
private:
    static bool isBase64(char c);
};

#endif /* __COM_BASE64_H__ */

