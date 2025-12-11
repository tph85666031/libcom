#ifndef __COM_MD5_H__
#define __COM_MD5_H__

#include "com_base.h"

#define COMMD5_SIZE        16

typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];
} COMMD5_CTX;

class COM_EXPORT ComMD5
{
public:
    ComMD5();
    virtual ~ComMD5();
    void append(const void* data, int data_size);
    void appendFile(const char* file_path, int64 offset = 0, int64 size = -1);
    ComBytes finish();

    static ComBytes Digest(const void* data, int data_size);
    static ComBytes Digest(const char* file_path);
private:
    COMMD5_CTX ctx;
};

#endif /* __COM_MD5_H__ */

