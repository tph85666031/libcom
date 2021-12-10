#include "com_base.h"
#include "com_file.h"
#include "com_config.h"
#include "com_log.h"
#include "iniparser.h"

static CPPConfig global_config(true);

CPPConfig::CPPConfig()
{
    this->data = NULL;
    this->data_changed = false;
    this->auto_save = true;
    this->thread_safe = false;
    this->data = (void*)dictionary_new(0);
}

CPPConfig::CPPConfig(bool thread_safe)
{
    this->data = NULL;
    this->data_changed = false;
    this->auto_save = true;
    this->thread_safe = thread_safe;
    this->data = (void*)dictionary_new(0);
}

CPPConfig::CPPConfig(const char* file, bool thread_safe)
{
    this->data = NULL;
    this->data_changed = false;
    this->auto_save = true;
    this->thread_safe = thread_safe;
    if(file != NULL)
    {
        this->config_file = file;
        int file_type = com_file_type(file);
        if(file_type == FILE_TYPE_FILE)
        {
            load(file);
        }
        else if(file_type == FILE_TYPE_NOT_EXIST)
        {
            this->data = (void*)dictionary_new(0);
        }
        else
        {
            LOG_W("%s may be directory or link file", file);
        }
    }
}

#if 0
//存在多地方存储到文件的风险
CPPConfig::CPPConfig(CPPConfig& config)//赋值构造函数
{
    if(this->data != NULL)
    {
        iniparser_freedict((dictionary*)this->data);
        this->data = NULL;
    }
    this->config_file = config.config_file;
    this->config_file_swap = config.config_file_swap;
    this->config_file_swap_bak = config.config_file_swap_bak;
    this->auto_save = config.auto_save;
    this->data_changed = config.data_changed.load();
    this->safe_mode = config.safe_mode.load();
    this->safe_dir = config.safe_dir;
    this->data = (void*)dictionary_new(0);
    if(this->data != NULL)
    {
        std::vector<std::string> groups = config.getAllGroups();
        for(int i = 0; i < groups.size(); i++)
        {
            std::vector<std::string> keys = config.getAllKeysByGroup(groups[i].c_str());
            if(keys.empty() == false)
            {
                for(int j = 0; j < keys.size(); j++)
                {
                    std::string value = config.getString(groups[i].c_str(), keys[j].c_str());
                    if(value.empty() == false)
                    {
                        set(groups[i].c_str(), keys[j].c_str(), value);
                    }
                }
            }
        }
    }
}

//存在多地方存储到文件的风险
CPPConfig& CPPConfig::operator=(CPPConfig& config)//拷贝构造函数
{
    if(this == &config)
    {
        return *this;
    }
    if(this->data != NULL)
    {
        iniparser_freedict((dictionary*)this->data);
        this->data = NULL;
    }
    this->config_file = config.config_file;
    this->config_file_swap = config.config_file_swap;
    this->config_file_swap_bak = config.config_file_swap_bak;
    this->auto_save = config.auto_save;
    this->data_changed = config.data_changed.load();
    this->safe_mode = config.safe_mode.load();
    this->safe_dir = config.safe_dir;
    this->data = (void*)dictionary_new(0);
    if(this->data != NULL)
    {
        std::vector<std::string> groups = config.getAllGroups();
        for(int i = 0; i < groups.size(); i++)
        {
            std::vector<std::string> keys = config.getAllKeysByGroup(groups[i].c_str());
            if(keys.empty() == false)
            {
                for(int j = 0; j < keys.size(); j++)
                {
                    std::string value = config.getString(groups[i].c_str(), keys[j].c_str());
                    if(value.empty() == false)
                    {
                        set(groups[i].c_str(), keys[j].c_str(), value);
                    }
                }
            }
        }
    }
    return *this;
}
#endif

CPPConfig::~CPPConfig()
{
    if(auto_save)
    {
        save();
    }
    if(this->data != NULL)
    {
        iniparser_freedict((dictionary*)this->data);
        this->data = NULL;
    }
}

bool CPPConfig::load(const char* file)
{
    if(file == NULL)
    {
        return false;
    }
    this->config_file = file;
    this->data_changed = false;

    lock();
    void* data = (void*)iniparser_load(this->config_file.c_str());
    if(data == NULL)
    {
        unlock();
        return false;
    }
    if(this->data != NULL)
    {
        iniparser_freedict((dictionary*)this->data);
    }
    this->data = data;
    unlock();

    return true;
}


bool CPPConfig::save()
{
    if(data_changed == false)
    {
        return true;
    }

    if(config_file.empty() || config_file.length() <= 0)
    {
        LOG_W("config file name not set");
        return false;
    }

    bool ret = false;
    ret = saveAs(config_file.c_str());
    if(ret)
    {
        data_changed = false;
    }

    return ret;
}

bool CPPConfig::saveAs(const char* config_file)
{
    if(com_string_len(config_file) <= 0)
    {
        return false;
    }
    std::string val = toString();
    if(val.empty())
    {
        return false;
    }

    lock();
    FILE* file = com_file_open(config_file, "w+");
    if(file == NULL)
    {
        unlock();
        return false;
    }

    int ret = com_file_write(file, val.data(), val.size());
    if(ret != (int)val.size())
    {
        com_file_close(file);
        unlock();
        return false;
    }
    com_file_flush(file);
    com_file_sync(file);
    com_file_close(file);
    unlock();

    return true;
}

void CPPConfig::lock()
{
    if(thread_safe)
    {
        mutex_data.lock();
    }
}

void CPPConfig::unlock()
{
    if(thread_safe)
    {
        mutex_data.unlock();
    }
}

void CPPConfig::enableAutoSave()
{
    this->auto_save = true;
}

void CPPConfig::disableAutoSave()
{
    this->auto_save = false;
}

Message CPPConfig::toMessage()
{
    Message msg;
    std::vector<std::string> groups = getAllGroups();
    for(size_t i = 0; i < groups.size(); i++)
    {
        std::vector<std::string> keys = getAllKeysByGroup(groups[i].c_str());
        if(keys.empty() == false)
        {
            for(size_t j = 0; j < keys.size(); j++)
            {
                std::string value = getString(groups[i].c_str(), keys[j].c_str());
                if(value.empty() == false)
                {
                    msg.set(com_string_format("%s.%s", groups[i].c_str(), keys[j].c_str()).c_str(), value);
                }
            }
        }
    }

    return msg;
}

std::string CPPConfig::toString()
{
    std::string val;
    std::vector<std::string> groups = getAllGroups();
    for(size_t i = 0; i < groups.size(); i++)
    {
        std::vector<std::string> keys = getAllKeysByGroup(groups[i].c_str());
        if(keys.empty() == false)
        {
            val += "[" + groups[i] + "]\n";
            for(size_t j = 0; j < keys.size(); j++)
            {
                std::string value = getString(groups[i].c_str(), keys[j].c_str());
                if(value.empty() == false)
                {
                    val += keys[j] + "=" + value + "\n";
                }
            }
        }
    }

    return val;
}

std::string CPPConfig::getConfigFile() const
{
    return config_file;
}

std::string CPPConfig::getString(const char* group, const char* key, std::string default_val)
{
    if(key == NULL)
    {
        return default_val;
    }
    std::string tag;
    if(group != NULL)
    {
        tag.append(group);
    }

    tag.append(":").append(key);
    lock();
    const char* val = iniparser_getstring((const dictionary*)this->data, tag.c_str(), default_val.c_str());
    unlock();
    if(val == NULL)
    {
        return default_val;
    }
    return val;
}

int8 CPPConfig::getInt8(const char* group, const char* key, int8 default_val)
{
    return (int8)getInt64(group, key, (int64)default_val);
}

uint8 CPPConfig::getUInt8(const char* group, const char* key, uint8 default_val)
{
    return (uint8)getUInt64(group, key, (uint64)default_val);
}

int16 CPPConfig::getInt16(const char* group, const char* key, int16 default_val)
{
    return (int16)getInt64(group, key, (int64)default_val);
}

uint16 CPPConfig::getUInt16(const char* group, const char* key, uint16 default_val)
{
    return (uint16)getUInt64(group, key, (uint64)default_val);
}

int32 CPPConfig::getInt32(const char* group, const char* key, int32 default_val)
{
    return (int32)getInt64(group, key, (int64)default_val);
}

uint32 CPPConfig::getUInt32(const char* group, const char* key, uint32 default_val)
{
    return (uint32)getUInt64(group, key, (uint64)default_val);
}

int64 CPPConfig::getInt64(const char* group, const char* key, int64 default_val)
{
    std::string val = getString(group, key);
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

uint64 CPPConfig::getUInt64(const char* group, const char* key, uint64 default_val)
{
    std::string val = getString(group, key);
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

bool CPPConfig::getBool(const char* group, const char* key, bool default_val)
{
    if(key == NULL)
    {
        return default_val;
    }
    std::string tag;
    if(group != NULL)
    {
        tag.append(group);
    }

    tag.append(":").append(key);
    lock();
    int ret = iniparser_getboolean((const dictionary*)this->data, tag.c_str(), default_val ? 1 : 0);
    unlock();
    return (ret == 1);
}

float CPPConfig::getFloat(const char* group, const char* key, float default_val)
{
    std::string val = getString(group, key);
    if(val.empty())
    {
        return default_val;
    }
    return strtof(val.c_str(), NULL);
}

double CPPConfig::getDouble(const char* group, const char* key, double default_val)
{
    std::string val = getString(group, key);
    if(val.empty())
    {
        return default_val;
    }
    return strtod(val.c_str(), NULL);
}

bool CPPConfig::isGroupExist(const char* group_name)
{
    if(group_name == NULL)
    {
        return false;
    }
    lock();
    int ret = iniparser_find_entry((const dictionary*)this->data, group_name);
    unlock();
    return (ret == 1);
}

bool CPPConfig::isKeyExist(const char* group_name, const char* key)
{
    std::string val = getString(group_name, key);
    return (val.empty() == false);
}

std::vector<std::string> CPPConfig::getAllGroups()
{
    lock();
    std::vector<std::string> groups;
    int count = iniparser_getnsec((const dictionary*)this->data);
    for(int i = 0; i < count; i++)
    {
        const char* name = iniparser_getsecname((const dictionary*)this->data, i);
        if(name != NULL)
        {
            groups.push_back(name);
        }
    }
    unlock();
    return groups;
}

std::vector<std::string> CPPConfig::getAllKeysByGroup(const char* group_name)
{
    std::vector<std::string> keys;
    int key_count = getGroupKeyCount(group_name);
    if(key_count <= 0)
    {
        return keys;
    }

    lock();
#if defined(_WIN32) || defined(_WIN64)
    char** keys_tmp = new char* [key_count]();
#else
    char* keys_tmp[key_count];
    memset(keys_tmp, 0, sizeof(keys_tmp));
#endif
    int len = strlen(group_name);
    const char** ret = iniparser_getseckeys((const dictionary*)this->data, group_name, (const char**)keys_tmp);
    if(ret != NULL)
    {
        for(int i = 0; i < key_count; i++)
        {
            if(keys_tmp[i] == NULL)
            {
                continue;
            }
            //key 默认包含了$group:需要去除
            if((int)strlen(keys_tmp[i]) > len + 1)
            {
                keys.push_back(keys_tmp[i] + len + 1);
            }
        }
    }
#if defined(_WIN32) || defined(_WIN64)
    delete[] keys_tmp;
#endif
    unlock();
    return keys;
}

int CPPConfig::getGroupKeyCount(const char* group_name)
{
    if(group_name == NULL)
    {
        return -1;
    }
    lock();
    int ret = iniparser_getsecnkeys((const dictionary*)this->data, group_name);
    unlock();
    return ret;
}

bool CPPConfig::set(const char* group, const char* key, std::string val)
{
    return set(group, key, val.c_str());
}

bool CPPConfig::set(const char* group, const char* key, char* val)
{
    return set(group, key, (const char*)val);
}

bool CPPConfig::set(const char* group, const char* key, const char* val)
{
    if(key == NULL || val == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    if(group != NULL && isGroupExist(group) == false)
    {
        if(addGroup(group) == false)
        {
            LOG_E("failed to add group");
            return false;
        }
    }

    std::string tag;
    if(group != NULL)
    {
        tag.append(group);
    }
    tag.append(":").append(key);

    lock();
    int ret = iniparser_set((dictionary*)this->data, tag.c_str(), val);
    unlock();
    if(ret != 0)
    {
        LOG_E("failed to set ini data");
        return false;
    }

    data_changed = true;
    if(auto_save)
    {
        save();
    }
    return true;
}

bool CPPConfig::addGroup(const char* group_name)
{
    if(group_name == NULL)
    {
        return false;
    }

    std::string tag = group_name;
    lock();
    int ret = iniparser_set((dictionary*)this->data, tag.c_str(), NULL);
    unlock();
    if(ret != 0)
    {
        return false;
    }

    data_changed = true;
    if(auto_save)
    {
        save();
    }
    return true;
}

bool CPPConfig::removeGroup(const char* group_name)
{
    if(group_name == NULL)
    {
        return false;
    }

    std::string tag = group_name;
    lock();
    iniparser_unset((dictionary*)this->data, tag.c_str());
    unlock();
    data_changed = true;
    if(auto_save)
    {
        save();
    }
    return true;
}

bool CPPConfig::removeItem(const char* group_name, const char* key)
{
    if(key == NULL)
    {
        return false;
    }
    std::string tag;
    if(group_name != NULL)
    {
        tag.append(group_name);
    }
    tag.append(":").append(key);
    lock();
    iniparser_unset((dictionary*)this->data, tag.c_str());
    unlock();
    data_changed = true;
    if(auto_save)
    {
        save();
    }
    return true;
}

Message com_global_config_to_message()
{
    return global_config.toMessage();
}

bool com_global_config_load(const char* config_file)
{
    global_config.enableAutoSave();
    return global_config.load(config_file);
}

bool com_global_config_save()
{
    return global_config.save();
}

bool com_global_config_save_as(const char* config_file)
{
    return global_config.saveAs(config_file);
}

bool com_global_config_is_group_exist(const char* group)
{
    return global_config.isGroupExist(group);
}

bool com_global_config_is_key_exist(const char* group, const char* key)
{
    return global_config.isKeyExist(group, key);
}

bool com_global_config_set(const char* group, const char* key, const bool val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const double val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const int8 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const int16 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const int32 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const int64 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const uint8 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const uint16 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const uint32 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const uint64 val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const std::string& val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_set(const char* group, const char* key, const char* val)
{
    return global_config.set(group, key, val);
}

bool com_global_config_remove(const char* group, const char* key)
{
    return global_config.removeItem(group, key);
}

bool com_global_config_get_bool(const char* group, const char* key, bool default_val)
{
    return global_config.getBool(group, key, default_val);
}

double com_global_config_get_double(const char* group, const char* key, double default_val)
{
    return global_config.getDouble(group, key, default_val);
}

int8 com_global_config_get_int8(const char* group, const char* key, int8 default_val)
{
    return global_config.getInt8(group, key, default_val);
}

int16 com_global_config_get_int16(const char* group, const char* key, int16 default_val)
{
    return global_config.getInt16(group, key, default_val);
}

int32 com_global_config_get_int32(const char* group, const char* key, int32 default_val)
{
    return global_config.getInt32(group, key, default_val);
}

int64 com_global_config_get_int64(const char* group, const char* key, int64 default_val)
{
    return global_config.getInt64(group, key, default_val);
}

uint8 com_global_config_get_uint8(const char* group, const char* key, uint8 default_val)
{
    return global_config.getUInt8(group, key, default_val);
}

uint16 com_global_config_get_uint16(const char* group, const char* key, uint16 default_val)
{
    return global_config.getUInt16(group, key, default_val);
}

uint32 com_global_config_get_uint32(const char* group, const char* key, uint32 default_val)
{
    return global_config.getUInt32(group, key, default_val);
}

uint64 com_global_config_get_uint64(const char* group, const char* key, uint64 default_val)
{
    return global_config.getUInt64(group, key, default_val);
}

std::string com_global_config_get_string(const char* group, const char* key, std::string default_val)
{
    return global_config.getString(group, key, default_val);
}

