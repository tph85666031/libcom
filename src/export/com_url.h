#ifndef __COM_URL_H__
#define __COM_URL_H__

#include "com_base.h"

class URL
{
public:
    URL();
    virtual ~URL();
    static std::string Encode(const char* str);
    static std::string Decode(const char* str);
private:
    static uint8_t toHex(uint8_t x);
    static uint8_t fromHex(uint8_t x);
};

#endif /* __COM_URL_H__ */

