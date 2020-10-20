#ifndef __COM_FILE_H__
#define __COM_FILE_H__

#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#endif

#include "com_base.h"

#if defined(_WIN32) || defined(_WIN64)
#define PATH_DELIM_CHAR '\\'
#define PATH_DELIM_STR "\\"
#else
#define PATH_DELIM_CHAR '/'
#define PATH_DELIM_STR "/"
#endif

#ifndef FILE_TYPE_UNKNOWN
#define FILE_TYPE_UNKNOWN     0
#endif
#define FILE_TYPE_DIR         1
#define FILE_TYPE_LINK        2
#define FILE_TYPE_FILE        3
#define FILE_TYPE_NOT_EXIST   0xFF

class FilePath
{
public:
    FilePath(const std::string& path);
    FilePath(const char* path);
    virtual ~FilePath();
    std::string getName(); //file name or dir name without dir prefix
    std::string getLocationDirectory();//endwith '/' or '\'
    bool isDirectory();// path endwith '/' or '\' is directory
private:
    bool parse(const char* path);
private:
    std::string name;
    std::string dir;
    bool is_dir = false;
};

std::string com_path_name(const char* path);
std::string com_path_dir(const char* path);

int com_file_type(const char* file);
bool com_file_create(const char* file_path);
FILE* com_file_open(const char* file_path, const char* flag);
int64 com_dir_size_max(const char* dir);
int64 com_dir_size_used(const char* dir);
int64 com_dir_size_freed(const char* dir);
bool com_dir_create(const char* full_path);
int com_dir_remove(const char* dir_path);
int64 com_file_size(const char* file_path);
int64 com_file_size_zip(const char* zip_file_path);
void com_file_copy(const char* file_path_dst, const char* file_path_src, bool append = false);
void com_file_clean(const char* file_path);
int com_file_read(FILE* file, void* buf, int size);
bool com_file_readline(FILE* file, char* buf, int size);
ByteArray com_file_readall(const char* file_path);
int com_file_write(FILE* file, const void* buf, int size);
int com_file_writef(FILE* fp, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
bool com_file_rename(const char* file_name_new, const char* file_name_old);
void com_file_close(FILE* file);
void com_file_flush(FILE* file);
void com_file_sync(FILE* file);
void com_file_remove(const char* file_name);
uint32 com_file_get_create_time(const char* file_path);
uint32 com_file_get_modify_time(const char* file_path);
uint32 com_file_get_access_time(const char* file_path);
int com_file_seek_head(FILE* file);
int com_file_seek_tail(FILE* file);
int com_file_seek(FILE* file, int64 pos);
bool com_file_crop(const char* file_name, uint8 start_percent_keeped, uint8 end_percent_keeped);

int com_file_get_fd(FILE* file);
bool com_file_lock(FILE* file, bool read_share);
bool com_file_is_locked(FILE* file, bool read_share);
bool com_file_unlock(FILE* file);

#endif // __COM_FILE_H__

