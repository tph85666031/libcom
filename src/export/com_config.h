#ifndef __COM_CONFIG_H__
#define __COM_CONFIG_H__

#include <string>
#include "com_base.h"
#include "SimpleIni.h"

//注意:能读取无GROUP的ini文件，但不能存储无GROUP的数据到文件
class COM_EXPORT ComConfig
{
public:
    ComConfig();
    ComConfig(const char* file);
    ComConfig(const ComConfig& config);
    virtual ~ComConfig();

    ComConfig& operator=(const ComConfig& config);
    
    bool load(const char* file);
    bool loadFromString(const char* value);
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
//typedef ComConfig DEPRECATED("Use ComConfig instead") CPPConfig;

#endif /* __COM_CONFIG_H__ */
