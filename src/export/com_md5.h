#ifndef __COM_MD5_H__
#define __COM_MD5_H__

#define CPPMD5_SIZE        16

typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];
} CPPMD5_CTX;

class CPPMD5 final
{
public:
    CPPMD5();
    ~CPPMD5();
    void append(const uint8* data, uint32 dataSize);
    void appendFile(const char* filePath);
    ByteArray finish();
private:
    CPPMD5_CTX ctx;
};

#endif /* __COM_MD5_H__ */

