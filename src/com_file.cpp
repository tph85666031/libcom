#include <fcntl.h>
#include "com_base.h"
#include "com_file.h"
#include "com_log.h"

bool com_dir_create(const char* full_path)
{
    if(full_path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    const char* delim = "\\";
#else
    const char* delim = "/";
#endif
    std::vector<std::string> paths = com_string_split(full_path, delim);
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
            path += delim;
            continue;
        }
        if(com_file_type(path.c_str()) != FILE_TYPE_NOT_EXIST)
        {
            if(com_file_type(path.c_str()) != FILE_TYPE_DIR)
            {
                LOG_E("file %s already exist", path.c_str());
                return false;
            }
            path += delim;
            continue;
        }
#if defined(_WIN32) || defined(_WIN64)
        if(_mkdir(path.c_str()) != 0)
        {
            LOG_E("make dir %s failed", path.c_str());
            return false;
        }
#else
        if(mkdir(path.c_str(), 0775) != 0)
        {
            LOG_E("make dir %s failed, err=%s", path.c_str(), strerror(errno));
            return false;
        }
#endif
        path += delim;
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
    if(GetDiskFreeSpaceExA((LPCSTR)dir, NULL, (ULARGE_INTEGER*)&total_size_byte,
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
    if(GetDiskFreeSpaceExA((LPCSTR)dir, NULL, (ULARGE_INTEGER*)&total_size_byte,
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

int com_dir_remove(const char* dir_path)
{
#if defined(_WIN32) || defined(_WIN64)
    return _rmdir(dir_path);
#else
    if(dir_path == NULL)
    {
        return -1;
    }
    DIR* dir = opendir(dir_path);
    if(dir == NULL)
    {
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
        int ret = strcmp(ptr->d_name, ".");
        if(0 == ret)
        {
            continue;
        }
        ret = strcmp(ptr->d_name, "..");
        if(0 == ret)
        {
            continue;
        }
        std::string path = dir_path;
        path.append("/");
        path.append(ptr->d_name);

        if(com_file_type(path.c_str()) == FILE_TYPE_DIR)
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

int64 com_dir_size_freed(const char* dir)
{
    if(dir == NULL)
    {
        return -1;
    }
    int64 free_size_byte = 0;
#if defined(_WIN32) || defined(_WIN64)
    int64 total_size_byte = 0;
    if(GetDiskFreeSpaceExA((LPCSTR)dir, NULL, (ULARGE_INTEGER*)&total_size_byte,
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

std::string com_path_name(const char* path)
{
    FilePath file_path(path);
    return file_path.getName();
}

std::string com_path_dir(const char* path)
{
    FilePath file_path(path);
    return file_path.getLocationDirectory();
}

int com_file_type(const char* file)
{
    if(file == NULL)
    {
        return FILE_TYPE_NOT_EXIST;
    }
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
#if __linux__ == 1
    if(buf.st_mode & S_IFLNK)
    {
        return FILE_TYPE_LINK;
    }
#endif
    return FILE_TYPE_UNKNOWN;
}

int64 com_file_size(const char* file_path)
{
    if(file_path == NULL)
    {
        return -1;
    }
    unsigned long filesize = 0;
#if defined(_WIN32) || defined(_WIN64)
    struct _stat statbuff;
    if(_stat(file_path, &statbuff) == 0)
    {
        filesize = statbuff.st_size;
    }
#else
    struct stat statbuff;
    if(stat(file_path, &statbuff) == 0)
    {
        filesize = statbuff.st_size;
    }
#endif
    return filesize;
}

uint32 com_file_get_create_time(const char* file_path)
{
    if(file_path == NULL)
    {
        return 0;
    }
    uint32 timestamp = 0;
#if defined(_WIN32) || defined(_WIN64)
    struct _stat statbuff;
    if(_stat(file_path, &statbuff) == 0)
    {
        timestamp = statbuff.st_ctime;
    }
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
    struct _stat statbuff;
    if(_stat(file_path, &statbuff) == 0)
    {
        timestamp = statbuff.st_mtime;
    }
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
    struct _stat statbuff;
    if(_stat(file_path, &statbuff) == 0)
    {
        timestamp = statbuff.st_atime;
    }
#else
    struct stat statbuff;
    if(stat(file_path, &statbuff) == 0)
    {
        timestamp = statbuff.st_atime;
    }
#endif
    return timestamp;
}

void com_file_copy(const char* file_path_to, const char* file_path_from, bool append)
{
    if(file_path_to == NULL || file_path_from == NULL)
    {
        return;
    }
    FILE* fpR, *fpW;
    //char buffer[8];
    int buffer_size = 1 * 1024 * 1024;
    uint8* buffer = new uint8[buffer_size]();
    int lenR, lenW;
    if((fpR = fopen(file_path_from, "r")) == NULL)
    {
        delete[] buffer;
        return;
    }
    if((fpW = fopen(file_path_to, append ? "a" : "w")) == NULL)
    {
        delete[] buffer;
        com_file_close(fpR);
        return;
    }
    memset(buffer, 0, buffer_size);
    while((lenR = fread(buffer, 1, buffer_size, fpR)) > 0)
    {
        if((lenW = fwrite(buffer, 1, lenR, fpW)) != lenR)
        {
            com_file_flush(fpW);
            com_file_close(fpR);
            com_file_close(fpW);
            return;
        }
        memset(buffer, 0, buffer_size);
    }
    com_file_flush(fpW);
    com_file_close(fpR);
    com_file_close(fpW);
    delete[] buffer;
    return;
}

void com_file_clean(const char* file_path)
{
    FILE* file = com_file_open(file_path, "w");
    if(file)
    {
#if defined(_WIN32) || defined(_WIN64)
        if(_lseeki64(_fileno(file), 0, SEEK_SET) < 0)
        {
            com_file_close(file);
            return;
        }
        if(!SetEndOfFile((HANDLE)file))
        {
            com_file_close(file);
            return;
        }
#else
        ftruncate(fileno(file), 0);
        lseek(fileno(file), 0, SEEK_SET);
#endif
        com_file_close(file);
    }
    return;
}

int com_file_seek_head(FILE* file)
{
    if(file == NULL)
    {
        return -1;
    }
    return fseek(file, 0L, SEEK_SET);
}

int com_file_seek_tail(FILE* file)
{
    if(file == NULL)
    {
        return -1;
    }
    return fseek(file, 0L, SEEK_END);
}

int com_file_seek(FILE* file, int64 pos)
{
    if(file == NULL)
    {
        return -1;
    }
    if(pos > 0)
    {
        return fseek(file, pos, SEEK_SET);
    }
    else
    {
        return fseek(file, pos, SEEK_END);
    }
}

bool com_file_create(const char* file_path)
{
    FILE* file = com_file_open(file_path, "w+");
    com_file_close(file);
    return file != NULL;
}

bool com_file_rename(const char* file_name_new, const char* file_name_old)
{
    if(file_name_new == NULL || file_name_old == NULL)
    {
        return false;
    }
    return rename(file_name_old, file_name_new) == 0 ? true : false;
}

FILE* com_file_open(const char* file_path, const char* flag)
{
    if(file_path == NULL || flag == NULL)
    {
        return NULL;
    }
    return fopen(file_path, flag);
}

int com_file_read(FILE* file, void* buf, int size)
{
    if(file == NULL || buf == NULL || size <= 0)
    {
        return 0;
    }
    int size_readed = 0;
    do
    {
        size_readed += fread((uint8*)buf + size_readed, 1, size - size_readed, file);
    }
    while(feof(file) == 0 && ferror(file) == 0 && size_readed < size);
    return size_readed;
}

bool com_file_readline(FILE* file, char* buf, int size)
{
    if(file == NULL || buf == NULL || size <= 0)
    {
        return false;
    }
    if(fgets(buf, size, file) != NULL)
    {
        buf[size - 1] = '\0';
        return true;
    }
    else
    {
        return false;
    }
}

CPPBytes com_file_readall(const char* file_path)
{
    CPPBytes bytes;
    FILE* file = com_file_open(file_path, "r");
    if(file == NULL)
    {
        return bytes;
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

int com_file_write(FILE* file, const void* buf, int size)
{
    if(file == NULL || buf == NULL || size <= 0)
    {
        return 0;
    }

    int len = -1;
    int total_size = 0;
    while(total_size < size
            && (len = fwrite((uint8*)buf + total_size, 1, size - total_size, file)) > 0)
    {
        total_size += len;
    }
    if(len > 0)
    {
        return total_size;
    }
    LOG_W("total_size=%d,size=%d,ferror=%d", total_size, size, ferror(file));
    return -1;
}


int com_file_writef(FILE* fp, const char* fmt, ...)
{
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
        fsync(fileno(file));
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

void com_file_remove(const char* file_name)
{
    remove(file_name);
}

int com_file_get_fd(FILE* file)
{
    if(file == NULL)
    {
        return -1;
    }
    return fileno(file);
}

bool com_file_lock(FILE* file, bool read_lock, bool wait)
{
    if(file == NULL)
    {
        return false;
    }
    return com_file_lock(fileno(file), read_lock, wait);
}
bool com_file_lock(int fd, bool read_lock, bool wait)
{
    if(fd < 0)
    {
        return false;
    }
#if __linux__ == 1
    struct flock fl;
    memset(&fl, 0, sizeof(struct flock));
    fl.l_type = read_lock ? F_RDLCK : F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;//意味着加锁一直加到EOF

    int ret = fcntl(fd, wait ? F_SETLKW : F_SETLK, &fl);
    return (ret == 0);
#else
    return false;
#endif
}

bool com_file_is_locked(FILE* file, bool read_lock)
{
    if(file == NULL)
    {
        return false;
    }
    return com_file_is_locked(fileno(file), read_lock);
}
bool com_file_is_locked(int fd, bool read_lock)
{
    if(fd < 0)
    {
        return false;
    }
#if __linux__ == 1
    struct flock fl;
    memset(&fl, 0, sizeof(struct flock));
    fl.l_type = read_lock ? F_RDLCK : F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;//意味着加锁一直加到EOF

    int ret = fcntl(fd, F_GETLK, &fl);
    if(ret != 0)
    {
        return false;
    }
    if(fl.l_type == F_UNLCK)
    {
        return false;
    }

    return (fl.l_pid != 0);
#else
    return false;
#endif
}

bool com_file_unlock(FILE* file)
{
    if(file == NULL)
    {
        return false;
    }
    return com_file_unlock(fileno(file));
}

bool com_file_unlock(int fd)
{
    if(fd < 0)
    {
        return false;
    }
#if __linux__ == 1
    struct flock fl;
    memset(&fl, 0, sizeof(struct flock));
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;//意味着加锁一直加到EOF

    int ret = fcntl(fd, F_SETLK, &fl);
    return (ret == 0);
#else
    return false;
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

    std::string path_str = path;
    if(path_str == PATH_DELIM_STR)
    {
        is_dir = true;
        dir = PATH_DELIM_STR;
        name = dir;
        return false;
    }

    if(path_str == ".")
    {
        is_dir = true;
        dir = ".";
        name = dir;
        return false;
    }

    if(path_str == "..")
    {
        is_dir = true;
        dir = "..";
        name = dir;
        return false;
    }

    if(path_str.back() == PATH_DELIM_CHAR)
    {
        is_dir = true;
        path_str.erase(path_str.length() - 1, 1);
    }
    //  ./1.txt   /1.txt  ./a/1.txt  /a/1.txt ./a/b/ /a/b/
    std::string::size_type pos = path_str.find_last_of(PATH_DELIM_CHAR);
    if(pos == std::string::npos)
    {
        dir = ".";
        name = path_str;
        return false;
    }

    name = path_str.substr(pos + 1);
    dir = path_str.substr(0, pos + 1);
    return true;
}

std::string FilePath::getName()
{
    return name;
}

std::string FilePath::getLocationDirectory()
{
    return dir;
}

bool FilePath::isDirectory()
{
    return is_dir;
}

