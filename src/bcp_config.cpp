#include "bcp_com.h"
#include "bcp_file.h"
#include "bcp_config.h"
#include "bcp_log.h"
#include "bcp_md5.h"
#include "iniparser.h"

static CPPConfig global_config(true);

CPPConfig::CPPConfig()
{
    this->data = NULL;
    this->data_changed = false;
    this->auto_save = true;
    this->safe_mode = true;
    this->thread_safe = false;
}

CPPConfig::CPPConfig(bool thread_safe)
{
    this->data = NULL;
    this->data_changed = false;
    this->auto_save = true;
    this->safe_mode = true;
    this->thread_safe = thread_safe;
}

CPPConfig::CPPConfig(const char* file, bool thread_safe, bool safe_mode, const char* safe_dir)
{
    this->data = NULL;
    this->data_changed = false;
    this->auto_save = true;
    this->safe_mode = safe_mode;
    this->thread_safe = thread_safe;
    if (safe_dir != NULL)
    {
        this->safe_dir = safe_dir;
    }
    if (file != NULL)
    {
        this->config_file = file;
        int file_type = bcp_file_type(file);
        if (file_type == FILE_TYPE_FILE)
        {
            load(file, safe_mode, safe_dir);
        }
        else if (file_type == FILE_TYPE_NOT_EXIST)
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
    if (this->data != NULL)
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
    if (this->data != NULL)
    {
        std::vector<std::string> groups = config.getAllGroups();
        for (int i = 0; i < groups.size(); i++)
        {
            std::vector<std::string> keys = config.getAllKeysByGroup(groups[i].c_str());
            if (keys.empty() == false)
            {
                for (int j = 0; j < keys.size(); j++)
                {
                    std::string value = config.getString(groups[i].c_str(), keys[j].c_str());
                    if (value.empty() == false)
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
    if (this == &config)
    {
        return *this;
    }
    if (this->data != NULL)
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
    if (this->data != NULL)
    {
        std::vector<std::string> groups = config.getAllGroups();
        for (int i = 0; i < groups.size(); i++)
        {
            std::vector<std::string> keys = config.getAllKeysByGroup(groups[i].c_str());
            if (keys.empty() == false)
            {
                for (int j = 0; j < keys.size(); j++)
                {
                    std::string value = config.getString(groups[i].c_str(), keys[j].c_str());
                    if (value.empty() == false)
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
    if (safe_mode || auto_save)
    {
        save();
    }
    if (this->data != NULL)
    {
        iniparser_freedict((dictionary*)this->data);
        this->data = NULL;
    }
}

bool CPPConfig::load(const char* file, bool safe_mode, const char* safe_dir)
{
    if (file == NULL)
    {
        return false;
    }
    lock();
    if (this->data != NULL)
    {
        iniparser_freedict((dictionary*)this->data);
        this->data = NULL;
    }
    unlock();
    bool ret = false;
    this->config_file = file;
    this->data_changed = false;
    this->safe_mode = safe_mode;
    this->safe_dir.clear();
    if (safe_dir != NULL)
    {
        this->safe_dir = safe_dir;
    }

    if (safe_mode)
    {
        std::string config_file_swap = getSwapFileName(file);
        std::string config_file_swap_bak = config_file_swap + ".bak";

        if (bcp_file_type(config_file_swap.c_str()) == FILE_TYPE_NOT_EXIST)
        {
            if (bcp_file_type(config_file_swap_bak.c_str()) == FILE_TYPE_FILE)
            {
                //从bak恢复
                bcp_file_copy(config_file_swap.c_str(), config_file_swap_bak.c_str());
            }
            else
            {
                //从初始文件恢复并生成signature
                CreateSignature(config_file_swap.c_str(), config_file.c_str());
            }
        }

        if (CheckSignature(config_file_swap.c_str()) == false)
        {
            return false;
        }
        this->config_file_swap = config_file_swap;
        this->config_file_swap_bak = config_file_swap_bak;
        lock();
        this->data = (void*)iniparser_load(this->config_file_swap.c_str());
        ret = (this->data != NULL);
        unlock();
    }
    else
    {
        lock();
        this->data = (void*)iniparser_load(this->config_file.c_str());
        ret = (this->data != NULL);
        unlock();
    }

    return ret;
}


bool CPPConfig::save()
{
    if (data_changed == false)
    {
        return true;
    }

    if (config_file.empty() || config_file.length() <= 0)
    {
        LOG_W("config file name not set");
        return false;
    }

    bool ret = false;
    if (safe_mode)
    {
        ret = saveAs(config_file_swap_bak.c_str());
        ret = saveAs(config_file_swap.c_str());
    }
    else
    {
        ret = saveAs(config_file.c_str());
    }
    if (ret)
    {
        data_changed = false;
    }

    return ret;
}

bool CPPConfig::saveAs(const char* config_file)
{
    if (bcp_string_len(config_file) <= 0)
    {
        return NULL;
    }
    std::string val = toString();
    if (val.empty())
    {
        return false;
    }

    lock();
    FILE* file = bcp_file_open(config_file, "w+");
    if (file == NULL)
    {
        unlock();
        return false;
    }

    int ret = bcp_file_write(file, val.data(), val.size());
    if (ret != val.size())
    {
        bcp_file_close(file);
        unlock();
        return false;
    }
    bcp_file_flush(file);
    bcp_file_sync(file);
    bcp_file_close(file);
    unlock();

    return CheckSignature(config_file);
}

void CPPConfig::lock()
{
    if (thread_safe)
    {
        mutex_data.lock();
    }
}

void CPPConfig::unlock()
{
    if (thread_safe)
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
    for (int i = 0; i < groups.size(); i++)
    {
        std::vector<std::string> keys = getAllKeysByGroup(groups[i].c_str());
        if (keys.empty() == false)
        {
            for (int j = 0; j < keys.size(); j++)
            {
                std::string value = getString(groups[i].c_str(), keys[j].c_str());
                if (value.empty() == false)
                {
                    msg.set(bcp_string_format("%s.%s", groups[i].c_str(), keys[j].c_str()).c_str(), value);
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
    for (int i = 0; i < groups.size(); i++)
    {
        std::vector<std::string> keys = getAllKeysByGroup(groups[i].c_str());
        if (keys.empty() == false)
        {
            val += "[" + groups[i] + "]\n";
            for (int j = 0; j < keys.size(); j++)
            {
                std::string value = getString(groups[i].c_str(), keys[j].c_str());
                if (value.empty() == false)
                {
                    val += keys[j] + "=" + value + "\n";
                }
            }
        }
    }

    CPPMD5 md5_tool;
    md5_tool.append((uint8*)val.data(), val.size());
    std::string header = ";md5=";
    std::string md5 = md5_tool.finish().toHexString(false) + "\n";
    header.append(md5);
    val.insert(0, header);
    return val;
}

std::string CPPConfig::getConfigFile() const
{
    return config_file;
}

std::string CPPConfig::getSwapFileName(const char* file)
{
    if (file == NULL)
    {
        return std::string();
    }
    std::string config_file_path = safe_dir;
    if (config_file_path.empty())
    {
        config_file_path = bcp_path_dir(file);
    }
    std::string config_file_name = bcp_path_name(file);
    if (config_file_path.empty() || config_file_name.empty())
    {
        return std::string();
    }
    return config_file_path + "/." + config_file_name;
}

bool CPPConfig::CheckSignature(const char* config_file)
{
    if (bcp_string_len(config_file) <= 0)
    {
        return false;
    }
    ByteArray bytes = bcp_file_readall(config_file);
    std::string val = bytes.toString();
    if (val.empty() || bcp_string_start_with(val.c_str(), ";md5=") == false)
    {
        LOG_E("signature missing for %s", config_file);
        return false;
    }
    size_t pos = val.find_first_of("\n");
    if (pos == std::string::npos)
    {
        LOG_E("signature not found for %s", config_file);
        return false;
    }

    std::string md5_required = val.substr(sizeof(";md5=") - 1, pos - 5);
    std::string content = val.substr(pos + 1);
    CPPMD5 md5_tool;
    md5_tool.append((uint8*)content.data(), content.size());
    std::string md5_calc = md5_tool.finish().toHexString(false);

    if (md5_calc != md5_required)
    {
        LOG_E("signature incorrect for (%s),md5_requied=%s,md5_calc=%s",
              config_file, md5_required.c_str(), md5_calc.c_str());
        return false;
    }

    return true;
}

bool CPPConfig::CreateSignature(const char* config_file_to, const char* config_file_from)
{
    if (bcp_string_len(config_file_to) <= 0 || bcp_string_len(config_file_from) <= 0)
    {
        return false;
    }
    ByteArray bytes = bcp_file_readall(config_file_from);
    std::string val = bytes.toString();
    std::string content;
    if (bcp_string_start_with(val.c_str(), ";md5="))
    {
        size_t pos = val.find_first_of("\n");
        if (pos == std::string::npos)
        {
            LOG_E("file content is empty(only md5 exist)");
            return false;
        }
        content = val.substr(pos);
    }
    else
    {
        content = val;
    }

    CPPMD5 md5_tool;
    md5_tool.append((uint8*)content.data(), content.size());
    std::string header = ";md5=";
    std::string md5 = md5_tool.finish().toHexString(false) + "\n";
    header.append(md5);
    content.insert(0, header);

    FILE* file = bcp_file_open(config_file_to, "w+");
    if (file == NULL)
    {
        LOG_E("failed to open file");
        return false;
    }

    int ret = bcp_file_write(file, content.data(), content.size());
    if (ret != content.size())
    {
        bcp_file_close(file);
        LOG_E("failed to write content to file");
        return false;
    }

    bcp_file_flush(file);
    bcp_file_sync(file);
    bcp_file_close(file);

    return true;
}

bool CPPConfig::CreateSignature(const char* config_file)
{
    return CreateSignature(config_file, config_file);
}

std::string CPPConfig::getString(const char* group, const char* key, std::string default_val)
{
    if (key == NULL)
    {
        return default_val;
    }
    if (group == NULL)
    {
        group = "";
    }
    std::string tag;
    tag.append(group).append(":").append(key);
    lock();
    const char* val = iniparser_getstring((const dictionary*)this->data, tag.c_str(), default_val.c_str());
    unlock();
    if (val == NULL)
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
    if (val.empty())
    {
        return default_val;
    }
    if (bcp_string_start_with(val.c_str(), "0x") || bcp_string_start_with(val.c_str(), "0X"))
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
    if (val.empty())
    {
        return default_val;
    }
    if (bcp_string_start_with(val.c_str(), "0x") || bcp_string_start_with(val.c_str(), "0X"))
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
    if (key == NULL)
    {
        return default_val;
    }
    if (group == NULL)
    {
        group = "";
    }
    std::string tag;
    tag.append(group).append(":").append(key);
    lock();
    int ret = iniparser_getboolean((const dictionary*)this->data, tag.c_str(), default_val ? 1 : 0);
    unlock();
    return (ret == 1);
}

float CPPConfig::getFloat(const char* group, const char* key, float default_val)
{
    std::string val = getString(group, key);
    if (val.empty())
    {
        return default_val;
    }
    return strtof(val.c_str(), NULL);
}

double CPPConfig::getDouble(const char* group, const char* key, double default_val)
{
    std::string val = getString(group, key);
    if (val.empty())
    {
        return default_val;
    }
    return strtod(val.c_str(), NULL);
}

bool CPPConfig::isGroupExist(const char* group_name)
{
    if (group_name == NULL)
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
    for (int i = 0; i < count; i++)
    {
        const char* name = iniparser_getsecname((const dictionary*)this->data, i);
        if (name != NULL)
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
    if (key_count <= 0)
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
    if (ret != NULL)
    {
        for (int i = 0; i < key_count; i++)
        {
            if (keys_tmp[i] == NULL)
            {
                continue;
            }
            //key 默认包含了$group:需要去除
            if (strlen(keys_tmp[i]) > len + 1)
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
    if (group_name == NULL)
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
    if (group == NULL || key == NULL || val == NULL)
    {
        return false;
    }
    std::string tag;
    tag.append(group).append(":").append(key);

    if (isGroupExist(group) == false)
    {
        if (addGroup(group) == false)
        {
            return false;
        }
    }

    lock();
    int ret = iniparser_set((dictionary*)this->data, tag.c_str(), val);
    unlock();
    if (ret != 0)
    {
        return false;
    }

    data_changed = true;
    if (auto_save)
    {
        save();
    }
    return true;
}

bool CPPConfig::addGroup(const char* group_name)
{
    if (group_name == NULL)
    {
        return false;
    }

    std::string tag = group_name;
    lock();
    int ret = iniparser_set((dictionary*)this->data, tag.c_str(), NULL);
    unlock();
    if (ret != 0)
    {
        return false;
    }

    data_changed = true;
    if (auto_save)
    {
        save();
    }
    return true;
}

bool CPPConfig::removeGroup(const char* group_name)
{
    if (group_name == NULL)
    {
        return false;
    }

    std::string tag = group_name;
    lock();
    iniparser_unset((dictionary*)this->data, tag.c_str());
    unlock();
    data_changed = true;
    if (auto_save)
    {
        save();
    }
    return true;
}

bool CPPConfig::removeItem(const char* group_name, const char* key)
{
    if (group_name == NULL || key == NULL)
    {
        return false;
    }
    std::string tag;
    tag.append(group_name).append(":").append(key);
    lock();
    iniparser_unset((dictionary*)this->data, tag.c_str());
    unlock();
    data_changed = true;
    if (auto_save)
    {
        save();
    }
    return true;
}

Message bcp_global_config_to_message()
{
    return global_config.toMessage();
}

bool bcp_global_config_load(const char* config_file, bool safe_mode, const char* safe_dir)
{
    global_config.enableAutoSave();
    return global_config.load(config_file, safe_mode, safe_dir);
}

bool bcp_global_config_save()
{
    return global_config.save();
}

bool bcp_global_config_save_as(const char* config_file)
{
    return global_config.saveAs(config_file);
}

bool bcp_global_config_is_group_exist(const char* group)
{
    return global_config.isGroupExist(group);
}

bool bcp_global_config_is_key_exist(const char* group, const char* key)
{
    return global_config.isKeyExist(group, key);
}

bool bcp_global_config_set(const char* group, const char* key, const bool val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const double val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const int8 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const int16 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const int32 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const int64 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const uint8 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const uint16 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const uint32 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const uint64 val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const std::string& val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_set(const char* group, const char* key, const char* val)
{
    return global_config.set(group, key, val);
}

bool bcp_global_config_remove(const char* group, const char* key)
{
    return global_config.removeItem(group, key);
}

bool bcp_global_config_get_bool(const char* group, const char* key, bool default_val)
{
    return global_config.getBool(group, key, default_val);
}

double bcp_global_config_get_double(const char* group, const char* key, double default_val)
{
    return global_config.getDouble(group, key, default_val);
}

int8 bcp_global_config_get_int8(const char* group, const char* key, int8 default_val)
{
    return global_config.getInt8(group, key, default_val);
}

int16 bcp_global_config_get_int16(const char* group, const char* key, int16 default_val)
{
    return global_config.getInt16(group, key, default_val);
}

int32 bcp_global_config_get_int32(const char* group, const char* key, int32 default_val)
{
    return global_config.getInt32(group, key, default_val);
}

int64 bcp_global_config_get_int64(const char* group, const char* key, int64 default_val)
{
    return global_config.getInt64(group, key, default_val);
}

uint8 bcp_global_config_get_uint8(const char* group, const char* key, uint8 default_val)
{
    return global_config.getUInt8(group, key, default_val);
}

uint16 bcp_global_config_get_uint16(const char* group, const char* key, uint16 default_val)
{
    return global_config.getUInt16(group, key, default_val);
}

uint32 bcp_global_config_get_uint32(const char* group, const char* key, uint32 default_val)
{
    return global_config.getUInt32(group, key, default_val);
}

uint64 bcp_global_config_get_uint64(const char* group, const char* key, uint64 default_val)
{
    return global_config.getUInt64(group, key, default_val);
}

std::string bcp_global_config_get_string(const char* group, const char* key, std::string default_val)
{
    return global_config.getString(group, key, default_val);
}

