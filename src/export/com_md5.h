#ifndef __COM_MD5_H__
#define __COM_MD5_H__

#include "com_base.h"

#define CPPMD5_SIZE        16

typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];
} CPPMD5_CTX;

class COM_EXPORT CPPMD5
{
public:
    CPPMD5();
    virtual ~CPPMD5();
    void append(const void* data, int data_size);
    void appendFile(const char* file_path, int64 offset = 0, int64 size = -1);
    CPPBytes finish();

    static CPPBytes Digest(const void* data, int data_size);
    static CPPBytes Digest(const char* file_path);
private:
    CPPMD5_CTX ctx;
};

#endif /* __COM_MD5_H__ */

