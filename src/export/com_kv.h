#ifndef __COM_KV_H__
#define __COM_KV_H__

#include <vector>
#include "com_base.h"

class COM_EXPORT KVResult
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

COM_EXPORT void* com_kv_batch_start(const char* file);
COM_EXPORT bool com_kv_batch_set(void* handle, const char* key, const uint8* data, int data_size);
COM_EXPORT bool com_kv_batch_set(void* handle, const char* key, const char* data);
COM_EXPORT bool com_kv_batch_remove(void* handle, const char* key);
COM_EXPORT void com_kv_batch_stop(void* handle, bool rollback = false);

COM_EXPORT bool com_kv_set(const char* file, const char* key, const uint8* data, int data_size);
COM_EXPORT bool com_kv_set(const char* file, const char* key, const char* value);

COM_EXPORT bool com_kv_set(const char* file, const char* key, bool value);

COM_EXPORT bool com_kv_set(const char* file, const char* key, int8 value);
COM_EXPORT bool com_kv_set(const char* file, const char* key, uint8 value);

COM_EXPORT bool com_kv_set(const char* file, const char* key, int16 value);
COM_EXPORT bool com_kv_set(const char* file, const char* key, uint16 value);

COM_EXPORT bool com_kv_set(const char* file, const char* key, int32 value);
COM_EXPORT bool com_kv_set(const char* file, const char* key, uint32 value);

COM_EXPORT bool com_kv_set(const char* file, const char* key, int64 value);
COM_EXPORT bool com_kv_set(const char* file, const char* key, uint64 value);

COM_EXPORT bool com_kv_set(const char* file, const char* key, double value);

COM_EXPORT int com_kv_count(const char* file);
 
COM_EXPORT bool com_kv_exist(const char* file, const char* key);

COM_EXPORT void com_kv_remove_front(const char* file, int count);
COM_EXPORT void com_kv_remove_tail(const char* file, int count);
COM_EXPORT void com_kv_remove_all(const char* file);
COM_EXPORT bool com_kv_remove(const char* file, const char* key);

COM_EXPORT std::vector<KVResult> com_kv_get_front(const char* file, int count);
COM_EXPORT std::vector<KVResult> com_kv_get_tail(const char* file, int count);

COM_EXPORT std::vector<std::string> com_kv_get_all_keys(const char* file);

COM_EXPORT CPPBytes com_kv_get_bytes(const char* file, const char* key);

COM_EXPORT bool com_kv_get_string(const char* file, const char* key, char* value, int value_size);
COM_EXPORT std::string com_kv_get_string(const char* file, const char* key, const char* default_value = NULL);

COM_EXPORT bool com_kv_get_bool(const char* file, const char* key, bool default_value = false);

COM_EXPORT int8 com_kv_get_int8(const char* file, const char* key, int8 default_value = 0);
COM_EXPORT uint8 com_kv_get_uint8(const char* file, const char* key, uint8 default_value = 0);

COM_EXPORT int16 com_kv_get_int16(const char* file, const char* key, int16 default_value = 0);
COM_EXPORT uint16 com_kv_get_uint16(const char* file, const char* key, uint16 default_value = 0);

COM_EXPORT int32 com_kv_get_int32(const char* file, const char* key, int32 default_value = 0);
COM_EXPORT uint32 com_kv_get_uint32(const char* file, const char* key, uint32 default_value = 0);

COM_EXPORT int64 com_kv_get_int64(const char* file, const char* key, int64 default_value = 0);
COM_EXPORT uint64 com_kv_get_uint64(const char* file, const char* key, uint64 default_value = 0);

COM_EXPORT double com_kv_get_double(const char* file, const char* key, double default_value = 0.0f);

#endif /* __COM_KV_H__ */
