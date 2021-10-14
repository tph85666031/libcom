#ifndef __COM_CONFIG_H__
#define __COM_CONFIG_H__

#include <string>
#include "com_base.h"

class CPPConfig
{
public:
    CPPConfig();
    CPPConfig(bool thread_safe);
    CPPConfig(const char* file, bool thread_safe = false);
    virtual ~CPPConfig();
    CPPConfig(CPPConfig&) = delete;//赋值构造函数
    CPPConfig& operator=(CPPConfig& config) = delete; //拷贝构造函数
    bool load(const char* file);
    bool save();
    bool saveAs(const char* config_file);
    void enableAutoSave();
    void disableAutoSave();
    Message toMessage();
    std::string toString();

    std::string getString(const char* group, const char* key, std::string default_val = std::string());
    int8 getInt8(const char* group, const char* key, int8 default_val = 0);
    uint8 getUInt8(const char* group, const char* key, uint8 default_val = 0);
    int16 getInt16(const char* group, const char* key, int16 default_val = 0);
    uint16 getUInt16(const char* group, const char* key, uint16 default_val = 0);
    int32 getInt32(const char* group, const char* key, int32 default_val = 0);
    uint32 getUInt32(const char* group, const char* key, uint32 default_val = 0);
    int64 getInt64(const char* group, const char* key, int64 default_val = 0);
    uint64 getUInt64(const char* group, const char* key, uint64 default_val = 0);
    bool getBool(const char* group, const char* key, bool default_val = false);
    float getFloat(const char* group, const char* key, float default_val = 0.0f);
    double getDouble(const char* group, const char* key, double default_val = 0.0f);
    bool isGroupExist(const char* group_name);
    bool isKeyExist(const char* group_name, const char* key);
    int getGroupKeyCount(const char* group_name);
    std::vector<std::string> getAllGroups();
    std::vector<std::string> getAllKeysByGroup(const char* group_name);
    bool addGroup(const char* group_name);
    bool removeGroup(const char* group_name);
    bool removeItem(const char* group_name, const char* key);

    template <class T>
    bool set(const char* group, const char* key, T val)
    {
        return set(group, key, std::to_string(val));
    };
    bool set(const char* group, const char* key, const char* val);
    bool set(const char* group, const char* key, char* val);
    bool set(const char* group, const char* key, std::string val);
private:
    std::string getConfigFile() const;
    void lock();
    void unlock();
private:
    void* data = NULL;
    std::string config_file;
    std::atomic<bool> data_changed = {false};
    std::atomic<bool> auto_save = {true};

    bool thread_safe = false;
    std::mutex mutex_data;
};

bool com_global_config_load(const char* config_file);
bool com_global_config_save();
bool com_global_config_save_as(const char* config_file);
Message com_global_config_to_message();
bool com_global_config_is_group_exist(const char* group);
bool com_global_config_is_key_exist(const char* group, const char* key);
bool com_global_config_set(const char* group, const char* key, const bool val);
bool com_global_config_set(const char* group, const char* key, const double val);
bool com_global_config_set(const char* group, const char* key, const int8 val);
bool com_global_config_set(const char* group, const char* key, const int16 val);
bool com_global_config_set(const char* group, const char* key, const int32 val);
bool com_global_config_set(const char* group, const char* key, const int64 val);
bool com_global_config_set(const char* group, const char* key, const uint8 val);
bool com_global_config_set(const char* group, const char* key, const uint16 val);
bool com_global_config_set(const char* group, const char* key, const uint32 val);
bool com_global_config_set(const char* group, const char* key, const uint64 val);
bool com_global_config_set(const char* group, const char* key, const std::string& val);
bool com_global_config_set(const char* group, const char* key, const char* val);
bool com_global_config_remove(const char* group, const char* key);
bool com_global_config_get_bool(const char* group, const char* key, bool default_val = false);
double com_global_config_get_double(const char* group, const char* key, double default_val = 0.0f);
int8 com_global_config_get_int8(const char* group, const char* key, int8 default_val = 0);
int16 com_global_config_get_int16(const char* group, const char* key, int16 default_val = 0);
int32 com_global_config_get_int32(const char* group, const char* key, int32 default_val = 0);
int64 com_global_config_get_int64(const char* group, const char* key, int64 default_val = 0);
uint8 com_global_config_get_uint8(const char* group, const char* key, uint8 default_val = 0);
uint16 com_global_config_get_uint16(const char* group, const char* key, uint16 default_val = 0);
uint32 com_global_config_get_uint32(const char* group, const char* key, uint32 default_val = 0);
uint64 com_global_config_get_uint64(const char* group, const char* key, uint64 default_val = 0);
std::string com_global_config_get_string(const char* group, const char* key, std::string default_val = std::string());


#endif /* __COM_CONFIG_H__ */
