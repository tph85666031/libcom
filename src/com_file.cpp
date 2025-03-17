#include <iostream>
#include <fstream>
#include <fcntl.h>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <direct.h>
#include <sys/utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#elif __linux__ == 1
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <utime.h>
#include <glob.h>
#elif defined(__APPLE__)
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <dirent.h>
#include <utime.h>
#include <glob.h>
#endif

#include "com_base.h"
#include "com_file.h"
#include "com_log.h"
#include "com_thread.h"

static bool dir_list(const char* dir_root, std::map<std::string, int>& list, const char* path_pattern, bool pattern_as_path)
{
    if(dir_root == NULL)
    {
        return false;
    }

#if defined(_WIN32) || defined(_WIN64)
    struct _wfinddata_t file_info;
    std::wstring dir_root_w = com_wstring_from_utf8(dir_root);
    if(dir_root_w.back() != PATH_DELIM_WCHAR)
    {
        dir_root_w.append(PATH_DELIM_WSTR);
    }
    intptr_t handle = _wfindfirst(com_wstring_format(L"%s%c*.*", dir_root_w.c_str(), PATH_DELIM_WCHAR).c_str(), &file_info);
    if(handle == -1)
    {
        return false;
    }
    do
    {
        std::string path = dir_root;
        if(path.back() != PATH_DELIM_CHAR)
        {
            path.append(PATH_DELIM_STR);
        }
        path.append(com_wstring_to_utf8(file_info.name).toString());
        if(file_info.attrib & _A_SUBDIR)
        {
            if(wcscmp(file_info.name, L".") == 0 || wcscmp(file_info.name, L"..") == 0)
            {
                continue;
            }
            if(com_string_match(path.c_str(), path_pattern, pattern_as_path))
            {
                list[path] = FILE_TYPE_DIR;
            }
            dir_list(path.c_str(), list, path_pattern, pattern_as_path);
        }
        else if(com_string_match(path.c_str(), path_pattern, pattern_as_path))
        {
            list[path] = FILE_TYPE_FILE;
        }
    }
    while(_wfindnext(handle, &file_info) == 0);
    _findclose(handle);
    return true;
#else
    DIR* dir = opendir(dir_root);
    if(dir == NULL)
    {
        return false;
    }

    struct dirent* ptr = NULL;
    while((ptr = readdir(dir)) != NULL)
    {
#if __ANDROID__ != 1
        if(ptr->d_name == NULL)
        {
            continue;
        }
#endif
        std::string path = dir_root;
        if(path.back() != '/')
        {
            path.append("/");
        }
        path.append(ptr->d_name);
        switch(ptr->d_type)
        {
            case DT_DIR:
                if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
                {
                    continue;
                }
                if(com_string_match(path.c_str(), path_pattern, pattern_as_path))
                {
                    list[path] = FILE_TYPE_DIR;
                }
                dir_list(path.c_str(), list, path_pattern, pattern_as_path);
                break;
            case DT_REG:
                if(com_string_match(path.c_str(), path_pattern, pattern_as_path))
                {
                    list[path] = FILE_TYPE_FILE;
                }
                break;
            case DT_SOCK:
                if(com_string_match(path.c_str(), path_pattern, pattern_as_path))
                {
                    list[path] = FILE_TYPE_SOCK;
                }
                break;
            case DT_LNK:
                if(com_string_match(path.c_str(), path_pattern, pattern_as_path))
                {
                    list[path] = FILE_TYPE_LINK;
                }
                break;
            default:
                if(com_string_match(path.c_str(), path_pattern, pattern_as_path))
                {
                    list[path] = FILE_TYPE_UNKNOWN;
                }
                break;
        }
    }

    closedir(dir);
    return true;
#endif
}

std::string PATH_TO_DOS(const char* path)
{
    if(path == NULL)
    {
        return std::string();
    }
    std::string val = path;
    com_string_replace(val, "/", "\\");
    return val;
}

std::string PATH_TO_DOS(const std::string& path)
{
    std::string val = path;
    com_string_replace(val, "/", "\\");
    return val;
}

std::string PATH_FROM_DOS(const char* path)
{
    if(path == NULL)
    {
        return std::string();
    }
    std::string val = path;
    com_string_replace(val, "\\", "/");
    return val;
}

std::string PATH_FROM_DOS(const std::string& path)
{
    std::string val = path;
    com_string_replace(val, "\\", "/");
    return val;
}

std::string PATH_TO_LOCAL(const char* path)
{
    if(path == NULL)
    {
        return std::string();
    }
    std::string val = path;
#if defined(_WIN32) || defined(_WIN64)
    com_string_replace(val, "/", "\\");
#else
    com_string_replace(val, "\\", "/");
#endif
    return val;
}

std::string PATH_TO_LOCAL(const std::string& path)
{
    std::string val = path;
#if defined(_WIN32) || defined(_WIN64)
    com_string_replace(val, "/", "\\");
#else
    com_string_replace(val, "\\", "/");
#endif
    return val;
}

std::string com_dir_system_temp()
{
#if defined(_WIN32) || defined(_WIN64)
    wchar_t buf[MAX_PATH + 1];
    int len = GetTempPathW(MAX_PATH, buf);
    if(len <= 0)
    {
        LOG_E("failed to get system temp dir,GetLastError=%d", GetLastError());
        return std::string();
    }
    else if(len >= sizeof(buf) / sizeof(wchar_t))
    {
        LOG_E("buf space not enough,require %d", len);
        return std::string();
    }
    buf[len] = L'\0';
    std::string dir = com_wstring_to_utf8(buf).toString();
    while(dir.empty() == false)
    {
        if(dir.back() != '\0')
        {
            break;
        }
        dir.pop_back();
    }
    if(dir.back() == PATH_DELIM_CHAR)
    {
        dir.pop_back();
    }
    return dir;
#else
    const char* dir = NULL;
    dir = getenv("TMPDIR");
    if(dir != NULL && dir[0] != '\0')
    {
        return dir;
    }
    dir = getenv("TMP");
    if(dir != NULL && dir[0] != '\0')
    {
        return dir;
    }
    dir = getenv("TEMP");
    if(dir != NULL && dir[0] != '\0')
    {
        return dir;
    }
    dir = getenv("TEMPDIR");
    if(dir != NULL && dir[0] != '\0')
    {
        return dir;
    }
    return "/tmp";
#endif
}

bool com_dir_exist(const char* dir)
{
    return FILE_TYPE_DIR == com_file_type(dir);
}

bool com_dir_create(const char* full_path)
{
    if(full_path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    std::vector<std::string> paths = com_string_split(full_path, PATH_DELIM_STR);
    if(paths.empty())
    {
        LOG_E("path incorrect");
        return false;
    }

    std::string path;
    for(size_t i = 0; i < paths.size(); i++)
    {
        path.append(paths[i]);
        if(path.empty())
        {
            path += PATH_DELIM_STR;
            continue;
        }
        int file_type = com_file_type(path.c_str());
        if(file_type != FILE_TYPE_NOT_EXIST)
        {
            if(file_type != FILE_TYPE_DIR)
            {
                LOG_E("file %s already exist:%d", path.c_str(), file_type);
                return false;
            }
            path += PATH_DELIM_STR;
            continue;
        }
#if defined(_WIN32) || defined(_WIN64)
        if(path.back() != ':' && _wmkdir(com_wstring_from_utf8(ComBytes(path)).c_str()) != 0)
        {
            LOG_W("make dir %s failed, dir may already exist", path.c_str());
            return false;
        }
#else
        if(mkdir(path.c_str(), 0775) != 0)
        {
            return false;
        }
#endif
        path += PATH_DELIM_STR;
    }
    return true;
}

int64 com_dir_size_max(const char* dir)
{
    if(dir == NULL)
    {
        return -1;
    }
    int64 total_size_byte = 0;
#if defined(_WIN32) || defined(_WIN64)
    int64 free_size_byte = 0;
    if(GetDiskFreeSpaceExW(com_wstring_from_utf8(ComBytes(dir)).c_str(), NULL, (ULARGE_INTEGER*)&total_size_byte,
                           (ULARGE_INTEGER*)&free_size_byte) == 0)
    {
        return -1;
    }
    return total_size_byte;
#else
    int64 block_size = 0;
    struct statfs dirfs_info;
    if(statfs(dir, &dirfs_info) != 0)
    {
        return -1;
    }
    block_size = dirfs_info.f_bsize;
    total_size_byte = block_size * dirfs_info.f_blocks;
    //total_size_byte = total_size_byte >> 20;
    //free_size_byte = block_size * dirfs_info.f_bfree;
    //free_size_byte = free_size_byte >> 20;
#endif
    return total_size_byte;
}

int64 com_dir_size_used(const char* dir)
{
    if(dir == NULL)
    {
        return -1;
    }
    int64 total_size_byte = 0;
    int64 free_size_byte = 0;
#if defined(_WIN32) || defined(_WIN64)
    if(GetDiskFreeSpaceExW(com_wstring_from_utf8(ComBytes(dir)).c_str(), NULL, (ULARGE_INTEGER*)&total_size_byte,
                           (ULARGE_INTEGER*)&free_size_byte) == 0)
    {
        return -1;
    }
    return total_size_byte - free_size_byte;
#else
    int64 block_size = 0;
    struct statfs dirfs_info;
    if(statfs(dir, &dirfs_info) != 0)
    {
        return -1;
    }
    block_size = dirfs_info.f_bsize;
    total_size_byte = block_size * dirfs_info.f_blocks;
    //total_size_byte = total_size_byte >> 20;
    free_size_byte = block_size * dirfs_info.f_bfree;
    //free_size_byte = free_size_byte >> 20;
#endif
    return total_size_byte - free_size_byte;
}

int64 com_dir_size_freed(const char* dir)
{
    if(dir == NULL)
    {
        return -1;
    }
    int64 free_size_byte = 0;
#if defined(_WIN32) || defined(_WIN64)
    int64 total_size_byte = 0;
    if(GetDiskFreeSpaceExW(com_wstring_from_utf8(ComBytes(dir)).c_str(), NULL, (ULARGE_INTEGER*)&total_size_byte,
                           (ULARGE_INTEGER*)&free_size_byte) == 0)
    {
        return -1;
    }
    return free_size_byte;
#else
    int64 block_size = 0;
    struct statfs dirfs_info;
    if(statfs(dir, &dirfs_info) != 0)
    {
        return -1;
    }
    block_size = dirfs_info.f_bsize;
    //total_size_byte = block_size * dirfs_info.f_blocks;
    //total_size_byte = total_size_byte >> 20;
    free_size_byte = block_size * dirfs_info.f_bfree;
    //free_size_byte = free_size_byte >> 20;
#endif
    return free_size_byte;
}

int com_dir_remove(const char* dir_path)
{
    if(dir_path == NULL)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    struct _wfinddata_t file_info;
    intptr_t handle = _wfindfirst(com_wstring_format(L"%s%c*.*", com_wstring_from_utf8(ComBytes(dir_path)).c_str(), PATH_DELIM_WCHAR).c_str(), &file_info);
    if(handle == -1)
    {
        _rmdir(dir_path);
        return false;
    }
    do
    {
        std::string path = dir_path;
        path.append(PATH_DELIM_STR);
        path.append(com_wstring_to_utf8(file_info.name).toString());
        if(file_info.attrib & _A_SUBDIR)
        {
            if(wcscmp(file_info.name, L".") == 0 || wcscmp(file_info.name, L"..") == 0)
            {
                continue;
            }
            com_dir_remove(path.c_str());
        }
        else
        {
            com_file_remove(path.c_str());
        }
    }
    while(_wfindnext(handle, &file_info) == 0);
    _findclose(handle);
    _rmdir(dir_path);
    return 0;
#else
    DIR* dir = opendir(dir_path);
    if(dir == NULL)
    {
        remove(dir_path);
        return -1;
    }

    struct dirent* ptr = NULL;
    while((ptr = readdir(dir)) != NULL)
    {
#if __ANDROID__ != 1
        if(ptr->d_name == NULL)
        {
            continue;
        }
#endif
        if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
        {
            continue;
        }
        std::string path = dir_path;
        path.append("/");
        path.append(ptr->d_name);

        if(ptr->d_type == DT_DIR)
        {
            com_dir_remove(path.c_str());
        }
        else
        {
            com_file_remove(path.c_str());
        }
    }

    remove(dir_path);
    closedir(dir);
    return 0;
#endif
}

void com_dir_clear(const char* dir_path)
{
    std::map<std::string, int> files;
    com_dir_list(dir_path, files, false);
    for(auto it = files.begin(); it != files.end(); it++)
    {
        if(it->second == FILE_TYPE_DIR)
        {
            com_dir_remove(it->first.c_str());
        }
        else
        {
            com_file_remove(it->first.c_str());
        }
    }
}

bool com_dir_list(const char* dir_path, std::map<std::string, int>& list, bool recursion)
{
    if(dir_path == NULL || dir_path[0] == '\0')
    {
        return false;
    }
    const char* p = dir_path;
    const char* p_delim = NULL;
    do
    {
        if(*p == PATH_DELIM_CHAR)
        {
            p_delim = p;
            continue;
        }
        if(*p == '?' || *p == '*' || *p == '[' || *p == ']')
        {
            //has pattern
            return dir_list(p_delim == NULL ? "." : std::string(dir_path, (int)(p_delim - dir_path)).c_str(),
                            list, dir_path, recursion ? false : true);
        }
    }
    while(*p++);
    std::string dir_pattern = dir_path;
    if(dir_pattern.back() != PATH_DELIM_CHAR)
    {
        dir_pattern.append(PATH_DELIM_STR);
    }
    dir_pattern.append("*");

    return dir_list(dir_path, list, dir_pattern.c_str(), recursion ? false : true);
}

std::string com_path_name(const char* path)
{
    FilePath file_path(path);
    return file_path.getName();
}

std::string com_path_name_without_suffix(const char* path)
{
    FilePath file_path(path);
    return file_path.getNameWithoutSuffix();
}

std::string com_path_suffix(const char* path)
{
    FilePath file_path(path);
    return file_path.getSuffix();
}

std::string com_path_dir(const char* path)
{
    FilePath file_path(path);
    return file_path.getDir();
}

int com_file_type(const char* file)
{
    if(file == NULL)
    {
        return FILE_TYPE_NOT_EXIST;
    }
#if defined(_WIN32) || defined(_WIN64)
    //_stat may has errno=132 error
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    if(GetFileAttributesExW(com_wstring_from_utf8(file).c_str(), GetFileExInfoStandard, &attr) == false)
    {
        return FILE_TYPE_NOT_EXIST;
    }

    if(attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return FILE_TYPE_DIR;
    }
    return FILE_TYPE_FILE;
#else
    struct stat buf;
    int ret = stat(file, &buf);
    if(0 != ret)
    {
        if(errno == ENOENT)
        {
            return FILE_TYPE_NOT_EXIST;
        }
        return FILE_TYPE_UNKNOWN;
    }
    if(buf.st_mode & S_IFDIR)
    {
        return FILE_TYPE_DIR;
    }
    if(buf.st_mode & S_IFREG)
    {
        return FILE_TYPE_FILE;
    }
    if(buf.st_mode & S_IFLNK)
    {
        return FILE_TYPE_LINK;
    }
    if(buf.st_mode & S_IFSOCK)
    {
        return FILE_TYPE_SOCK;
    }
#endif
    return FILE_TYPE_UNKNOWN;
}

int com_file_type(int fd)
{
    if(fd < 0)
    {
        return FILE_TYPE_UNKNOWN;
    }
#if defined(_WIN32) || defined(_WIN64)
    //_fstat may has errno=132 error
    BY_HANDLE_FILE_INFORMATION info = {0};
    if(GetFileInformationByHandle((HANDLE)_get_osfhandle(fd), &info) == false)
    {
        return FILE_TYPE_UNKNOWN;
    }
    if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return FILE_TYPE_DIR;
    }
    return FILE_TYPE_FILE;
#else
    struct stat buf;
    int ret = fstat(fd, &buf);
    if(0 != ret)
    {
        if(errno == ENOENT)
        {
            return FILE_TYPE_NOT_EXIST;
        }
        return FILE_TYPE_UNKNOWN;
    }
    if(buf.st_mode & S_IFDIR)
    {
        return FILE_TYPE_DIR;
    }
    if(buf.st_mode & S_IFREG)
    {
        return FILE_TYPE_FILE;
    }
    if(buf.st_mode & S_IFLNK)
    {
        return FILE_TYPE_LINK;
    }
    if(buf.st_mode & S_IFSOCK)
    {
        return FILE_TYPE_SOCK;
    }
#endif
    return FILE_TYPE_UNKNOWN;
}

int com_file_type(FILE* file)
{
    return com_file_type(com_file_get_fd(file));
}

int64 com_file_size(int fd)
{
    if(fd < 0)
    {
        return -1;
    }
    unsigned long filesize = 0;
#if defined(_WIN32) || defined(_WIN64)
    //_fstat may has errno=132 error
    int64 pos = _lseeki64(fd, 0, SEEK_CUR);
    int64 size = _lseeki64(fd, 0, SEEK_END);
    _lseeki64(fd, pos, SEEK_SET);
    return size;
#else
    struct stat statbuff;
    int ret = fstat(fd, &statbuff);
    if(ret == 0)
    {
        filesize = statbuff.st_size;
    }
    else
    {
        LOG_E("failed to get file size,ret=%d,errno=%d", ret, errno);
    }
#endif
    return filesize;
}

int64 com_file_size(FILE* file)
{
    if(file == NULL)
    {
        return -1;
    }
    return com_file_size(com_file_get_fd(file));
}

int64 com_file_size(const char* file_path)
{
    if(file_path == NULL)
    {
        return -1;
    }
    unsigned long filesize = 0;
#if defined(_WIN32) || defined(_WIN64)
    //_stat may has errno=132 error
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    if(GetFileAttributesExW(com_wstring_from_utf8(file_path).c_str(), GetFileExInfoStandard, &attr) == false)
    {
        LOG_E("failed to get file size,file=%s,errno=%d", file_path, errno);
        return -1;
    }
    return ((int64)attr.nFileSizeHigh << 32) + attr.nFileSizeLow;
#else
    struct stat statbuff;
    int ret = stat(file_path, &statbuff);
    if(ret == 0)
    {
        filesize = statbuff.st_size;
    }
    else
    {
        LOG_E("failed to get file size,file=%s,ret=%d,errno=%d", file_path, ret, errno);
    }
#endif
    return filesize;
}

uint64 com_file_get_id(const char* file_path)
{
    if(file_path == NULL)
    {
        return 0;
    }
    uint64 inode = 0;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE fp = CreateFileW(com_wstring_from_utf8(file_path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if(fp == NULL)
    {
        return 0;
    }
    BY_HANDLE_FILE_INFORMATION info;
    if(GetFileInformationByHandle(fp, &info) == false)
    {
        CloseHandle(fp);
        return 0;
    }
    inode = ((uint64)info.nFileIndexHigh) << 32 | info.nFileIndexLow;
    CloseHandle(fp);
#else
    if(file_path == NULL)
    {
        return 0;
    }
    struct stat statbuff;
    if(stat(file_path, &statbuff) == 0)
    {
        inode = statbuff.st_ino;
    }
#endif
    return inode;
}

uint32 com_file_get_change_time(const char* file_path)
{
    if(file_path == NULL)
    {
        return 0;
    }
    uint32 timestamp = 0;
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    if(GetFileAttributesExW(com_wstring_from_utf8(file_path).c_str(), GetFileExInfoStandard, &attr) == false)
    {
        return 0;
    }
    timestamp = (((int64)attr.ftCreationTime.dwHighDateTime << 32) + attr.ftCreationTime.dwLowDateTime) / 100000000;
#else
    struct stat statbuff;
    if(stat(file_path, &statbuff) == 0)
    {
        timestamp = statbuff.st_ctime;
    }
#endif
    return timestamp;
}

uint32 com_file_get_modify_time(const char* file_path)
{
    if(file_path == NULL)
    {
        return 0;
    }
    uint32 timestamp = 0;
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    if(GetFileAttributesExW(com_wstring_from_utf8(file_path).c_str(), GetFileExInfoStandard, &attr) == false)
    {
        return 0;
    }
    timestamp = (((int64)attr.ftLastWriteTime.dwHighDateTime << 32) + attr.ftLastWriteTime.dwLowDateTime) / 100000000;
#else
    struct stat statbuff;
    if(stat(file_path, &statbuff) == 0)
    {
        timestamp = statbuff.st_mtime;
    }
#endif
    return timestamp;
}

uint32 com_file_get_access_time(const char* file_path)
{
    if(file_path == NULL)
    {
        return 0;
    }
    uint32 timestamp = 0;
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    if(GetFileAttributesExW(com_wstring_from_utf8(file_path).c_str(), GetFileExInfoStandard, &attr) == false)
    {
        return 0;
    }
    timestamp = (((int64)attr.ftLastAccessTime.dwHighDateTime << 32) + attr.ftLastAccessTime.dwLowDateTime) / 100000000;
#else
    struct stat statbuff;
    if(stat(file_path, &statbuff) == 0)
    {
        timestamp = statbuff.st_atime;
    }
#endif
    return timestamp;
}

bool com_file_set_access_time(const char* path, uint32 time_s)
{
    if(path == NULL)
    {
        return false;
    }
    if(time_s == 0)
    {
        time_s = com_time_rtc_s();
    }
#if defined(_WIN32) || defined(_WIN64)
    struct _utimbuf buf;
    buf.actime = time_s;
    buf.modtime = com_file_get_modify_time(path);
    _wutime(com_wstring_from_utf8(path).c_str(), &buf);
#else
    struct utimbuf buf;
    buf.actime = time_s;
    buf.modtime = com_file_get_modify_time(path);
    utime(path, &buf);
#endif
    return true;
}

bool com_file_set_modify_time(const char* path, uint32 time_s)
{
    if(path == NULL)
    {
        return false;
    }
    if(time_s == 0)
    {
        time_s = com_time_rtc_s();
    }
#if defined(_WIN32) || defined(_WIN64)
    struct _utimbuf buf;
    buf.actime = time_s;
    buf.modtime = time_s;
    _wutime(com_wstring_from_utf8(path).c_str(), &buf);
#else
    struct utimbuf buf;
    buf.actime = time_s;
    buf.modtime = time_s;
    utime(path, &buf);
#endif
    return true;
}

bool com_file_set_owner(const char* file, int uid, int gid)
{
#if __linux__==1
    if(file == NULL || uid < 0 || gid < 0)
    {
        return false;
    }
    return (chown(file, uid, gid) == 0);
#else
    return false;
#endif
}

bool com_file_set_mod(const char* file, int mod)
{
#if __linux__==1
    if(file == NULL || mod < 0)
    {
        return false;
    }
    return (chmod(file, mod) == 0);
#else
    return false;
#endif
}

bool com_file_copy(const char* file_path_to, const char* file_path_from, bool append)
{
    if(file_path_to == NULL || file_path_from == NULL)
    {
        return false;
    }
    std::string dir = com_path_dir(file_path_to);
    if(com_file_type(dir.c_str()) == FILE_TYPE_NOT_EXIST)
    {
        com_dir_create(dir.c_str());
    }
#if defined(_WIN32) || defined(_WIN64)
    std::wifstream ifs(com_wstring_from_utf8(file_path_from), std::ios::binary);
    std::wofstream ofs(com_wstring_from_utf8(file_path_to), append ? std::ios::binary | std::ios_base::app : std::ios::binary);
#else
    std::ifstream ifs(file_path_from, std::ios::binary);
    std::ofstream ofs(file_path_to, append ? std::ios::binary | std::ios_base::app : std::ios::binary);
#endif
    ofs << ifs.rdbuf();
    ofs.flush();
    ifs.close();
    ofs.close();
    return true;
}

bool com_file_truncate(FILE* file, int64 size)
{
    if(file == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    int64 pos = com_file_seek_get(file);
    com_file_seek_tail(file);
    int64 total_size = com_file_seek_get(file);
    if(size < 0)
    {
        size = total_size;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(_chsize_s(com_file_get_fd(file), total_size - size) != 0)
    {
        com_file_seek_set(file, pos);
        LOG_E("failed,total_size=%lld,size=%lld", total_size, size);
        return false;
    }
#else
    if(ftruncate(com_file_get_fd(file), total_size - size) != 0)
    {
        com_file_seek_set(file, pos);
        LOG_E("failed,total_size=%lld,size=%lld", total_size, size);
        return false;
    }
    com_file_seek_set(file, pos);
#endif
    return true;
}

bool com_file_truncate(const char* file_path, int64 size)
{
    if(file_path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    FILE* file = com_file_open(file_path, "ab");
    if(file == NULL)
    {
        LOG_E("failed to open file,errno=%d", errno);
        return false;
    }
    com_file_seek_tail(file);
    int64 total_size = com_file_seek_get(file);
    if(size < 0)
    {
        size = total_size;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(_chsize_s(com_file_get_fd(file), total_size - size) != 0)
    {
        com_file_close(file);
        LOG_E("failed,total_size=%lld,size=%lld", total_size, size);
        return false;
    }
#else
    if(ftruncate(com_file_get_fd(file), total_size - size) != 0)
    {
        com_file_close(file);
        LOG_E("failed,total_size=%lld,size=%lld", total_size, size);
        return false;
    }
    lseek(com_file_get_fd(file), total_size - size, SEEK_SET);
#endif
    com_file_flush(file);
    com_file_close(file);
    return true;
}

bool com_file_clean(const char* file_path)
{
    return com_file_truncate(file_path, -1);
}

bool com_file_erase(const char* file_path, uint8 val)
{
    FILE* fp = com_file_open(file_path, "wb");
    if(fp == NULL)
    {
        LOG_E("failed to open file,errno=%d", errno);
        return false;
    }
    com_file_seek_tail(fp);
    int64 total_size = com_file_seek_get(fp);
    if(total_size <= 0)
    {
        com_file_close(fp);
        LOG_E("file is empty:%s", file_path);
        return false;
    }
    com_file_seek_head(fp);
    uint8 buf[1024];
    memset(buf, val, sizeof(buf));
    for(int64 i = 0; i < total_size / (int64)sizeof(buf); i++)
    {
        com_file_write(fp, buf, sizeof(buf));
    }
    if(total_size % sizeof(buf))
    {
        com_file_write(fp, buf, total_size % sizeof(buf));
    }
    com_file_flush(fp);
    com_file_close(fp);
    return true;
}

bool com_file_seek_head(FILE* file)
{
    if(file == NULL)
    {
        return false;
    }
    return (fseek(file, 0L, SEEK_SET) == 0);
}

bool com_file_seek_head(int fd)
{
    if(fd < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return (_lseeki64(fd, 0L, SEEK_SET) != -1);
#else
    return (lseek(fd, 0L, SEEK_SET) != -1);
#endif
}

bool com_file_seek_tail(FILE* file)
{
    if(file == NULL)
    {
        return false;
    }
    return (fseek(file, 0L, SEEK_END) == 0);
}

bool com_file_seek_tail(int fd)
{
    if(fd < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return (_lseeki64(fd, 0L, SEEK_END) != -1);
#else
    return (lseek(fd, 0L, SEEK_END) != -1);
#endif
}

bool com_file_seek_step(FILE* file, int64 pos)
{
    if(file == NULL)
    {
        return false;
    }
    return (fseek(file, pos, SEEK_CUR) == 0);
}

bool com_file_seek_step(int fd, int64 pos)
{
    if(fd < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return (_lseeki64(fd, pos, SEEK_CUR) != -1);
#else
    return (lseek(fd, pos, SEEK_CUR) != -1);
#endif
}

bool com_file_seek_set(FILE* file, int64 pos)
{
    if(file == NULL)
    {
        return false;
    }
    if(pos >= 0)
    {
        return (fseek(file, pos, SEEK_SET) == 0);
    }
    else
    {
        return (fseek(file, pos, SEEK_END) == 0);
    }
}

bool com_file_seek_set(int fd, int64 pos)
{
    if(fd < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(pos >= 0)
    {
        return (_lseeki64(fd, pos, SEEK_SET) != -1);
    }
    else
    {
        return (_lseeki64(fd, pos, SEEK_END) != -1);
    }
#else
    if(pos >= 0)
    {
        return (lseek(fd, pos, SEEK_SET) != -1);
    }
    else
    {
        return (lseek(fd, pos, SEEK_END) != -1);
    }
#endif
}

int64 com_file_seek_get(FILE* file)
{
    if(file == NULL)
    {
        return -1;
    }
    return ftell(file);
}

int64 com_file_seek_get(int fd)
{
    if(fd < 0)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    return _lseeki64(fd, 0, SEEK_CUR);
#else
    return lseek(fd, 0, SEEK_CUR);
#endif
}

bool com_file_exist(const char* file_path)
{
    return FILE_TYPE_FILE == com_file_type(file_path);
}

bool com_file_create(const char* file_path)
{
    FILE* file = com_file_open(file_path, "w+");
    com_file_close(file);
    return file != NULL;
}

bool com_file_rename(const char* file_name_new, const char* file_name_old)
{
    if(com_string_is_empty(file_name_new) || com_string_is_empty(file_name_old))
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    return (_wrename(com_wstring_from_utf8(file_name_old).c_str(), com_wstring_from_utf8(file_name_new).c_str()) == 0);
#else
    return (rename(file_name_old, file_name_new) == 0);
#endif
}

FILE* com_file_open(const char* file_path, const char* flag)
{
    if(com_string_is_empty(file_path) || com_string_is_empty(flag))
    {
        return NULL;
    }
#if defined(_WIN32) || defined(_WIN64)
    return _wfopen(com_wstring_from_utf8(file_path).c_str(), com_wstring_from_utf8(flag).c_str());
#else
    return fopen(file_path, flag);
#endif
}

ComBytes com_file_read(const char* file, int size, int64 offset)
{
    if(com_string_is_empty(file) || size <= 0)
    {
        return ComBytes();
    }
    FILE* fp = com_file_open(file, "rb");
    if(fp == NULL)
    {
        return ComBytes();
    }
    if(offset >= 0)
    {
        com_file_seek_set(fp, offset);
    }

    ComBytes data;
    uint8 buf[1024];

    for(size_t i = 0; i < size / sizeof(buf); i++)
    {
        int ret = com_file_read(fp, buf, sizeof(buf));
        if(ret <= 0)
        {
            break;
        }
        data.append(buf, ret);
    }

    if(size % sizeof(buf))
    {
        int ret = com_file_read(fp, buf, size % sizeof(buf));
        data.append(buf, ret);
    }

    com_file_close(fp);
    return data;
}

ComBytes com_file_read(FILE* file, int size)
{
    if(file == NULL || size <= 0)
    {
        return ComBytes();
    }

    ComBytes data;
    uint8 buf[1024];

    for(size_t i = 0; i < size / sizeof(buf); i++)
    {
        int ret = com_file_read(file, buf, sizeof(buf));
        if(ret <= 0)
        {
            break;
        }
        data.append(buf, ret);
    }

    if(size % sizeof(buf))
    {
        int ret = com_file_read(file, buf, size % sizeof(buf));
        data.append(buf, ret);
    }

    return data;
}

ComBytes com_file_read(int fd, int size)
{
    if(fd < 0 || size <= 0)
    {
        return ComBytes();
    }

    ComBytes data;
    uint8 buf[1024];

    for(size_t i = 0; i < size / sizeof(buf); i++)
    {
        int ret = com_file_read(fd, buf, sizeof(buf));
        if(ret <= 0)
        {
            break;
        }
        data.append(buf, ret);
    }

    if(size % sizeof(buf))
    {
        int ret = com_file_read(fd, buf, size % sizeof(buf));
        data.append(buf, ret);
    }

    return data;
}

int64 com_file_read(FILE* file, void* buf, int64 size)
{
    if(file == NULL || buf == NULL || size <= 0)
    {
        LOG_E("arg incorrect,file=%p,buf=%p,size=%lld", file, buf, size);
        return -1;
    }
    int64 size_readed = 0;
#if 1
    do
    {
        int ret = fread((uint8*)buf + size_readed, 1, size - size_readed, file);
        if(ret <= 0)
        {
            if(feof(file))
            {
                break;
            }
            if(ferror(file))
            {
                return -1;
            }
        }
        else
        {
            size_readed += ret;
        }
    }
    while(size_readed < size);

#else
    do
    {
        size_readed += fread((uint8*)buf + size_readed, 1, size - size_readed, file);
    }
    while(feof(file) == 0 && ferror(file) == 0 && size_readed < size);
#endif

    return size_readed;
}

int64 com_file_read(int fd, void* buf, int64 size)
{
    if(fd < 0 || buf == NULL || size <= 0)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    int64 size_readed = 0;

    do
    {
#if defined(_WIN32) || defined(_WIN64)
        int ret = _read(fd, (uint8*)buf + size_readed, size - size_readed);
#else
        int ret = read(fd, (uint8*)buf + size_readed, size - size_readed);
#endif
        if(ret < 0)
        {
            LOG_E("read error,fd=%d,size_readed=%lld,errno=%d", fd, size_readed, errno);
            return -1;
        }
        else if(ret == 0)
        {
            break;
        }
        else
        {
            size_readed += ret;
        }
    }
    while(size_readed < size);

    return size_readed;
}

ComBytes com_file_read_until(FILE* file, const char* str)
{
    return com_file_read_until(file, (const uint8*)str, com_string_len(str));
}

ComBytes com_file_read_until(FILE* file, const uint8* data, int data_size)
{
    if(file == NULL || data == NULL || data_size <= 0)
    {
        return ComBytes();
    }

    ComBytes result;
    int match_count = 0;
    uint8 val = 0;
    while(true)
    {
        if(fread(&val, 1, 1, file) != 1)
        {
            break;
        }
        result.append(val);
        if(val != data[match_count])
        {
            match_count = 0;
            continue;
        }
        match_count++;
        if(match_count == data_size)
        {
            result.removeTail(data_size);
            fseek(file, -1 * data_size, SEEK_CUR);
            break;
        }
    }
    return result;
}

ComBytes com_file_read_until(FILE* file, std::function<bool(uint8)> func)
{
    if(file == NULL || func == NULL)
    {
        return ComBytes();
    }

    ComBytes result;
    uint8 val = 0;
    while(true)
    {
        if(fread(&val, 1, 1, file) != 1)
        {
            break;
        }
        if(func(val))
        {
            fseek(file, -1, SEEK_CUR);
            break;
        }
        result.append(val);
    }
    return result;
}

int64 com_file_find(const char* file, const char* key, int64 offset)
{
    FILE* fp = com_file_open(file, "rb");
    if(offset >= 0)
    {
        com_file_seek_set(fp, offset);
    }
    int64 ret = com_file_find(fp, key);
    com_file_close(fp);
    return ret;
}

int64 com_file_find(const char* file, const uint8* key, int key_size, int64 offset)
{
    FILE* fp = com_file_open(file, "rb");
    if(offset >= 0)
    {
        com_file_seek_set(fp, offset);
    }
    int64 ret = com_file_find(fp, key, key_size);
    com_file_close(fp);
    return ret;
}

int64 com_file_find(FILE* file, const char* key)
{
    return com_file_find(file, (const uint8*)key, com_string_len(key));
}

int64 com_file_find(FILE* file, const uint8* key, int key_size)
{
    if(file == NULL || key == NULL || key_size <= 0)
    {
        return -1;
    }
    int match_count = 0;
    while(true)
    {
        uint8 val = 0;
        if(fread(&val, 1, 1, file) != 1)
        {
            break;
        }
        if(val != key[match_count])
        {
            match_count = 0;
            continue;
        }
        match_count++;
        if(match_count == key_size)
        {
            com_file_seek_step(file, -1 * key_size);
            return com_file_seek_get(file);
        }
    }
    return -3;
}

int64 com_file_rfind(const char* file, const char* key, int64 offset)
{
    FILE* fp = com_file_open(file, "rb");
    if(offset < 0)
    {
        com_file_seek_set(fp, offset);
    }
    int64 ret = com_file_rfind(fp, key);
    com_file_close(fp);
    return ret;
}

int64 com_file_rfind(const char* file, const uint8* key, int key_size, int64 offset)
{
    FILE* fp = com_file_open(file, "rb");
    if(offset < 0)
    {
        com_file_seek_set(fp, offset);
    }
    int64 ret = com_file_rfind(fp, key, key_size);
    com_file_close(fp);
    return ret;
}

int64 com_file_rfind(FILE* file, const char* key)
{
    return com_file_rfind(file, (const uint8*)key, com_string_len(key));
}

int64 com_file_rfind(FILE* file, const uint8* key, int key_size)
{
    if(file == NULL || key == NULL || key_size <= 0)
    {
        return -1;
    }
    com_file_seek_tail(file);
    if(fseek(file, -1, SEEK_CUR) != 0)
    {
        return -2;
    }
    bool checked_first_char = false;
    int match_count = 0;
    while(true)
    {
        uint8 val = 0;
        if(fread(&val, 1, 1, file) != 1)
        {
            break;
        }
        if(fseek(file, -2, SEEK_CUR) != 0)
        {
            int64 pos_cur = ftell(file);
            if(pos_cur > 1)
            {
                break;
            }

            checked_first_char = true;
            com_file_seek_head(file);
        }
        if(val != key[key_size - match_count - 1])
        {
            match_count = 0;
            continue;
        }
        match_count++;
        if(match_count == key_size)
        {
            if(!checked_first_char)
            {
                com_file_seek_step(file, 1);
            }
            return com_file_seek_get(file);
        }
    }
    return -3;
}

bool com_file_readline(FILE* file, char* buf, int size)
{
    if(file == NULL || buf == NULL || size <= 0)
    {
        return false;
    }
    std::string line;
    if(com_file_readline(file, line) == false)
    {
        return false;
    }
    strncpy(buf, line.c_str(), size - 1);
    buf[size - 1] = '\0';
    return true;
}

bool com_file_readline(FILE* file, std::string& line)
{
    if(file == NULL)
    {
        return false;
    }
    std::string result;
    char buf[1024];
    char* ret = NULL;
    while((ret = fgets(buf, sizeof(buf), file)) != NULL)
    {
        result.append(buf);
        if(result.rfind('\n') != std::string::npos)
        {
            break;
        }
    }
    if(result.back() == '\n')
    {
        result.pop_back();
    }
    if(ret == NULL && result.empty())
    {
        return false;
    }
    line = result;
    return true;
}

ComBytes com_file_readall(const std::string& file_path, int64 offset)
{
    return com_file_readall(file_path.c_str());
}

ComBytes com_file_readall(const char* file_path, int64 offset)
{
    ComBytes bytes;
    FILE* file = com_file_open(file_path, "rb");
    if(file == NULL)
    {
        return bytes;
    }
    if(offset >= 0)
    {
        com_file_seek_set(file, offset);
    }
    uint8 buf[1024];
    int size = 0;
    while((size = com_file_read(file, buf, sizeof(buf))) > 0)
    {
        bytes.append(buf, size);
    }
    com_file_close(file);
    return bytes;
}

ComBytes com_file_readall(int fd, int64 offset)
{
    if(fd < 0)
    {
        return ComBytes();
    }
    if(offset >= 0)
    {
#if defined(_WIN32) || defined(_WIN64)
        _lseeki64(fd, offset, SEEK_SET);
#else
        lseek(fd, offset, SEEK_SET);
#endif
    }
    uint8 buf[1024];
    int size = 0;
    ComBytes bytes;
    while((size = com_file_read(fd, buf, sizeof(buf))) > 0)
    {
        bytes.append(buf, size);
    }
    return bytes;
}

int64 com_file_write(FILE* file, const void* buf, int size)
{
    if(file == NULL || buf == NULL || size <= 0)
    {
        return 0;
    }

    int len = -1;
    int64 total_size = 0;
    while(total_size < size
            && (len = fwrite((uint8*)buf + total_size, 1, size - total_size, file)) > 0)
    {
        total_size += len;
    }
    if(len > 0)
    {
        return total_size;
    }
    LOG_E("len=%d,total_size=%lld,size=%d,errno=%d", len, total_size, size, errno);
    return -1;
}


int64 com_file_writef(FILE* fp, const char* fmt, ...)
{
    if(fp == NULL || fmt == NULL)
    {
        return -1;
    }
    std::string str;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len > 0)
    {
        len += 1;  //上面返回的长度不包含\0，这里加上
        std::vector<char> buf(len);
        va_start(args, fmt);
        len = vsnprintf(buf.data(), len, fmt, args);
        va_end(args);
        if(len > 0)
        {
            str.assign(buf.data(), len);
        }
    }
    return com_file_write(fp, str.data(), str.length());
}

int64 com_file_writef(const char* file, const char* fmt, ...)
{
    if(file == NULL || fmt == NULL)
    {
        return -1;
    }

    std::string str;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len > 0)
    {
        len += 1;  //上面返回的长度不包含\0，这里加上
        std::vector<char> buf(len);
        va_start(args, fmt);
        len = vsnprintf(buf.data(), len, fmt, args);
        va_end(args);
        if(len > 0)
        {
            str.assign(buf.data(), len);
        }
    }

    FILE* fp = com_file_open(file, "w+b");
    if(fp == NULL)
    {
        return -1;
    }
    int64 ret = com_file_write(fp, str.data(), str.length());
    com_file_flush(fp);
    com_file_close(fp);
    return ret;
}

int64 com_file_appendf(const char* file, const char* fmt, ...)
{
    if(file == NULL || fmt == NULL)
    {
        return -1;
    }

    std::string str;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len > 0)
    {
        len += 1;  //上面返回的长度不包含\0，这里加上
        std::vector<char> buf(len);
        va_start(args, fmt);
        len = vsnprintf(buf.data(), len, fmt, args);
        va_end(args);
        if(len > 0)
        {
            str.assign(buf.data(), len);
        }
    }

    FILE* fp = com_file_open(file, "a+b");
    if(fp == NULL)
    {
        return -1;
    }
    int64 ret = com_file_write(fp, str.data(), str.length());
    com_file_flush(fp);
    com_file_close(fp);
    return ret;
}

void com_file_close(FILE* file)
{
    if(file)
    {
        fclose(file);
    }
    return;
}

void com_file_flush(FILE* file)
{
    if(file)
    {
        fflush(file);
    }
    return;
}

void com_file_sync(FILE* file)
{
#if __linux__ == 1
    if(file)
    {
        fsync(com_file_get_fd(file));
    }
#endif
}

bool com_file_crop(const char* file_name, uint8 start_percent_keepd, uint8 end_percent_keeped)
{
    if(file_name == NULL || start_percent_keepd >= 100 || end_percent_keeped == 0)
    {
        return false;
    }
    if(end_percent_keeped > 100)
    {
        end_percent_keeped = 100;
    }
    uint64 file_size = com_file_size(file_name);
    if(file_size == 0)
    {
        return false;
    }
    FILE* file_in = com_file_open(file_name, "rb");
    if(file_in == NULL)
    {
        return false;
    }
    char file_temp[256];
    snprintf(file_temp, sizeof(file_temp), "%s.bak", file_name);
    FILE* file_out = com_file_open(file_temp, "w+");
    if(file_out == NULL)
    {
        com_file_close(file_in);
        return false;
    }
    uint64 start_size = file_size * start_percent_keepd / 100;
    uint64 end_size = file_size * end_percent_keeped / 100;
    uint64 block_size = file_size / 100;
    if(block_size > 1024 * 1024)
    {
        block_size = 1024 * 1024;
    }
    uint8* buf = (uint8*)calloc(1, block_size);
    if(buf == NULL)
    {
        com_file_close(file_in);
        com_file_close(file_out);
        return false;
    }
    uint64 total_size = 0;
    int size = 0;
    while(true)
    {
        size = com_file_read(file_in, buf, sizeof(buf));
        if(size <= 0)
        {
            break;
        }
        total_size += size;
        if(total_size > start_size && total_size < end_size)
        {
            com_file_write(file_out, buf, size);
        }
        else if(total_size >= end_size)
        {
            break;
        }
    }
    free(buf);
    com_file_flush(file_out);
    com_file_close(file_out);
    com_file_close(file_in);
    com_file_remove(file_name);
    return com_file_rename(file_name, file_temp);
}

bool com_file_remove(const char* file_name)
{
    if(file_name == NULL)
    {
        return false;
    }
    int ret = 0;
#if defined(_WIN32) || defined(_WIN64)
    std::wstring file_name_wstr = com_wstring_from_utf8(file_name);
    SetFileAttributesW(file_name_wstr.c_str(), FILE_ATTRIBUTE_NORMAL);
    ret = DeleteFileW(file_name_wstr.c_str());
#else
    ret = remove(file_name);
#endif
    if(ret != 0)
    {
        LOG_D("failed to remove %s,ret=%d", file_name, errno);
        return false;
    }
    return true;
}

int com_file_get_fd(FILE* file)
{
    if(file == NULL)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    return _fileno(file);
#else
    return fileno(file);
#endif
}

bool com_file_lock(FILE* file, bool allow_share_read, bool wait)
{
    if(file == NULL)
    {
        return false;
    }
    return com_file_lock(com_file_get_fd(file), allow_share_read, wait);
}

bool com_file_lock(int fd, bool allow_share_read, bool wait)
{
    if(fd < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    OVERLAPPED overlapvar;
    overlapvar.Offset = 0;
    overlapvar.OffsetHigh = 0;
    DWORD flag = 0;
    if(allow_share_read == false)
    {
        flag |= LOCKFILE_EXCLUSIVE_LOCK;
    }
    if(wait == false)
    {
        flag |= LOCKFILE_FAIL_IMMEDIATELY;
    }
    return LockFileEx((HANDLE)_get_osfhandle(fd), flag, 0, MAXDWORD, MAXDWORD, &overlapvar);
#else
    struct flock lock;
    memset(&lock, 0, sizeof(struct flock));
    lock.l_type = allow_share_read ? F_RDLCK : F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    int ret = fcntl(fd, wait ? F_SETLKW : F_SETLK, &lock);
    return (ret == 0);
#endif
}

bool com_file_is_locked(const char* file_path)
{
    int type = 0;
    int64 pid = 0;
    return com_file_is_locked(file_path, type, pid);
}

bool com_file_is_locked(const char* file_path, int& type, int64& pid)
{
    if(com_string_is_empty(file_path))
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return false;
#else
    FILE* fp = com_file_open(file_path, "r+");
    if(fp == NULL)
    {
        return false;
    }
    bool locked = com_file_is_locked(fp, type, pid);
    com_file_close(fp);
    return locked;
#endif
}

bool com_file_is_locked(FILE* file)
{
    return com_file_is_locked(com_file_get_fd(file));
}

bool com_file_is_locked(FILE* file, int& type, int64& pid)
{
    if(file == NULL)
    {
        return false;
    }
    return com_file_is_locked(com_file_get_fd(file), type, pid);
}

bool com_file_is_locked(int fd)
{
    int type = 0;
    int64 pid = 0;
    return com_file_is_locked(fd, type, pid);
}

bool com_file_is_locked(int fd, int& type, int64& pid)
{
    if(fd < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return false;
#else
    struct flock lock;
    memset(&lock, 0, sizeof(struct flock));
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = F_WRLCK;

    int ret = fcntl(fd, F_GETLK, &lock);
    if(ret != 0)
    {
        LOG_E("failed to get lock");
        return false;
    }
    
    if(lock.l_type == F_UNLCK)
    {
        return false;
    }

    type = lock.l_type;
    pid = (int64)lock.l_pid;

    return true;
#endif
}

bool com_file_unlock(FILE* file)
{
    if(file == NULL)
    {
        return false;
    }
    return com_file_unlock(com_file_get_fd(file));
}

bool com_file_unlock(int fd)
{
    if(fd < 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    OVERLAPPED overlapvar;
    overlapvar.Offset = 0;
    overlapvar.OffsetHigh = 0;
    return UnlockFileEx((HANDLE)_get_osfhandle(fd), 0, MAXDWORD, MAXDWORD, &overlapvar);
#else
    struct flock fl;
    memset(&fl, 0, sizeof(struct flock));
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;//意味着加锁一直加到EOF

    int ret = fcntl(fd, F_SETLK, &fl);
    return (ret == 0);
#endif
}

std::string com_file_path_absolute(const char* path)
{
    if(path == NULL || path[0] == '\0')
    {
        return std::string();
    }
#if defined(_WIN32) || defined(_WIN64)
    wchar_t path_full[_MAX_PATH];
    if(_wfullpath(path_full, com_wstring_from_utf8(path).c_str(), sizeof(path_full)) == NULL)
    {
        return std::string();
    }
    return com_wstring_to_utf8(path_full).toString();
#else
    char path_full[PATH_MAX];
    if(realpath(path, path_full) == NULL)
    {
        return std::string();
    }
    return path_full;
#endif
}

FilePath::FilePath(const std::string& path)
{
    parse(path.c_str());
}

FilePath::FilePath(const char* path)
{
    parse(path);
}

FilePath::~FilePath()
{
}

bool FilePath::parse(const char* path)
{
    if(path == NULL || path[0] == '\0')
    {
        return false;
    }

    this->path = path;
    if(this->path.length() > 1 && this->path.back() == PATH_DELIM_CHAR)
    {
        this->path.pop_back();
    }
    if(this->path.back() == ':'  /* C: */
            || this->path == PATH_DELIM_STR
            || this->path == "."
            || this->path == "..")
    {
        dir = path;
        name = path;
        return true;
    }

    //  ./1.txt   /1.txt  ./a/1.txt  /a/1.txt ./a/b/ /a/b/
    std::string::size_type pos = this->path.find_last_of(PATH_DELIM_CHAR);
    if(pos == std::string::npos)
    {
        dir = ".";
        name = this->path;
        pos = name.find_last_of('.');
        if(pos != std::string::npos)
        {
            suffix = name.substr(pos + 1);
            name_without_suffix = name.substr(0, pos);
        }
        return true;
    }

    name = this->path.substr(pos + 1);
    dir = this->path.substr(0, pos + 1);
    if(dir != "/") /*  /1.txt  */
    {
        dir.pop_back();//目录不带最后的分隔符，除非是单独的一个根目录
    }
    pos = name.find_last_of('.');
    if(pos != std::string::npos)
    {
        suffix = name.substr(pos + 1);
        name_without_suffix = name.substr(0, pos);
    }
    return true;
}

std::string FilePath::getName()
{
    return name;
}

std::string FilePath::getNameWithoutSuffix()
{
    return name_without_suffix;
}

std::string FilePath::getDir()
{
    return dir;
}

std::string FilePath::getPath()
{
    return path;
}

std::string FilePath::getSuffix()
{
    return suffix;
}

FileDetail::FileDetail(const char* path) : FilePath(path)
{
    if(path == NULL)
    {
        return;
    }
    parse(path);
    this->path = path;
#if defined(_WIN32) || defined(_WIN64)
    //_stat may has errno=132 error
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    if(GetFileAttributesExW(com_wstring_from_utf8(ComBytes(path)).c_str(), GetFileExInfoStandard, &attr) == false)
    {
        type = FILE_TYPE_NOT_EXIST;
        return;
    }

    if(attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        type = FILE_TYPE_DIR;
    }
    else
    {
        type = FILE_TYPE_FILE;
    }
    size = ((int64)attr.nFileSizeHigh << 32) + attr.nFileSizeLow;
    time_change_s = (((int64)attr.ftCreationTime.dwHighDateTime << 32) + attr.ftCreationTime.dwLowDateTime) / 100000000;
    time_access_s = (((int64)attr.ftLastAccessTime.dwHighDateTime << 32) + attr.ftLastAccessTime.dwLowDateTime) / 100000000;
    time_modify_s = (((int64)attr.ftLastWriteTime.dwHighDateTime << 32) + attr.ftLastWriteTime.dwLowDateTime) / 100000000;
#else
    struct stat buf;
    int ret = stat(this->path.c_str(), &buf);
    if(0 != ret)
    {
        if(errno == ENOENT)
        {
            type = FILE_TYPE_NOT_EXIST;
        }
        return;
    }
    if(buf.st_mode & S_IFDIR)
    {
        type = FILE_TYPE_DIR;
    }
    else if(buf.st_mode & S_IFREG)
    {
        type = FILE_TYPE_FILE;
    }
    else if(buf.st_mode & S_IFLNK)
    {
        type = FILE_TYPE_LINK;
    }
    else if(buf.st_mode & S_IFSOCK)
    {
        type = FILE_TYPE_SOCK;
    }
    size = buf.st_size;
    time_change_s = buf.st_ctime;
    time_access_s = buf.st_atime;
    time_modify_s = buf.st_mtime;
#endif
    return;
}

FileDetail::~FileDetail()
{
}

int FileDetail::getType()
{
    return type;
}

uint64 FileDetail::getSize()
{
    return size;
}

uint32 FileDetail::getChangeTimeS()
{
    return time_change_s;
}

uint32 FileDetail::getAccessTimeS()
{
    return time_access_s;
}

uint32 FileDetail::getModifyTimeS()
{
    return time_modify_s;
}

bool FileDetail::setAccessTime(uint32 timestamp_s)
{
    if(path.empty())
    {
        return false;
    }
    if(timestamp_s == 0)
    {
        timestamp_s = com_time_rtc_s();
    }
    time_access_s = timestamp_s;
    time_change_s = timestamp_s;
#if defined(_WIN32) || defined(_WIN64)
    struct _utimbuf buf;
    buf.actime = time_access_s;
    buf.modtime = time_modify_s;
    _wutime(com_wstring_from_utf8(ComBytes(path)).c_str(), &buf);
#else
    struct utimbuf buf;
    buf.actime = time_access_s;
    buf.modtime = time_modify_s;
    utime(path.c_str(), &buf);
#endif
    return true;
}

bool FileDetail::setModifyTime(uint32 timestamp_s)
{
    if(path.empty())
    {
        return false;
    }
    if(timestamp_s == 0)
    {
        timestamp_s = com_time_rtc_s();
    }
    time_access_s = timestamp_s;
    time_modify_s = timestamp_s;
    time_change_s = timestamp_s;
#if defined(_WIN32) || defined(_WIN64)
    struct _utimbuf buf;
    buf.actime = time_access_s;
    buf.modtime = time_modify_s;
    _wutime(com_wstring_from_utf8(ComBytes(path)).c_str(), &buf);
#else
    struct utimbuf buf;
    buf.actime = time_access_s;
    buf.modtime = time_modify_s;
    utime(path.c_str(), &buf);
#endif
    return true;
}

SingleInstanceProcess::SingleInstanceProcess(const char* file_lock)
{
    std::string file;
    if(file_lock == NULL)
    {
        file = com_string_format("%s%ssip_%s.lck", com_dir_system_temp().c_str(), PATH_DELIM_STR, com_get_bin_name().c_str());
    }
    else
    {
        file = file_lock;
    }
#if defined(_WIN32) || defined(_WIN64)
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &sd;

    fp = CreateMutexW(&sa, FALSE, com_wstring_from_utf8(ComBytes(file)).c_str());
    if(ERROR_ALREADY_EXISTS == GetLastError())
    {
        LOG_W("process already running");
        exit(0);
    }
#else
    fp = com_file_open(file.c_str(), "w+");
    if(fp == NULL)
    {
        LOG_E("file to create process lock file, errno=%d", errno);
    }
    else
    {
        int type = 0;
        int64 pid = 0;
        if(com_file_is_locked((FILE*)fp, type, pid))
        {
            com_file_close((FILE*)fp);
            LOG_W("process already running, pid=%llu, type=%d", pid, type);
            exit(0);
        }
        com_file_lock((FILE*)fp);
    }
#endif
}

SingleInstanceProcess::~SingleInstanceProcess()
{
#if defined(_WIN32) || defined(_WIN64)
    if(fp)
    {
        CloseHandle((HANDLE)fp);
    }
#else
    com_file_close((FILE*)fp);
#endif
}

