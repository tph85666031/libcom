#ifndef __BCP_URL_H__
#define __BCP_URL_H__

#include "bcp_com.h"

class URL final
{
public:
    URL();
    ~URL();
    static std::string Encode(const char* str);
    static std::string Decode(const char* str);
private:
    static uint8 toHex(uint8 x);
    static uint8 fromHex(uint8 x);
};

#endif /* __BCP_URL_H__ */

