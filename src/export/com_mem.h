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
#define DATA_SYNC_CLEARED -2
#define DATA_SYNC_REMOVED -1
#define DATA_SYNC_NEW     0
#define DATA_SYNC_UPDATED 1
#define DATA_SYNC_RELOAD  2

typedef void (*cb_mem_data_change)(const std::string& key, int flag, void* ctx);

COM_EXPORT void com_mem_set(const char* key, const char val);
COM_EXPORT void com_mem_set(const char* key, const bool val);
COM_EXPORT void com_mem_set(const char* key, const double val);
COM_EXPORT void com_mem_set(const char* key, const int8 val);
COM_EXPORT void com_mem_set(const char* key, const int16 val);
COM_EXPORT void com_mem_set(const char* key, const int32 val);
COM_EXPORT void com_mem_set(const char* key, const int64 val);
COM_EXPORT void com_mem_set(const char* key, const uint8 val);
COM_EXPORT void com_mem_set(const char* key, const uint16 val);
COM_EXPORT void com_mem_set(const char* key, const uint32 val);
COM_EXPORT void com_mem_set(const char* key, const uint64 val);

COM_EXPORT void com_mem_set(const char* key, const std::string& val);
COM_EXPORT void com_mem_set(const char* key, const char* val);
COM_EXPORT void com_mem_set(const char* key, const uint8* val, int val_size);
COM_EXPORT void com_mem_set(const char* key, ComBytes& bytes);

COM_EXPORT void com_mem_remove(const char* key);
COM_EXPORT void com_mem_clear();
COM_EXPORT bool com_mem_is_key_exist(const char* key);

COM_EXPORT bool com_mem_get_bool(const char* key, bool default_val = false);
COM_EXPORT char com_mem_get_char(const char* key, char default_val = ' ');
COM_EXPORT double com_mem_get_double(const char* key, double default_val = 0.0f);
COM_EXPORT int8 com_mem_get_int8(const char* key, int8 default_val = 0);
COM_EXPORT int16 com_mem_get_int16(const char* key, int16 default_val = 0);
COM_EXPORT int32 com_mem_get_int32(const char* key, int32 default_val = 0);
COM_EXPORT int64 com_mem_get_int64(const char* key, int64 default_val = 0);
COM_EXPORT uint8 com_mem_get_uint8(const char* key, uint8 default_val = 0);
COM_EXPORT uint16 com_mem_get_uint16(const char* key, uint16 default_val = 0);
COM_EXPORT uint32 com_mem_get_uint32(const char* key, uint32 default_val = 0);
COM_EXPORT uint64 com_mem_get_uint64(const char* key, uint64 default_val = 0);
COM_EXPORT std::string com_mem_get_string(const char* key, std::string default_val = "");
COM_EXPORT ComBytes com_mem_get_bytes(const char* key);

COM_EXPORT std::string com_mem_to_json();
COM_EXPORT void com_mem_from_json(std::string json);

COM_EXPORT bool com_mem_to_file(std::string file);
COM_EXPORT void com_mem_from_file(std::string file);

COM_EXPORT Message com_mem_to_message();
COM_EXPORT void com_mem_from_message(Message msg);

COM_EXPORT void com_mem_add_notify(const char* key, cb_mem_data_change cb, void* ctx = NULL);
COM_EXPORT void com_mem_remove_notify(const char* key, cb_mem_data_change cb = NULL);

COM_EXPORT void com_mem_add_notify(const char* key, const char* task_name);
COM_EXPORT void com_mem_remove_notify(const char* key, const char* task_name = NULL);

#endif /* __COM_MEM_H__ */

