#ifndef __COM_KV_RESULT_H__
#define __COM_KV_RESULT_H__

#include <string>
#include "com_base.h"

class KVResult
{
public:
    KVResult(const char* key, uint8* data, int data_size);
    KVResult(const KVResult& result);
    KVResult& operator=(const KVResult& result);
    virtual ~KVResult();
public:
    std::string key;
    int data_size = 0;
    uint8* data = NULL;
};

#endif /* __COM_KV_RESULT_H__ */
