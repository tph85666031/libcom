#ifndef __COM_MEM_H__
#define __COM_MEM_H__

#include "com_base.h"

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

void com_mem_set(const char* key, const char val);
void com_mem_set(const char* key, const bool val);
void com_mem_set(const char* key, const double val);
void com_mem_set(const char* key, const int8_t val);
void com_mem_set(const char* key, const int16_t val);
void com_mem_set(const char* key, const int32_t val);
void com_mem_set(const char* key, const int64_t val);
void com_mem_set(const char* key, const uint8_t val);
void com_mem_set(const char* key, const uint16_t val);
void com_mem_set(const char* key, const uint32_t val);
void com_mem_set(const char* key, const uint64_t val);

void com_mem_set(const char* key, const std::string& val);
void com_mem_set(const char* key, const char* val);
void com_mem_set(const char* key, const uint8_t* val, int val_size);
void com_mem_set(const char* key, CPPBytes& bytes);

void com_mem_remove(const char* key);
void com_mem_remove_all();
bool com_mem_is_key_exist(const char* key);

bool com_mem_get_bool(const char* key, bool default_val = false);
char com_mem_get_char(const char* key, char default_val = ' ');
double com_mem_get_double(const char* key, double default_val = 0.0f);
int8_t com_mem_get_int8(const char* key, int8_t default_val = 0);
int16_t com_mem_get_int16(const char* key, int16_t default_val = 0);
int32_t com_mem_get_int32(const char* key, int32_t default_val = 0);
int64_t com_mem_get_int64(const char* key, int64_t default_val = 0);
uint8_t com_mem_get_uint8(const char* key, uint8_t default_val = 0);
uint16_t com_mem_get_uint16(const char* key, uint16_t default_val = 0);
uint32_t com_mem_get_uint32(const char* key, uint32_t default_val = 0);
uint64_t com_mem_get_uint64(const char* key, uint64_t default_val = 0);
std::string com_mem_get_string(const char* key, std::string default_val = "");
CPPBytes com_mem_get_bytes(const char* key);

std::string com_mem_to_json();
void com_mem_from_json(std::string json);

bool com_mem_to_file(std::string file);
void com_mem_from_file(std::string file);

Message com_mem_to_message();
void com_mem_from_message(Message msg);

void com_mem_add_notify(const char* key, cb_mem_data_change cb);
void com_mem_remove_notify(const char* key, cb_mem_data_change cb);

void InitMemDataSyncManager();
void UninitMemDataSyncManager();

#endif /* __COM_MEM_H__ */
