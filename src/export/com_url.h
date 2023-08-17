#ifndef __COM_URL_H__
#define __COM_URL_H__

#include "com_base.h"

class COM_EXPORT URL
{
public:
    URL();
    virtual ~URL();
    static std::string Encode(const char* str);
    static std::string Decode(const char* str);
private:
    static uint8 toHex(uint8 x);
    static uint8 fromHex(uint8 x);
};

#endif /* __COM_URL_H__ */

