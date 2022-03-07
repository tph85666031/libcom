#ifndef __COM_CONFIG_H__
#define __COM_CONFIG_H__

#include <string>
#include "com_base.h"
#include "SimpleIni.h"

//注意:能读取无GROUP的ini文件，但不能存储无GROUP的数据到文件
class CPPConfig
{
public:
    CPPConfig();
    CPPConfig(const char* file);
    virtual ~CPPConfig();
    bool load(const char* file);
    bool save();
    bool saveAs(const char* config_file);
    Message toMessage();

    std::string getString(const char* section, const char* key, std::string default_val = std::string());
    int8 getInt8(const char* section, const char* key, int8 default_val = 0);
    uint8 getUInt8(const char* section, const char* key, uint8 default_val = 0);
    int16 getInt16(const char* section, const char* key, int16 default_val = 0);
    uint16 getUInt16(const char* section, const char* key, uint16 default_val = 0);
    int32 getInt32(const char* section, const char* key, int32 default_val = 0);
    uint32 getUInt32(const char* section, const char* key, uint32 default_val = 0);
    int64 getInt64(const char* section, const char* key, int64 default_val = 0);
	uint64 getUInt64(const char* section, const char* key, uint64 default_val = 0);
    bool getBool(const char* section, const char* key, bool default_val = false);
    float getFloat(const char* section, const char* key, float default_val = 0.0f);
    double getDouble(const char* section, const char* key, double default_val = 0.0f);
    bool isSectionExist(const char* section);
    bool isKeyExist(const char* section, const char* key);
    int getSectionKeyCount(const char* section);
    std::vector<std::string> getAllSections();
    std::vector<std::string> getAllKeysBySection(const char* section);
    bool removeSection(const char* section);
    bool removeItem(const char* section, const char* key);

    template <class T>
    bool set(const char* section, const char* key, T val)
    {
        return set(section, key, std::to_string(val));
    };
    bool set(const char* section, const char* key, const char* val);
    bool set(const char* section, const char* key, char* val);
    bool set(const char* section, const char* key, std::string val);
private:
    std::string getConfigFile() const;
private:
    std::mutex mutex_ini;
    CSimpleIniA ini;
    std::string file_config;
};

bool com_global_config_load(const char* config_file);
bool com_global_config_save();
bool com_global_config_save_as(const char* config_file);
Message com_global_config_to_message();
bool com_global_config_is_section_exist(const char* section);
bool com_global_config_is_key_exist(const char* section, const char* key);
bool com_global_config_set(const char* section, const char* key, const bool val);
bool com_global_config_set(const char* section, const char* key, const double val);
bool com_global_config_set(const char* section, const char* key, const int8 val);
bool com_global_config_set(const char* section, const char* key, const int16 val);
bool com_global_config_set(const char* section, const char* key, const int32 val);
bool com_global_config_set(const char* section, const char* key, const int64 val);
bool com_global_config_set(const char* section, const char* key, const uint8 val);
bool com_global_config_set(const char* section, const char* key, const uint16 val);
bool com_global_config_set(const char* section, const char* key, const uint32 val);
bool com_global_config_set(const char* section, const char* key, const uint64 val);
bool com_global_config_set(const char* section, const char* key, const std::string& val);
bool com_global_config_set(const char* section, const char* key, const char* val);
bool com_global_config_remove(const char* section, const char* key);
bool com_global_config_get_bool(const char* section, const char* key, bool default_val = false);
double com_global_config_get_double(const char* section, const char* key, double default_val = 0.0f);
int8 com_global_config_get_int8(const char* section, const char* key, int8 default_val = 0);
int16 com_global_config_get_int16(const char* section, const char* key, int16 default_val = 0);
int32 com_global_config_get_int32(const char* section, const char* key, int32 default_val = 0);
int64 com_global_config_get_int64(const char* section, const char* key, int64 default_val = 0);
uint8 com_global_config_get_uint8(const char* section, const char* key, uint8 default_val = 0);
uint16 com_global_config_get_uint16(const char* section, const char* key, uint16 default_val = 0);
uint32 com_global_config_get_uint32(const char* section, const char* key, uint32 default_val = 0);
uint64 com_global_config_get_uint64(const char* section, const char* key, uint64 default_val = 0);
std::string com_global_config_get_string(const char* section, const char* key, std::string default_val = std::string());


#endif /* __COM_CONFIG_H__ */
