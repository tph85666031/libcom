#ifndef __COM_MD5_H__
#define __COM_MD5_H__

#define CPPMD5_SIZE        16

typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];
} CPPMD5_CTX;

class CPPMD5
{
public:
    CPPMD5();
    virtual ~CPPMD5();
    void append(const uint8_t* data, uint32_t dataSize);
    void appendFile(const char* filePath);
    CPPBytes finish();
private:
    CPPMD5_CTX ctx;
};

#endif /* __COM_MD5_H__ */

