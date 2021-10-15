#include "com_url.h"

URL::URL()
{
}

URL::~URL()
{
}

uint8_t URL::toHex(uint8_t x)
{
    return  x > 9 ? x + 55 : x + 48;
}

uint8_t URL::fromHex(uint8_t x)
{
    uint8_t y = ' ';
    if (x >= 'A' && x <= 'Z')
    {
        y = x - 'A' + 10;
    }
    else if (x >= 'a' && x <= 'z')
    {
        y = x - 'a' + 10;
    }
    else if (x >= '0' && x <= '9')
    {
        y = x - '0';
    }
    else
    {
    }
    return y;
}

std::string URL::Encode(const char* str)
{
    std::string encodeData;
    if (str == NULL)
    {
        return encodeData;
    }
    size_t length = strlen(str);
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
        {
            encodeData += str[i];
        }
        else if (str[i] == ' ')
        {
            encodeData += "+";
        }
        else
        {
            encodeData += '%';
            encodeData += toHex((unsigned char)str[i] >> 4);
            encodeData += toHex((unsigned char)str[i] % 16);
        }
    }
    return encodeData;
}

std::string URL::Decode(const char* str)
{
    std::string decodeData;
    if (str == NULL)
    {
        return decodeData;
    }
	size_t length = strlen(str);
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+')
        {
            decodeData += ' ';
        }
        else if (str[i] == '%')
        {
            if (i + 2 < length)
            {
                unsigned char high = fromHex((unsigned char)str[++i]);
                unsigned char low = fromHex((unsigned char)str[++i]);
                decodeData += high * 16 + low;
            }
        }
        else
        {
            decodeData += str[i];
        }
    }
    return decodeData;
}

