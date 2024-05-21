#ifndef __COM_BASE64_H__
#define __COM_BASE64_H__

#include "com_base.h"
#include "com_serializer.h"

class COM_EXPORT ComBase64
{
public:
    ComBase64();
    virtual ~ComBase64();
    static std::string Encode(const CPPBytes& bytes);
    static std::string Encode(const uint8* data, int data_size);
    static CPPBytes Decode(const char* base64_data);
private:
    static bool IsBase64(char c);
};

typedef COM_EXPORT ComBase64 DEPRECATED("Use ComBase64 instead") Base64;

#endif /* __COM_BASE64_H__ */

