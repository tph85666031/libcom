#include "com_base64.h"

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

Base64::Base64()
{
}

Base64::~Base64()
{
}

bool Base64::isBase64(char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Base64::Encode(CPPBytes& bytes)
{
    return Encode(bytes.getData(), bytes.getDataSize());
}

std::string Base64::Encode(const uint8_t* data, int data_size)
{
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    std::string encode_data;

    if (data == NULL || data_size <= 0)
    {
        return encode_data;
    }

    while (data_size--)
    {
        char_array_3[i++] = *(data++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4) ; i++)
            {
                encode_data += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
        {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
        {
            encode_data += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3))
        {
            encode_data += '=';
        }
    }

    return encode_data;
}

CPPBytes Base64::Decode(const char* base64_data)
{
    CPPBytes bytes;
    if (base64_data == NULL)
    {
        return bytes;
    }
    std::string base64String = base64_data;
    int in_len = base64String.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    uint8_t char_array_4[4];
    uint8_t char_array_3[3];

    if (in_len % 4 > 0)
    {
        base64String.append(4 - in_len % 4, '=');
    }

    while (in_len-- && (base64String[in_] != '=') && isBase64(base64String[in_]))
    {
        char_array_4[i++] = base64String[in_];
        in_++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
            {
                char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

            for (i = 0; (i < 3); i++)
            {
                bytes.append(char_array_3[i]);
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = 0; j < i; j++)
        {
            char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++)
        {
            bytes.append(char_array_3[j]);
        }
    }

    return bytes;
}

