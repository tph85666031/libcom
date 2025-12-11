#ifndef __COM_BASE64_H__
#define __COM_BASE64_H__

#include "com_base.h"
#include "com_serializer.h"

class COM_EXPORT ComBase64
{
public:
    ComBase64();
    virtual ~ComBase64();
    void show();
    static std::string Encode(const ComBytes& bytes);
    static std::string Encode(const uint8* data, int data_size);
    static ComBytes Decode(const char* base64_data);
private:
    static bool IsBase64(char c);
};

#endif /* __COM_BASE64_H__ */

