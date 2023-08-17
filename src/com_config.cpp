#include "com_base.h"
#include "com_file.h"
#include "com_log.h"
#include "com_config.h"

CPPConfig::CPPConfig()
{
    ini.SetUnicode();
}

CPPConfig::CPPConfig(const char* file)
{
    ini.SetUnicode();
    load(file);
}

CPPConfig::CPPConfig(const CPPConfig& config)
{
    if(this == &config)
    {
        return;
    }
    this->load(config.getConfigFile().c_str());
}

CPPConfig::~CPPConfig()
{
    ini.Reset();
}

CPPConfig& CPPConfig::operator=(const CPPConfig& config)
{
    if(this == &config)
    {
        return *this;
    }

    this->load(config.getConfigFile().c_str());
    return *this;
}

bool CPPConfig::load(const char* file)
{
    if(file == NULL)
    {
        return false;
    }
    this->file_config = file;
    std::lock_guard<std::mutex> lck(mutex_ini);
    this->ini.Reset();
#if defined(_WIN32) || defined(_WIN64)
	SI_Error ret = ini.LoadFile(com_string_utf8_to_ansi(file).c_str());
#else
	SI_Error ret = ini.LoadFile(file);
#endif
    return (ret == SI_OK);
}

bool CPPConfig::loadFromString(const char* value)
{
    if(value == NULL)
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(mutex_ini);
    this->file_config.clear();
    this->ini.Reset();
	SI_Error ret = ini.LoadData(value);
	if (ret != SI_OK)
	{
		LOG_E("failed to load config from data: %s", value);
		return false;
	}
    return true;
}

bool CPPConfig::save()
{
    return saveAs(file_config.c_str());
}

bool CPPConfig::saveAs(const char* config_file)
{
    if(config_file == NULL || config_file[0] == '\0')
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(mutex_ini);
#if defined(_WIN32) || defined(_WIN64)
	SI_Error ret = ini.SaveFile(com_string_utf8_to_ansi(config_file).c_str(), false);
#else
	SI_Error ret = ini.SaveFile(config_file, false);
#endif
    return (ret == SI_OK);
}

Message CPPConfig::toMessage()
{
    Message msg;
    std::vector<std::string> sections = getAllSections();
    for(size_t i = 0; i < sections.size(); i++)
    {
        std::vector<std::string> keys = getAllKeysBySection(sections[i].c_str());
        if(keys.empty() == false)
        {
            for(size_t j = 0; j < keys.size(); j++)
            {
                std::string value = getString(sections[i].c_str(), keys[j].c_str());
                if(value.empty() == false)
                {
                    msg.set(com_string_format("%s.%s", sections[i].c_str(), keys[j].c_str()).c_str(), value);
                }
            }
        }
    }

    return msg;
}

std::string CPPConfig::getConfigFile() const
{
    return file_config;
}

std::string CPPConfig::getString(const char* section, const char* key, std::string default_val)
{
    if(key == NULL)
    {
        return default_val;
    }
    if(section == NULL)
    {
        section = "";
    }

    std::lock_guard<std::mutex> lck(mutex_ini);
    const char* pv = ini.GetValue(section, key, default_val.c_str());
    if(pv == NULL)
    {
        return default_val;
    }
    return pv;
}

int8 CPPConfig::getInt8(const char* section, const char* key, int8 default_val)
{
    return (int8)getInt64(section, key, (int64)default_val);
}

uint8 CPPConfig::getUInt8(const char* section, const char* key, uint8 default_val)
{
    return (uint8)getUInt64(section, key, (uint64)default_val);
}

int16 CPPConfig::getInt16(const char* section, const char* key, int16 default_val)
{
    return (int16)getInt64(section, key, (int64)default_val);
}

uint16 CPPConfig::getUInt16(const char* section, const char* key, uint16 default_val)
{
    return (uint16)getUInt64(section, key, (uint64)default_val);
}

int32 CPPConfig::getInt32(const char* section, const char* key, int32 default_val)
{
    return (int32)getInt64(section, key, (int64)default_val);
}

uint32 CPPConfig::getUInt32(const char* section, const char* key, uint32 default_val)
{
    return (uint32)getUInt64(section, key, (uint64)default_val);
}

int64 CPPConfig::getInt64(const char* section, const char* key, int64 default_val)
{
    std::string val = getString(section, key);
    if(val.empty())
    {
        return default_val;
    }
    if(com_string_start_with(val.c_str(), "0x") || com_string_start_with(val.c_str(), "0X"))
    {
        return strtoll(val.c_str(), NULL, 16);
    }
    else
    {
        return strtoll(val.c_str(), NULL, 10);
    }
}

uint64 CPPConfig::getUInt64(const char* section, const char* key, uint64 default_val)
{
    std::string val = getString(section, key);
    if(val.empty())
    {
        return default_val;
    }
    if(com_string_start_with(val.c_str(), "0x") || com_string_start_with(val.c_str(), "0X"))
    {
        return strtoull(val.c_str(), NULL, 16);
    }
    else
    {
        return strtoull(val.c_str(), NULL, 10);
    }
}

bool CPPConfig::getBool(const char* section, const char* key, bool default_val)
{
    if(key == NULL)
    {
        return default_val;
    }
    if(section == NULL)
    {
        section = "";
    }

    std::lock_guard<std::mutex> lck(mutex_ini);
    return ini.GetBoolValue(section, key, default_val);
}

float CPPConfig::getFloat(const char* section, const char* key, float default_val)
{
    std::string val = getString(section, key);
    if(val.empty())
    {
        return default_val;
    }
    return strtof(val.c_str(), NULL);
}

double CPPConfig::getDouble(const char* section, const char* key, double default_val)
{
    std::string val = getString(section, key);
    if(val.empty())
    {
        return default_val;
    }
    return strtod(val.c_str(), NULL);
}

bool CPPConfig::isSectionExist(const char* section)
{
    if(section == NULL)
    {
        return false;
    }

    std::lock_guard<std::mutex> lck(mutex_ini);
    CSimpleIniA::TNamesDepend sections;
    ini.GetAllSections(sections);
    for(const CSimpleIniA::Entry& it : sections)
    {
        if(com_string_equal(it.pItem, section))
        {
            return true;
        }
    }
    return false;
}

bool CPPConfig::isKeyExist(const char* section, const char* key)
{
    std::string val = getString(section, key);
    return (val.empty() == false);
}

std::vector<std::string> CPPConfig::getAllSections()
{
    std::vector<std::string> result;

    std::lock_guard<std::mutex> lck(mutex_ini);
    CSimpleIniA::TNamesDepend sections;
    ini.GetAllSections(sections);
    for(const CSimpleIniA::Entry& it : sections)
    {
        if(it.pItem != NULL)
        {
            result.push_back(it.pItem);
        }
    }
    return result;
}

std::vector<std::string> CPPConfig::getAllKeysBySection(const char* section)
{
    std::vector<std::string> result;
    if(section == NULL)
    {
        section = "";
    }

    std::lock_guard<std::mutex> lck(mutex_ini);
    CSimpleIniA::TNamesDepend keys;
    ini.GetAllKeys(section, keys);

    for(const CSimpleIniA::Entry& it : keys)
    {
        if(it.pItem != NULL)
        {
            result.push_back(it.pItem);
        }
    }

    return result;
}

int CPPConfig::getSectionKeyCount(const char* section)
{
    if(section == NULL)
    {
        section = "";
    }

    std::lock_guard<std::mutex> lck(mutex_ini);
    CSimpleIniA::TNamesDepend keys;
    ini.GetAllKeys(section, keys);
    return keys.size();
}

bool CPPConfig::set(const char* section, const char* key, std::string val)
{
    return set(section, key, val.c_str());
}

bool CPPConfig::set(const char* section, const char* key, char* val)
{
    return set(section, key, (const char*)val);
}

bool CPPConfig::set(const char* section, const char* key, const char* val)
{
    if(key == NULL || val == NULL)
    {
        return false;
    }

    if(section == NULL)
    {
        section = "";
    }

    std::lock_guard<std::mutex> lck(mutex_ini);
    ini.SetValue(section, key, val);
    return true;
}

bool CPPConfig::removeSection(const char* section)
{
    if(section == NULL)
    {
        section = "";
    }
    std::lock_guard<std::mutex> lck(mutex_ini);
    ini.Delete(section, NULL);
    return true;
}

bool CPPConfig::removeItem(const char* section, const char* key)
{
    if(key == NULL)
    {
        return false;
    }
    std::string tag;
    if(section == NULL)
    {
        section = "";
    }

    std::lock_guard<std::mutex> lck(mutex_ini);
    ini.Delete(section, key);
    return true;
}

