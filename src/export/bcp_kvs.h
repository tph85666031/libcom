#ifndef __BCP_KVS_H__
#define __BCP_KVS_H__

#include <vector>
#include "bcp_com.h"
#include "bcp_kv_result.h"

void* bcp_kvs_batch_start(const char* file);
bool bcp_kvs_batch_set(void* handle, const char* key, const uint8* data, int data_size);
bool bcp_kvs_batch_set(void* handle, const char* key, const char* data);
bool bcp_kvs_batch_remove(void* handle, const char* key);
void bcp_kvs_batch_stop(void* handle, bool rollback = false);

bool bcp_kvs_set(const char* file, const char* key, const uint8* data, int data_size);
bool bcp_kvs_set(const char* file, const char* key, const char* value);

bool bcp_kvs_set(const char* file, const char* key, bool value);

bool bcp_kvs_set(const char* file, const char* key, int8 value);
bool bcp_kvs_set(const char* file, const char* key, uint8 value);

bool bcp_kvs_set(const char* file, const char* key, int16 value);
bool bcp_kvs_set(const char* file, const char* key, uint16 value);

bool bcp_kvs_set(const char* file, const char* key, int32 value);
bool bcp_kvs_set(const char* file, const char* key, uint32 value);

bool bcp_kvs_set(const char* file, const char* key, int64 value);
bool bcp_kvs_set(const char* file, const char* key, uint64 value);

bool bcp_kvs_set(const char* file, const char* key, double value);

int bcp_kvs_count(const char* file);

bool bcp_kvs_exist(const char* file, const char* key);

void bcp_kvs_remove_front(const char* file, int count);
void bcp_kvs_remove_tail(const char* file, int count);
void bcp_kvs_remove_all(const char* file);
bool bcp_kvs_remove(const char* file, const char* key);

std::vector<KVResult> bcp_kvs_get_front(const char* file, int count);
std::vector<KVResult> bcp_kvs_get_tail(const char* file, int count);

std::vector<std::string> bcp_kvs_get_all_keys(const char* file);

ByteArray bcp_kvs_get_bytes(const char* file, const char* key);

bool bcp_kvs_get_string(const char* file, const char* key, char* value, int value_size);
std::string bcp_kvs_get_string(const char* file, const char* key, const char* default_value = NULL);

bool bcp_kvs_get_bool(const char* file, const char* key, bool default_value = false);

int8 bcp_kvs_get_int8(const char* file, const char* key, int8 default_value = 0);
uint8 bcp_kvs_get_uint8(const char* file, const char* key, uint8 default_value = 0);

int16 bcp_kvs_get_int16(const char* file, const char* key, int16 default_value = 0);
uint16 bcp_kvs_get_uint16(const char* file, const char* key, uint16 default_value = 0);

int32 bcp_kvs_get_int32(const char* file, const char* key, int32 default_value = 0);
uint32 bcp_kvs_get_uint32(const char* file, const char* key, uint32 default_value = 0);

int64 bcp_kvs_get_int64(const char* file, const char* key, int64 default_value = 0);
uint64 bcp_kvs_get_uint64(const char* file, const char* key, uint64 default_value = 0);

double bcp_kvs_get_double(const char* file, const char* key, double default_value = 0.0f);

#endif /* __BCP_KVS_H__ */
