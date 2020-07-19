#ifndef __BCP_MEM_H__
#define __BCP_MEM_H__

#include "bcp_com.h"

/*
flag:
  -2=delete all
  -1=delete
   0=update
   1=add
   2=add all
if flag is -2 or 2, key will be empty
*/
typedef void (*cb_mem_data_change)(std::string key, int flag);

void bcp_mem_set(const char* key, const char val);
void bcp_mem_set(const char* key, const bool val);
void bcp_mem_set(const char* key, const double val);
void bcp_mem_set(const char* key, const int8 val);
void bcp_mem_set(const char* key, const int16 val);
void bcp_mem_set(const char* key, const int32 val);
void bcp_mem_set(const char* key, const int64 val);
void bcp_mem_set(const char* key, const uint8 val);
void bcp_mem_set(const char* key, const uint16 val);
void bcp_mem_set(const char* key, const uint32 val);
void bcp_mem_set(const char* key, const uint64 val);

void bcp_mem_set(const char* key, const std::string& val);
void bcp_mem_set(const char* key, const char* val);
void bcp_mem_set(const char* key, const uint8* val, int val_size);
void bcp_mem_set(const char* key, ByteArray& bytes);

void bcp_mem_remove(const char* key);
void bcp_mem_remove_all();
bool bcp_mem_is_key_exist(const char* key);

bool bcp_mem_get_bool(const char* key, bool default_val = false);
char bcp_mem_get_char(const char* key, char default_val = ' ');
double bcp_mem_get_double(const char* key, double default_val = 0.0f);
int8 bcp_mem_get_int8(const char* key, int8 default_val = 0);
int16 bcp_mem_get_int16(const char* key, int16 default_val = 0);
int32 bcp_mem_get_int32(const char* key, int32 default_val = 0);
int64 bcp_mem_get_int64(const char* key, int64 default_val = 0);
uint8 bcp_mem_get_uint8(const char* key, uint8 default_val = 0);
uint16 bcp_mem_get_uint16(const char* key, uint16 default_val = 0);
uint32 bcp_mem_get_uint32(const char* key, uint32 default_val = 0);
uint64 bcp_mem_get_uint64(const char* key, uint64 default_val = 0);
std::string bcp_mem_get_string(const char* key, std::string default_val = "");
ByteArray bcp_mem_get_bytes(const char* key);

std::string bcp_mem_to_json();
void bcp_mem_from_json(std::string json);

bool bcp_mem_to_file(std::string file);
void bcp_mem_from_file(std::string file);

Message bcp_mem_to_message();
void bcp_mem_from_message(Message msg);

void bcp_mem_add_notify(const char* key, cb_mem_data_change cb);
void bcp_mem_remove_notify(const char* key, cb_mem_data_change cb);

#endif /* __BCP_MEM_H__ */
