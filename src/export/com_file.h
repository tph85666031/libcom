#ifndef __COM_FILE_H__
#define __COM_FILE_H__

#include <functional>
#include "com_base.h"

#ifndef FILE_TYPE_UNKNOWN
#define FILE_TYPE_UNKNOWN     0
#endif
#define FILE_TYPE_DIR         1
#define FILE_TYPE_LINK        2
#define FILE_TYPE_FILE        3
#define FILE_TYPE_SOCK        4
#define FILE_TYPE_SYS         10
#define FILE_TYPE_NOT_EXIST   0xFF

#if defined(_WIN32) || defined(_WIN64)
#define PATH_DELIM_CHAR   '\\'
#define PATH_DELIM_STR    "\\"
#define PATH_DELIM_WCHAR L'\\'
#define PATH_DELIM_WSTR  L"\\"
#else
#define PATH_DELIM_CHAR   '/'
#define PATH_DELIM_STR    "/"
#define PATH_DELIM_WCHAR L'/'
#define PATH_DELIM_WSTR  L"/"
#endif

class COM_EXPORT FilePath
{
public:
    FilePath(const std::string& path);
    FilePath(const char* path);
    virtual ~FilePath();
    std::string getName(); //file name or dir name without dir prefix but with file suffix
    std::string getNameWithoutSuffix(); //file name or dir name without dir prefix or file suffix
    std::string getDir();//must not end with '/' or '\'
    std::string getPath();
    std::string getSuffix();
protected:
    bool parse(const char* path);
private:
    std::string path;
    std::string name;
    std::string name_without_suffix;
    std::string dir;
    std::string suffix;
};

class COM_EXPORT FileDetail : public FilePath
{
public:
    FileDetail(const char* path);
    virtual ~FileDetail();

    int getType();
    uint64 getSize();
    uint32 getAccessTimeS();
    uint32 getChangeTimeS();
    uint32 getModifyTimeS();

    bool setAccessTime(uint32 timestamp_s = 0);
    bool setModifyTime(uint32 timestamp_s = 0);
private:
    std::string path;
    int type = FILE_TYPE_UNKNOWN;
    uint64 size = 0;
    uint32 time_change_s = 0;//文件属性修改时间(非内容)时间戳，秒，带时区
    uint32 time_access_s = 0;//文件访问时间(非实时)时间戳，秒，带时区
    uint32 time_modify_s = 0;//文件内容修改时间,时间戳，秒，带时区
};

COM_EXPORT std::string com_dir_system_temp();
COM_EXPORT int64 com_dir_size_max(const char* dir);
COM_EXPORT int64 com_dir_size_used(const char* dir);
COM_EXPORT int64 com_dir_size_freed(const char* dir);
COM_EXPORT bool com_dir_exists(const char* dir);
COM_EXPORT bool com_dir_create(const char* full_path);
COM_EXPORT void com_dir_clear(const char* dir_path);
COM_EXPORT bool com_file_erase(const char* file_path, uint8 val = 0);
COM_EXPORT int com_dir_remove(const char* dir_path);
COM_EXPORT bool com_dir_list(const char* dir_path, std::map<std::string, int>& list, bool recursion = false);

COM_EXPORT std::string com_path_name(const char* path);
COM_EXPORT std::string com_path_suffix(const char* path);
COM_EXPORT std::string com_path_name_without_suffix(const char* path);
COM_EXPORT std::string com_path_dir(const char* path);

COM_EXPORT int com_file_type(const char* file);
COM_EXPORT int com_file_type(int fd);
COM_EXPORT int com_file_type(FILE* file);
COM_EXPORT bool com_file_exist(const char* file_path);
COM_EXPORT bool com_file_create(const char* file_path);
COM_EXPORT FILE* com_file_open(const char* file_path, const char* flag);
COM_EXPORT int64 com_file_size(int fd);
COM_EXPORT int64 com_file_size(FILE* file);
COM_EXPORT int64 com_file_size(const char* file_path);
COM_EXPORT int64 com_file_size_zip(const char* zip_file_path);
COM_EXPORT bool com_file_copy(const char* file_path_dst, const char* file_path_src, bool append = false);
COM_EXPORT bool com_file_truncate(FILE* file, int64 size);
COM_EXPORT bool com_file_truncate(const char* file_path, int64 size);
COM_EXPORT bool com_file_clean(const char* file_path);
COM_EXPORT CPPBytes com_file_read(const char* file, int size, int64 offset = 0);
COM_EXPORT CPPBytes com_file_read(FILE* file, int size);
COM_EXPORT int64 com_file_read(FILE* file, void* buf, int64 size);
COM_EXPORT CPPBytes com_file_read_until(FILE* file, const char* str);
COM_EXPORT CPPBytes com_file_read_until(FILE* file, const uint8* data, int data_size);
COM_EXPORT CPPBytes com_file_read_until(FILE* file, std::function<bool(uint8)> func);
COM_EXPORT int64 com_file_find(const char* file, const char* key, int64 offset = 0);
COM_EXPORT int64 com_file_find(const char* file, const uint8* key, int key_size, int64 offset = 0);
COM_EXPORT int64 com_file_find(FILE* file, const char* key);
COM_EXPORT int64 com_file_find(FILE* file, const uint8* key, int key_size);
COM_EXPORT int64 com_file_rfind(const char* file, const char* key, int64 offset = 0);
COM_EXPORT int64 com_file_rfind(const char* file, const uint8* key, int key_size, int64 offset = 0);
COM_EXPORT int64 com_file_rfind(FILE* file, const char* key);
COM_EXPORT int64 com_file_rfind(FILE* file, const uint8* key, int key_size);
COM_EXPORT bool com_file_readline(FILE* file, char* buf, int size);
COM_EXPORT bool com_file_readline(FILE* file, std::string& line);
COM_EXPORT CPPBytes com_file_readall(const std::string& file_path, int64 offset = 0);
COM_EXPORT CPPBytes com_file_readall(const char* file_path, int64 offset = 0);
COM_EXPORT int64 com_file_write(FILE* file, const void* buf, int size);
COM_EXPORT int64 com_file_writef(FILE* fp, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
COM_EXPORT int64 com_file_writef(const char* file, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
COM_EXPORT int64 com_file_appendf(const char* file, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
COM_EXPORT bool com_file_rename(const char* file_name_new, const char* file_name_old);
COM_EXPORT void com_file_close(FILE* file);
COM_EXPORT void com_file_flush(FILE* file);
COM_EXPORT void com_file_sync(FILE* file);
COM_EXPORT bool com_file_remove(const char* file_name);
COM_EXPORT uint64 com_file_get_id(const char* file_path);
COM_EXPORT uint32 com_file_get_change_time(const char* file_path);
COM_EXPORT uint32 com_file_get_modify_time(const char* file_path);
COM_EXPORT uint32 com_file_get_access_time(const char* file_path);
COM_EXPORT bool com_file_set_access_time(const char* path, uint32 time_s = 0);
COM_EXPORT bool com_file_set_modify_time(const char* path, uint32 time_s = 0);
COM_EXPORT bool com_file_set_owner(const char* file, int uid, int gid);
COM_EXPORT bool com_file_set_mod(const char* file, int mod);
COM_EXPORT bool com_file_seek_head(FILE* file);
COM_EXPORT bool com_file_seek_tail(FILE* file);
COM_EXPORT bool com_file_seek_step(FILE* file, int64 pos);
COM_EXPORT bool com_file_seek_set(FILE* file, int64 pos);
COM_EXPORT int64 com_file_seek_get(FILE* file);
COM_EXPORT bool com_file_crop(const char* file_name, uint8 start_percent_keeped, uint8 end_percent_keeped);

COM_EXPORT int com_file_get_fd(FILE* file);
COM_EXPORT bool com_file_lock(FILE* file, bool share_read = false, bool wait = false);
COM_EXPORT bool com_file_lock(int fd, bool share_read = false, bool wait = false);
COM_EXPORT bool com_file_is_locked(FILE* file);
COM_EXPORT bool com_file_is_locked(FILE* file, int& type, int64& pid);
COM_EXPORT bool com_file_is_locked(int fd);
COM_EXPORT bool com_file_is_locked(int fd, int& type, int64& pid);
COM_EXPORT bool com_file_unlock(FILE* file);
COM_EXPORT bool com_file_unlock(int fd);

class COM_EXPORT SingleInstanceProcess
{
public:
    SingleInstanceProcess(const char* file_lock = NULL);
    virtual ~SingleInstanceProcess();;
private:
    void* fp = NULL;
};

COM_EXPORT std::string PATH_TO_DOS(const char* path);
COM_EXPORT std::string PATH_TO_DOS(const std::string& path);
COM_EXPORT std::string PATH_FROM_DOS(const char* path);
COM_EXPORT std::string PATH_FROM_DOS(const std::string& path);
COM_EXPORT std::string PATH_TO_LOCAL(const char* path);
COM_EXPORT std::string PATH_TO_LOCAL(const std::string& path);

#endif /* __COM_FILE_H__ */

