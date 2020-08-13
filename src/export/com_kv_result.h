#ifndef __BCP_KV_RESULT_H__
#define __BCP_KV_RESULT_H__

#include <string>
#include "com_base.h"

class KVResult final
{
public:
    KVResult(const char* key, uint8* data, int data_size);
    KVResult(const KVResult& result);
    KVResult& operator=(const KVResult& result);
    ~KVResult();
public:
    std::string key;
    int data_size = 0;
    uint8* data = NULL;
};

#endif /* __BCP_KV_RESULT_H__ */
