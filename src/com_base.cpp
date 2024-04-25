﻿#include <cctype>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#include <time.h>
#include <direct.h>
#else
#include <unistd.h> //usleep readlink
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <netdb.h> //gethostbyname
#include <pwd.h>
#include <locale>
#if __GNUC__>4
#include <codecvt>
#endif
#endif

#if defined(__APPLE__)
#include <libproc.h>
#include <codecvt>
#endif

#include "ConvertUTF.h"
#include "CJsonObject.h"
#include "com_base.h"
#include "com_md5.h"
#include "com_file.h"
#include "com_thread.h"
#include "com_log.h"
#include "com_serializer.h"

static std::mutex mutex_app_path;

#define XFNM_PATHNAME    (1 << 0)        /* No wildcard can ever match `/'.  */
#define XFNM_NOESCAPE    (1 << 1)        /* Backslashes don't quote special chars.  */
#define XFNM_PERIOD      (1 << 2)        /* Leading `.' is matched only explicitly.  */
#define XFNM_FILE_NAME   XFNM_PATHNAME   /* Preferred GNU name.  */
#define XFNM_LEADING_DIR (1 << 3)        /* Ignore `/...' after a match.  */
#define XFNM_CASEFOLD    (1 << 4)        /* Compare without regard to case.  */
#define XFNM_EXTMATCH    (1 << 5)        /* Use ksh-like extended matching. */
#define XFNM_NOMATCH     (1)

#define XFOLD(c) ((flags & XFNM_CASEFOLD) ? std::tolower(c) : (c))
#define XEOS '\0'

static const char* xfnmatch_rangematch(const char* pattern, int test, int flags)
{
    char c = 0;
    char c2 = 0;
    int negate = 0;
    int ok = 0;

    negate = (*pattern == '!' || *pattern == '^');
    if(negate)
    {
        pattern++;
    }

    for(ok = 0; (c = *pattern++) != ']';)
    {
        if(c == '\\' && !(flags & XFNM_NOESCAPE))
        {
            c = *pattern++;
        }
        if(c == XEOS)
        {
            return NULL;
        }

        if(*pattern == '-' && (c2 = *(pattern + 1)) != XEOS && c2 != ']')
        {
            pattern += 2;
            if(c2 == '\\' && !(flags & XFNM_NOESCAPE))
            {
                c2 = *pattern++;
            }
            if(c2 == XEOS)
            {
                return NULL;
            }
            if(c <= test && test <= c2)
            {
                ok = 1;
            }
        }
        else if(c == test)
        {
            ok = 1;
        }
    }

    return ok == negate ? NULL : pattern;
}

int xfnmatch(const char* pattern, const char* string, int flags)
{
    if(pattern == NULL || pattern[0] == '\0')
    {
        return XFNM_NOMATCH;
    }
    if(string == NULL || string[0] == '\0')
    {
        return XFNM_NOMATCH;
    }
    const char* stringstart = string;
    char c = 0;
    char test = 0;

    for(;;)
    {
        switch(c = *pattern++)
        {
            case XEOS:
                return *string == XEOS ? 0 : XFNM_NOMATCH;

            case '?':
                if(*string == XEOS)
                {
                    return XFNM_NOMATCH;
                }
                if(*string == PATH_DELIM_CHAR && (flags & XFNM_PATHNAME))
                {
                    return XFNM_NOMATCH;
                }

                if(*string == '.' &&
                        (flags & XFNM_PERIOD) &&
                        (string == stringstart || ((flags & XFNM_PATHNAME) && *(string - 1) == PATH_DELIM_CHAR)))
                {
                    return XFNM_NOMATCH;
                }

                string++;
                break;

            case '*':
                c = *pattern;
                // Collapse multiple stars
                while(c == '*')
                {
                    c = *++pattern;
                }

                if(*string == '.' &&
                        (flags & XFNM_PERIOD) &&
                        (string == stringstart || ((flags & XFNM_PATHNAME) && *(string - 1) == PATH_DELIM_CHAR)))
                {
                    return XFNM_NOMATCH;
                }

                // Optimize for pattern with * at end or before
                if(c == XEOS)
                {
                    if(flags & XFNM_PATHNAME)
                    {
                        return strchr(string, PATH_DELIM_CHAR) == NULL ? 0 : XFNM_NOMATCH;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else if(c == PATH_DELIM_CHAR && flags & XFNM_PATHNAME)
                {
                    if((string = strchr(string, PATH_DELIM_CHAR)) == NULL)
                    {
                        return XFNM_NOMATCH;
                    }
                    break;
                }

                // General case, use recursion
                while((test = *string) != XEOS)
                {
                    if(!xfnmatch(pattern, string, flags & ~XFNM_PERIOD))
                    {
                        return 0;
                    }
                    if(test == PATH_DELIM_CHAR && flags & XFNM_PATHNAME)
                    {
                        break;
                    }
                    string++;
                }
                return XFNM_NOMATCH;

            case '[':
                if(*string == XEOS)
                {
                    return XFNM_NOMATCH;
                }
                if(*string == PATH_DELIM_CHAR && flags & XFNM_PATHNAME)
                {
                    return XFNM_NOMATCH;
                }
                if((pattern = xfnmatch_rangematch(pattern, *string, flags)) == NULL)
                {
                    return XFNM_NOMATCH;
                }
                string++;
                break;

            case '\\':
                if(!(flags & XFNM_NOESCAPE))
                {
                    if((c = *pattern++) == XEOS)
                    {
                        c = '\\';
                        pattern--;
                    }
                }
            // FALLTHROUGH

            default:
                if(c != *string++)
                {
                    return XFNM_NOMATCH;
                }
                break;
        }
    }
}

static void shell_cmd_thread(std::string cmd)
{
    std::string result = com_run_shell_with_output("%s", cmd.c_str());
    LOG_D("cmd=%s,result=%s", cmd.c_str(), result.c_str());
}

int com_run_shell(const char* fmt, ...)
{
    if(fmt == NULL)
    {
        return -1;
    }
    std::string cmd;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len <= 0)
    {
        return -1;
    }
    len += 1;  //上面返回的长度不包含\0，这里加上
    std::vector<char> cmd_buf(len);
    va_start(args, fmt);
    len = vsnprintf(cmd_buf.data(), len, fmt, args);
    va_end(args);
    if(len <= 0)
    {
        return -1;
    }
    cmd.assign(cmd_buf.data(), len);

#if defined(_WIN32) || defined(_WIN64)
    FILE* fp = _popen(cmd.c_str(), "r");
#else
    FILE* fp = popen(cmd.c_str(), "r");
#endif
    if(fp == NULL)
    {
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    _pclose(fp);
    return 0;
#else
    int ret = pclose(fp);
    if(WIFEXITED(ret))
    {
        return WEXITSTATUS(ret);
    }
    else if(WIFSIGNALED(ret))
    {
        return WTERMSIG(ret);
    }
    else if(WCOREDUMP(ret))
    {
        return -2;
    }
    return ret;
#endif
}

std::string com_run_shell_with_output(const char* fmt, ...)
{
    std::string result;
    if(fmt == NULL)
    {
        return result;
    }
    std::string cmd;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len <= 0)
    {
        return result;
    }
    len += 1;  //上面返回的长度不包含\0，这里加上
    std::vector<char> cmd_buf(len);
    va_start(args, fmt);
    len = vsnprintf(cmd_buf.data(), len, fmt, args);
    va_end(args);
    if(len <= 0)
    {
        return result;
    }
    cmd.assign(cmd_buf.data(), len);

#if defined(_WIN32) || defined(_WIN64)
    FILE* fp = _popen(cmd.c_str(), "r");
#else
    FILE* fp = popen(cmd.c_str(), "r");
#endif
    if(fp == NULL)
    {
        return result;
    }
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        result.append(buf);
        memset(buf, 0, sizeof(buf));
    }
#if defined(_WIN32) || defined(_WIN64)
    _pclose(fp);
#else
    pclose(fp);
#endif
    return result;
}

bool com_run_shell_in_thread(const char* fmt, ...)
{
    std::string cmd;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len <= 0)
    {
        return false;
    }
    len += 1;  //上面返回的长度不包含\0，这里加上
    std::vector<char> cmd_buf(len);
    va_start(args, fmt);
    len = vsnprintf(cmd_buf.data(), len, fmt, args);
    va_end(args);
    if(len <= 0)
    {
        return false;
    }
    cmd.assign(cmd_buf.data(), len);
    std::thread t(shell_cmd_thread, cmd);
    t.detach();
    return true;
}

std::vector<std::string> com_string_split(const char* str, const char* delim)
{
    std::vector<std::string> vals;
    if(str != NULL && delim != NULL)
    {
        std::string orgin = str;
        int delim_len = (int)strlen(delim);
        std::string::size_type pos = 0;
        std::string::size_type pos_pre = 0;
        while(true)
        {
            pos = orgin.find_first_of(delim, pos_pre);
            if(pos == std::string::npos)
            {
                std::string val = orgin.substr(pos_pre);
                vals.push_back(val);
                break;
            }
            std::string val = orgin.substr(pos_pre, pos - pos_pre);
            vals.push_back(val);
            pos_pre = pos + delim_len;
        }
    }
    return vals;
}

char* com_strncpy(char* dst, const char* src, int dst_size)
{
    if(dst == NULL || src == NULL || dst_size <= 0)
    {
        return NULL;
    }
    strncpy(dst, src, dst_size);
    dst[dst_size - 1] = '\0';
    return dst;
}

bool com_strncmp_ignore_case(const char* str1, const char* str2, int size)
{
    if(str1 == NULL || str2 == NULL || size <= 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(_strnicmp(str1, str2, size) == 0)
#else
    if(strncasecmp(str1, str2, size) == 0)
#endif
    {
        return true;
    }
    return false;
}

bool com_strncmp(const char* str1, const char* str2, int size)
{
    if(str1 == NULL || str2 == NULL || size <= 0)
    {
        return false;
    }
    if(strncmp(str1, str2, size) == 0)
    {
        return true;
    }
    return false;
}

bool com_string_equal(const char* a, const char* b)
{
    if(a == NULL || b == NULL)
    {
        return false;
    }
    if(strcmp(a, b) == 0)
    {
        return true;
    }
    return false;
}

bool com_string_equal_ignore_case(const char* a, const char* b)
{
    if(a == NULL || b == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(_stricmp(a, b) == 0)
#else
    if(strcasecmp(a, b) == 0)
#endif
    {
        return true;
    }
    return false;
}

std::string& com_string_trim_left(std::string& str, const char* t)
{
    if(t == NULL)
    {
        t = " \t\n\r\f\v";
    }
    str.erase(0, str.find_first_not_of(t));
    return str;
}

std::string& com_string_trim_right(std::string& str, const char* t)
{
    if(t == NULL)
    {
        t = " \t\n\r\f\v";
    }
    str.erase(str.find_last_not_of(t) + 1);
    return str;
}

std::string& com_string_trim(std::string& str, const char* t)
{
    return com_string_trim_left(com_string_trim_right(str, t), t);
}

char* com_string_trim_left(char* str, const char* t)
{
    if(com_string_is_empty(str) || com_string_is_empty(t))
    {
        return str;
    }
    char* p1 = str;
    char* p2 = str;
    int len_t = com_string_len(t);
    do
    {
        bool match = false;
        for(int i = 0; i < len_t; i++)
        {
            if(*p1 == t[i])
            {
                match = true;
                break;
            }
        }

        if(match == false)
        {
            break;
        }
        p1++;
    }
    while(*p1);

    while(*p1 != '\0')
    {
        *p2 = *p1;
        p1++;
        p2++;
    }
    *p2 = '\0';
    return str;
}

char* com_string_trim_right(char* str, const char* t)
{
    if(com_string_is_empty(str) || com_string_is_empty(t))
    {
        return str;
    }
    int len_t = com_string_len(t);
    int i = 0;
    for(i = com_string_len(str) - 1; i >= 0; i--)
    {
        bool match = false;
        for(int j = 0; j < len_t; j++)
        {
            if(str[i] == t[j])
            {
                match = true;
                break;
            }
        }

        if(match == false)
        {
            str[i + 1] = '\0';
            break;
        }
    }
    return str;
}

char* com_string_trim(char* str)
{
    com_string_trim_right(str);
    com_string_trim_left(str);
    return str;
}

bool com_string_start_with(const char* str, const char* prefix)
{
    if(str == NULL || prefix == NULL)
    {
        return false;
    }
    if(strncmp(str, prefix, strlen(prefix)) == 0)
    {
        return true;
    }
    return false;
}

bool com_string_end_with(const char* str, const char* tail)
{
    if(str == NULL || tail == NULL)
    {
        return false;
    }
    int l1 = com_string_len(str);
    int l2 = com_string_len(tail);
    if(l1 >= l2)
    {
        if(com_string_equal(str + l1 - l2, tail))
        {
            return true;
        }
    }
    return false;
}

bool com_string_contain(const char* str, const char* sub)
{
    if(str == NULL || sub == NULL)
    {
        return false;
    }
    if(strstr(str, sub))
    {
        return true;
    }
    return false;
}

std::string& com_string_to_upper(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string& com_string_to_lower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

char* com_string_to_upper(char* str)
{
    if(str == NULL)
    {
        return str;
    }
    char* orignal = str;
    for(; *str != '\0'; str++)
    {
        *str = toupper(*str);
    }
    return orignal;
}

char* com_string_to_lower(char* str)
{
    if(str == NULL)
    {
        return str;
    }
    char* orignal = str;
    for(; *str != '\0'; str++)
    {
        *str = tolower(*str);
    }
    return orignal;
}

std::string com_string_format(const char* fmt, ...)
{
    if(NULL == fmt)
    {
        return "";
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
    return str;
}

std::wstring com_wstring_format(const wchar_t* fmt, ...)
{
    if(NULL == fmt)
    {
        return L"";
    }
    std::wstring str;
    va_list args;
    va_start(args, fmt);
    int len = vswprintf(NULL, 0, fmt, args);
    va_end(args);
    if(len > 0)
    {
        len += 1;  //上面返回的长度不包含\0，这里加上
        std::vector<wchar_t> buf(len);
        va_start(args, fmt);
        len = vswprintf(buf.data(), len, fmt, args);
        va_end(args);
        if(len > 0)
        {
            str.assign(buf.data(), len);
        }
    }
    return str;
}

bool com_string_match(const char* str, const char* pattern, bool is_path)
{
    return (xfnmatch(pattern, str, is_path ? XFNM_PATHNAME | XFNM_NOESCAPE : XFNM_NOESCAPE) == 0);
}

CPPBytes com_string_utf8_to_utf16(const CPPBytes& utf8)
{
    if(utf8.empty())
    {
        return CPPBytes();
    }
    const UTF8* p_utf8 = utf8.getData();
    UTF16* p_utf16 = new UTF16[utf8.getDataSize()];
    UTF16* p_utf16_tmp = p_utf16;
    ConversionResult ret = ConvertUTF8toUTF16(&p_utf8, p_utf8 + utf8.getDataSize(),
                           &p_utf16_tmp, p_utf16_tmp + utf8.getDataSize(), lenientConversion);
    if(ret != conversionOK && ret != sourceExhausted)
    {
        delete[] p_utf16;
        return CPPBytes();
    }

    if(p_utf16_tmp <= p_utf16)
    {
        delete[] p_utf16;
        return CPPBytes();
    }
    CPPBytes result;
    result.append((uint8*)p_utf16, (int)(p_utf16_tmp - p_utf16) * 2);
    delete[] p_utf16;
    return result;
}

CPPBytes com_string_utf16_to_utf8(const CPPBytes& utf16)
{
    if(utf16.empty())
    {
        return CPPBytes();
    }
    const UTF16* p_utf16 = (const UTF16*)utf16.getData();
    UTF8* p_utf8 = new UTF8[utf16.getDataSize() * 2];
    UTF8* p_utf8_tmp = p_utf8;
    ConversionResult ret = ConvertUTF16toUTF8(&p_utf16, p_utf16 + utf16.getDataSize() / 2,
                           &p_utf8_tmp, p_utf8_tmp + utf16.getDataSize() * 2, lenientConversion);
    if(ret != conversionOK && ret != sourceExhausted)
    {
        delete[] p_utf8;
        return CPPBytes();
    }

    if(p_utf8_tmp <= p_utf8)
    {
        delete[] p_utf8;
        return CPPBytes();
    }
    CPPBytes result;
    result.append(p_utf8, (int)(p_utf8_tmp - p_utf8));
    delete[] p_utf8;
    return result;
}

CPPBytes com_string_utf8_to_utf32(const CPPBytes& utf8)
{
    if(utf8.empty())
    {
        return CPPBytes();
    }
    const UTF8* p_utf8 = utf8.getData();
    UTF32* p_utf32 = new UTF32[utf8.getDataSize()];
    UTF32* p_utf32_tmp = p_utf32;
    ConversionResult ret = ConvertUTF8toUTF32(&p_utf8, p_utf8 + utf8.getDataSize(),
                           &p_utf32_tmp, p_utf32 + utf8.getDataSize(), lenientConversion);
    if(ret != conversionOK && ret != sourceExhausted)
    {
        delete[] p_utf32;
        return CPPBytes();
    }

    if(p_utf32_tmp <= p_utf32)
    {
        delete[] p_utf32;
        return CPPBytes();
    }
    CPPBytes result;
    result.append((uint8*)p_utf32, (int)(p_utf32_tmp - p_utf32) * 4);
    delete[] p_utf32;
    return result;
}

CPPBytes com_string_utf32_to_utf8(const CPPBytes& utf32)
{
    if(utf32.empty())
    {
        return CPPBytes();
    }
    const UTF32* p_utf32 = (const UTF32*)utf32.getData();
    UTF8* p_utf8 = new UTF8[utf32.getDataSize()];
    UTF8* p_utf8_tmp = p_utf8;
    ConversionResult ret = ConvertUTF32toUTF8(&p_utf32, p_utf32 + utf32.getDataSize() / 4,
                           &p_utf8_tmp, p_utf8_tmp + utf32.getDataSize(), lenientConversion);
    if(ret != conversionOK && ret != sourceExhausted)
    {
        delete[] p_utf8;
        return CPPBytes();
    }

    if(p_utf8_tmp <= p_utf8)
    {
        delete[] p_utf8;
        return CPPBytes();
    }
    CPPBytes result;
    result.append(p_utf8, (int)(p_utf8_tmp - p_utf8));
    delete[] p_utf8;
    return result;
}

CPPBytes com_string_utf16_to_utf32(const CPPBytes& utf16)
{
    if(utf16.empty())
    {
        return CPPBytes();
    }
    const UTF16* p_utf16 = (const UTF16*)utf16.getData();
    UTF32* p_utf32 = new UTF32[utf16.getDataSize()];
    UTF32* p_utf32_tmp = p_utf32;
    ConversionResult ret = ConvertUTF16toUTF32(&p_utf16, p_utf16 + utf16.getDataSize() / 2,
                           &p_utf32_tmp, p_utf32_tmp + utf16.getDataSize(), lenientConversion);
    if(ret != conversionOK && ret != sourceExhausted)
    {
        delete[] p_utf32;
        return CPPBytes();
    }

    if(p_utf32_tmp <= p_utf32)
    {
        delete[] p_utf32;
        return CPPBytes();
    }
    CPPBytes result;
    result.append((uint8*)p_utf32, (int)(p_utf32_tmp - p_utf32) * 4);
    delete[] p_utf32;
    return result;
}

CPPBytes com_string_utf32_to_utf16(const CPPBytes& utf32)
{
    if(utf32.empty())
    {
        return CPPBytes();
    }
    const UTF32* p_utf32 = (const UTF32*)utf32.getData();
    UTF16* p_utf16 = new UTF16[utf32.getDataSize()];
    UTF16* p_utf16_tmp = p_utf16;
    ConversionResult ret = ConvertUTF32toUTF16(&p_utf32, p_utf32 + utf32.getDataSize() / 4,
                           &p_utf16_tmp, p_utf16_tmp + utf32.getDataSize(), lenientConversion);
    if(ret != conversionOK && ret != sourceExhausted)
    {
        delete[] p_utf16;
        return CPPBytes();
    }

    if(p_utf16_tmp <= p_utf16)
    {
        delete[] p_utf16;
        return CPPBytes();
    }
    CPPBytes result;
    result.append((uint8*)p_utf16, (int)(p_utf16_tmp - p_utf16) * 2);
    delete[] p_utf16;
    return result;
}

std::string com_string_ansi_to_utf8(const std::string& ansi)
{
    return com_string_ansi_to_utf8(ansi.c_str());
}

std::string com_string_ansi_to_utf8(const char* ansi)
{
    if(ansi == NULL)
    {
        return std::string();
    }
#if defined(_WIN32) || defined(_WIN64)
    std::wstring str_utf16;
    int size = MultiByteToWideChar(CP_ACP, 0, ansi, -1, 0, 0);
    if(size)
    {
        str_utf16.resize(size);
        MultiByteToWideChar(CP_ACP, 0, ansi, -1, &str_utf16[0], str_utf16.size());
    }

    return com_wstring_to_utf8(str_utf16).toString();
#else
    return ansi;
#endif
}

std::string com_string_utf8_to_ansi(const std::string& utf8)
{
    return com_string_utf8_to_ansi(utf8.c_str());
}

std::string com_string_utf8_to_ansi(const char* utf8)
{
    if(utf8 == NULL)
    {
        return std::string();
    }
#if defined(_WIN32) || defined(_WIN64)
    std::wstring wstr = com_wstring_from_utf8(CPPBytes(utf8));
    std::string str_ansi;
    int size = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    if(size > 0)
    {
        str_ansi.resize(size);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str_ansi[0], str_ansi.size(), NULL, NULL);
    }
    return str_ansi;
#else
    return utf8;
#endif
}

std::string com_string_utf8_to_local(const std::string& utf8)
{
    return com_string_utf8_to_local(utf8.c_str());
}

std::string com_string_utf8_to_local(const char* utf8)
{
    if(utf8 == NULL)
    {
        return std::string();
    }
#if defined(_WIN32) || defined(_WIN64)
    return com_string_utf8_to_ansi(utf8);
#else
    return utf8;
#endif
}

std::string com_string_local_to_utf8(const std::string& s)
{
    return com_string_local_to_utf8(s.c_str());
}

std::string com_string_local_to_utf8(const char* s)
{
    if(s == NULL)
    {
        return std::string();
    }
#if defined(_WIN32) || defined(_WIN64)
    return com_string_ansi_to_utf8(s);
#else
    return s;
#endif
}

std::wstring com_wstring_from_utf8(const CPPBytes& utf8)
{
    std::wstring wstr;
    if(sizeof(wchar_t) == 2)
    {
        CPPBytes utf16 = com_string_utf8_to_utf16(utf8);
        wstr.append((wchar_t*)utf16.getData(), utf16.getDataSize() / 2);
    }
    else
    {
        CPPBytes utf32 = com_string_utf8_to_utf32(utf8);
        wstr.append((wchar_t*)utf32.getData(), utf32.getDataSize() / 4);
    }

    return wstr;
}

std::wstring com_wstring_from_utf16(const CPPBytes& utf16)
{
    std::wstring wstr;
    if(sizeof(wchar_t) == 2)
    {
        wstr.append((wchar_t*)utf16.getData(), utf16.getDataSize() / 2);
    }
    else
    {
        CPPBytes utf32 = com_string_utf16_to_utf32(utf16);
        wstr.append((wchar_t*)utf32.getData(), utf32.getDataSize() / 4);
    }

    return wstr;
}

std::wstring com_wstring_from_utf32(const CPPBytes& utf32)
{
    std::wstring wstr;
    if(sizeof(wchar_t) == 2)
    {
        CPPBytes utf16 = com_string_utf32_to_utf16(utf32);
        wstr.append((wchar_t*)utf16.getData(), utf16.getDataSize() / 2);
    }
    else
    {
        wstr.append((wchar_t*)utf32.getData(), utf32.getDataSize() / 4);
    }

    return wstr;
}

CPPBytes com_wstring_to_utf8(const wchar_t* wstr)
{
    if(wstr == NULL)
    {
        return CPPBytes();
    }
    CPPBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr, wcslen(wstr) * 2);
        result = com_string_utf16_to_utf8(result);
    }
    else
    {
        result.append((uint8*)wstr, wcslen(wstr) * 4);
        result = com_string_utf32_to_utf8(result);
    }

    return result;
}

CPPBytes com_wstring_to_utf8(const std::wstring& wstr)
{
    CPPBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr.data(), wstr.length() * 2);
        result = com_string_utf16_to_utf8(result);
    }
    else
    {
        result.append((uint8*)wstr.data(), wstr.length() * 4);
        result = com_string_utf32_to_utf8(result);
    }

    return result;
}

CPPBytes com_wstring_to_utf16(const wchar_t* wstr)
{
    if(wstr == NULL)
    {
        return CPPBytes();
    }
    CPPBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr, wcslen(wstr) * 2);
    }
    else
    {
        result.append((uint8*)wstr, wcslen(wstr) * 4);
        result = com_string_utf32_to_utf16(result);
    }

    return result;
}

CPPBytes com_wstring_to_utf16(const std::wstring& wstr)
{
    CPPBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr.data(), wstr.length() * 2);
    }
    else
    {
        result.append((uint8*)wstr.data(), wstr.length() * 4);
        result = com_string_utf32_to_utf16(result);
    }

    return result;
}

CPPBytes com_wstring_to_utf32(const wchar_t* wstr)
{
    if(wstr == NULL)
    {
        return CPPBytes();
    }
    CPPBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr, wcslen(wstr) * 2);
        result = com_string_utf16_to_utf32(result);
    }
    else
    {
        result.append((uint8*)wstr, wcslen(wstr) * 4);
    }

    return result;
}

CPPBytes com_wstring_to_utf32(const std::wstring& wstr)
{
    CPPBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr.data(), wstr.length() * 2);
        result = com_string_utf16_to_utf32(result);
    }
    else
    {
        result.append((uint8*)wstr.data(), wstr.length() * 4);
    }

    return result;
}

std::string com_bytes_to_hexstring(const uint8* data, uint16 size)
{
    char buf[8];
    std::string result;
    if(data != NULL && size > 0)
    {
        for(int i = 0; i < size; i++)
        {
            snprintf(buf, sizeof(buf), "%02x", data[i]);
            result.append(buf, 2);
        }
    }
    return result;
}

CPPBytes com_hexstring_to_bytes(const char* str)
{
    CPPBytes bytes;
    if(str == NULL)
    {
        return bytes;
    }
    std::string val = str;
    com_string_replace(val, " ", "");

    char tmp[4];
    memset(tmp, 0, sizeof(tmp));
    for(size_t i = 0; i < val.length(); i = i + 2)
    {
        memcpy(tmp, val.data() + i, 2);
        bytes.append(strtoul(tmp, NULL, 16));
    }

    return bytes;
}

int com_hexstring_to_bytes(const char* str, unsigned char* bytes, int size)
{
    char* strEnd;
    int m = 0;
    int len = strlen(str) / 2;
    int i = 0;
    char tmp[4] = {0};
    if(size < len)
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        m = i * 2;
        memcpy(tmp, str + m, 2);
        bytes[i] = strtol(tmp, &strEnd, 16);
    }
    return i;
}

bool com_string_replace(char* str, char from, char to)
{
    int len = com_string_len(str);
    if(len <= 0)
    {
        return false;
    }
    for(int i = 0; i < len; i++)
    {
        if(str[i] == from)
        {
            str[i] = to;
        }
    }
    return true;
}

bool com_string_replace(std::string& str, const char* from, const char* to)
{
    if(from == NULL || to == NULL)
    {
        return false;
    }

    std::string::size_type pos = 0;
    int from_len = com_string_len(from);
    int to_len = com_string_len(to);
    while((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from_len, to);
        pos += to_len;
    }
    return true;
}

int com_string_len_utf8(const char* str)
{
    int i = 0, j = 0;
    while(str[i])
    {
        if((str[i] & 0xc0) != 0x80)
        {
            j++;
        }
        i++;
    }
    return j;
}

int com_string_len(const char* str)
{
    if(str == NULL)
    {
        return 0;
    }
    return strlen(str);
}

int com_string_size(const char* str)
{
    if(str == NULL)
    {
        return 0;
    }
    return com_string_len(str) + 1;
}

bool com_string_is_empty(const char* str)
{
    return (str == NULL || str[0] == '\0');
}

bool com_string_is_ip(const char* ip)
{
    if(ip == NULL)
    {
        return false;
    }
    int ip_val[4];
    if(sscanf(ip, "%d.%d.%d.%d", ip_val, ip_val + 1, ip_val + 2, ip_val + 3) != 4)
    {
        return false;
    }

    for(int i = 0; i < 4; i++)
    {
        if(ip_val[i] < 0 || ip_val[i] > 255)
        {
            return false;
        }
    }
    return true;
}

bool com_string_is_utf8(const std::string& str)
{
    if(str.empty())
    {
        return false;
    }
    return isLegalUTF8Sequence((const UTF8*)str.c_str(), (const UTF8*)str.c_str() + str.length());
}

bool com_string_is_utf8(const char* str)
{
    if(str == NULL || str[0] == '\0')
    {
        return false;
    }

    return isLegalUTF8Sequence((const UTF8*)str, (const UTF8*)str + strlen(str));
}


//返回值为已写入buf的长度，不包括末尾的\0
int com_snprintf(char* buf, int buf_size, const char* fmt, ...)
{
    if(buf == NULL || buf_size <= 0 || fmt == NULL)
    {
        return 0;
    }
    va_list list;
    va_start(list, fmt);
    int ret = vsnprintf(buf, buf_size, fmt, list);
    va_end(list);
    if(ret < 0)
    {
        ret = 0;
    }
    else if(ret >= buf_size)
    {
        ret = buf_size - 1;
    }
    buf[buf_size - 1] = '\0';
    return ret;
}

void com_sleep_ms(uint32 val)
{
    if(val == 0)
    {
        return;
    }
#if defined(_WIN32) || defined(_WIN64)
    Sleep(val);
#else
#if 1
    usleep((uint64)val * 1000);
#else
    struct timeval time;
    time.tv_sec = val / 1000;//seconds
    time.tv_usec = val % 1000 * 1000;//microsecond
    select(0, NULL, NULL, NULL, &time);
#endif
#endif
}

void com_sleep_s(uint32 val)
{
    com_sleep_ms((uint64)val * 1000);
}

uint32 com_rand(uint32 min, uint32 max)
{
    static uint32 seed = 0;
    srand(com_time_cpu_ms() + (seed++));
    return (uint32)(rand() % ((uint64)max - min + 1) + min);
}

bool com_gettimeofday(struct timeval* tp)
{
    if(tp == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    time_t clock;
    struct tm tm = {0};
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year   = wtm.wYear - 1900;
    tm.tm_mon   = wtm.wMonth - 1;
    tm.tm_mday   = wtm.wDay;
    tm.tm_hour   = wtm.wHour;
    tm.tm_min   = wtm.wMinute;
    tm.tm_sec   = wtm.wSecond;
    tm.tm_isdst  = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return true;
#else
    return gettimeofday(tp, NULL) == 0;
#endif
}

uint64 com_time_cpu_s()
{
#if defined(_WIN32) || defined(_WIN64)
    return GetTickCount() / 1000;
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return tss.tv_sec;
#endif
}

uint64 com_time_cpu_ms()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetTickCount();
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return (tss.tv_sec * 1000ULL + tss.tv_nsec / 1000000);
#endif
}

uint64 com_time_cpu_us()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetTickCount() * 1000;
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return (tss.tv_sec * 1000 * 1000ULL + tss.tv_nsec / 1000);
#endif
}

uint64 com_time_cpu_ns()
{
#if defined(_WIN32) || defined(_WIN64)
    return (uint64)GetTickCount() * 1000 * 1000;
#else
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tss);
    return (tss.tv_sec * 1000 * 1000 * 1000ULL + tss.tv_nsec);
#endif
}

//UTC seconds form 1970-1-1 0-0-0
uint64 com_time_rtc_s()
{
    struct timeval tv;
    com_gettimeofday(&tv);
    return tv.tv_sec;
}

//UTC milliseconds form 1970-1-1 0-0-0
uint64 com_time_rtc_ms()
{
    struct timeval tv;
    com_gettimeofday(&tv);
    return (tv.tv_sec * 1000ULL + tv.tv_usec / 1000);
}

//UTC macroseconds form 1970-1-1 0-0-0
uint64 com_time_rtc_us()
{
    struct timeval tv;
    com_gettimeofday(&tv);
    return (tv.tv_sec * 1000 * 1000ULL + tv.tv_usec);
}

uint64 com_time_diff_ms(uint64 start, uint64 end)
{
    if(end < start)
    {
        LOG_W("incorrect timestamp, start=%llu, end=%llu", start, end);
        return 0;
    }
    return end - start;
}

//获取与utc时间的秒速差值
int64 com_timezone_get_s()
{
    time_t t1, t2;
    time(&t1);
    t2 = t1;
    int64 timezone_s = 0;
    struct tm* tm_local = NULL;
    struct tm* tm_utc = NULL;
    tm_local = localtime(&t1);
    t1 = mktime(tm_local) ;
    tm_utc = gmtime(&t2);
    t2 = mktime(tm_utc);
    timezone_s = (t1 - t2);
    return timezone_s;
}

int64 com_timezone_china_s()
{
    return 8 * 3600;
}

/*  UTC
    %a 星期几的简写
    %A 星期几的全称
    %b 月份的简写
    %B 月份的全称
    %c 标准的日期的时间串
    %C 年份的前两位数字
    %d 十进制表示的每月的第几天
    %D 月/天/年
    %e 在两字符域中，十进制表示的每月的第几天
    %F 年-月-日
    %g 年份的后两位数字，使用基于周的年
    %G 年份，使用基于周的年
    %h 简写的月份名
    %H 24小时制的小时
    %I 12小时制的小时
    %j 十进制表示的每年的第几天
    %m 十进制表示的月份
    %M 十时制表示的分钟数
    %n 新行符
    %p 本地的AM或PM的等价显示
    %r 12小时的时间
    %R 显示小时和分钟：hh:mm
    %S 十进制的秒数
    %t 水平制表符
    %T 显示时分秒：hh:mm:ss
    %u 每周的第几天，星期一为第一天 （值从1到7，星期一为1）
    %U 第年的第几周，把星期日作为第一天（值从0到53）
    %V 每年的第几周，使用基于周的年
    %w 十进制表示的星期几（值从0到6，星期天为0）
    %W 每年的第几周，把星期一做为第一天（值从0到53）
    %x 标准的日期串
    %X 标准的时间串
    %y 不带世纪的十进制年份（值从0到99）
    %Y 带世纪部分的十制年份
    %z，%Z 时区名称，如果不能得到时区名称则返回空字符
    %% 百分号
*/
std::string com_time_to_string(uint64 time_s, const char* format)
{
    std::string str;
    if(format == NULL)
    {
        return std::string();
    }
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(struct tm));
    if(com_time_to_tm(time_s, &tm_val) == false)
    {
        return str;
    }
    char buf[128];
    memset(buf, 0, sizeof(buf));
    if(strftime(buf, sizeof(buf), format, &tm_val) > 0)
    {
        str = buf;
    }
    return str;
}

//UTC
bool com_time_to_tm(uint64 time_s,  struct tm* tm_val)
{
    if(tm_val == NULL)
    {
        return false;
    }
    time_t val = time_s;
#if defined(_WIN32) || defined(_WIN64)
    if(gmtime_s(tm_val, &val) != 0)
#else
    if(gmtime_r(&val, tm_val) == NULL)
#endif
    {
        return false;
    }
    return true;
}

//UTC
uint64 com_time_from_tm(struct tm* tm_val)
{
    if(tm_val == NULL)
    {
        return 0;
    }
#if defined(_WIN32) || defined(_WIN64)
#else
    tm_val->tm_gmtoff = 0;
    tm_val->tm_zone = NULL;
#endif
    //mktime默认会扣除当前系统配置的时区秒数,由于我们传入的参数默认为UTC时间，因此需要再加上时区秒数
    return mktime(tm_val) + com_timezone_get_s();
}

uint64 com_time_from_string(const char* date_str, const char* format)
{
    if(date_str == NULL)
    {
        return 0;
    }
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(struct tm));
    int count = sscanf(date_str, format,
                       &tm_val.tm_year, &tm_val.tm_mon, &tm_val.tm_mday,
                       &tm_val.tm_hour, &tm_val.tm_min, &tm_val.tm_sec);
    if(count < 3)
    {
        return 0;
    }
    tm_val.tm_year -= 1900;
    tm_val.tm_mon -= 1;
    return com_time_from_tm(&tm_val);
}

uint8 com_uint8_to_bcd(uint8 v)
{
    return ((v / 10) << 4 | v % 10);
}

uint8 com_bcd_to_uint8(uint8 bcd)
{
    uint8 tmp = 0;
    tmp = ((bcd & 0xF0) >> 0x4) * 10;
    return (tmp + (bcd & 0x0F));
}

bool com_sem_init(Sem* sem, const char* name)
{
    if(sem == NULL)
    {
        return false;
    }
    if(name == NULL)
    {
        name = "Unknown";
    }
    sem->name = name;
#if defined(_WIN32) || defined(_WIN64)
    sem->handle = CreateEvent(
                      NULL,               // default security attributes
                      FALSE,              // manual-reset event?
                      FALSE,              // initial state is nonsignaled
                      NULL                // object name
                  );
#elif __linux__ == 1
    if(sem_init(&sem->handle, 0, 0) != 0)
    {
        return false;
    }
#elif defined(__APPLE__)
    sem->handle = dispatch_semaphore_create(0);
#else
    {
        return false;
    }
#endif
    return true;
}

bool com_sem_uninit(Sem* sem)
{
    if(sem == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(sem->handle == NULL)
    {
        return false;
    }
    CloseHandle(sem->handle);
    return true;
#elif __linux__ == 1
    sem_destroy(&sem->handle);
    return true;
#elif defined(__APPLE__)
    return true;// not need destory sem
#else
    return false;
#endif
}

Sem* com_sem_create(const char* name)
{
    if(name == NULL)
    {
        name = "Unknown";
    }
    Sem* sem = new Sem();
    sem->name = name;
#if defined(_WIN32) || defined(_WIN64)
    sem->handle = CreateEvent(
                      NULL,               // default security attributes
                      FALSE,              // manual-reset event?
                      FALSE,              // initial state is nonsignaled
                      NULL                // object name
                  );
#elif __linux__ == 1
    if(sem_init(&sem->handle, 0, 0) != 0)
    {
        delete sem;
        return NULL;
    }
#elif defined(__APPLE__)
    sem->handle = dispatch_semaphore_create(0);
#else
#endif
    return sem;
}

bool com_sem_post(Sem* sem)
{
    if(sem == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(sem->handle == NULL)
    {
        return false;
    }
    return SetEvent(sem->handle);
#elif __linux__ == 1
    if(sem_post(&sem->handle) != 0)
    {
        return false;
    }
    return true;
#elif defined(__APPLE__)
    if(sem->handle == NULL)
    {
        return false;
    }
    dispatch_semaphore_signal(sem->handle);
    return true;
#else
    return false;
#endif
}

bool com_sem_wait(Sem* sem, int timeout_ms)
{
    if(sem == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(sem->handle == NULL)
    {
        return false;
    }
    if(timeout_ms <= 0)
    {
        timeout_ms = INFINITE;
    }
    int rc = WaitForSingleObject(sem->handle, timeout_ms);
    if(rc != WAIT_OBJECT_0)
    {
        return false;
    }
    return true;
#elif __linux__ == 1
    if(timeout_ms > 0)
    {
        struct timespec ts;
        memset(&ts, 0, sizeof(struct timespec));

        clock_gettime(CLOCK_REALTIME, &ts);
        uint64 tmp = ts.tv_nsec + (int64)timeout_ms * 1000 * 1000;
        ts.tv_nsec = tmp % (1000 * 1000 * 1000);
        ts.tv_sec += tmp / (1000 * 1000 * 1000);

        int ret = sem_timedwait(&sem->handle, &ts);
        if(ret != 0)
        {
            if(errno == ETIMEDOUT)
            {
                //LOG_W("timeout:%s:%d", sem->name.c_str(), timeout_ms);
            }
            else
            {
                //LOG_E("failed,ret=%d,errno=%d", ret, errno);
            }
            return false;
        }
    }
    else
    {
        if(sem_wait(&sem->handle) != 0)
        {
            return false;
        }
    }
    return true;
#elif defined(__APPLE__)
    if(sem->handle == NULL)
    {
        return false;
    }
    dispatch_time_t timeout = DISPATCH_TIME_FOREVER;
    if(timeout_ms > 0)
    {
        timeout = dispatch_time(DISPATCH_TIME_NOW, (int64)timeout_ms * NSEC_PER_MSEC);
    }
    return dispatch_semaphore_wait(sem->handle, timeout) == 0;
#else
    return false;
#endif
}

bool com_sem_destroy(Sem* sem)
{
    if(sem == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(sem->handle == NULL)
    {
        return false;
    }
    CloseHandle(sem->handle);
#elif __linux__ == 1
    sem_destroy(&sem->handle);
#endif
    delete sem;
    return true;
}

bool com_mutex_init(Mutex* mutex, const char* name)
{
    if(mutex == NULL)
    {
        return false;
    }
    if(name == NULL)
    {
        name = "Unknown";
    }
    mutex->name = name;
#if defined(_WIN32) || defined(_WIN64)
    mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
#else
    if(pthread_mutex_init(&mutex->handle, NULL) != 0)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#endif
    return true;
}

bool com_mutex_uninit(Mutex* mutex)
{
    if(mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(mutex->handle != NULL)
    {
        CloseHandle(mutex->handle);
    }
#else
    pthread_mutex_destroy(&mutex->handle);
#endif
    return true;
}

Mutex* com_mutex_create(const char* name)
{
    if(name == NULL)
    {
        name = "Unknown";
    }
    Mutex* mutex = new Mutex();
    mutex->name = name;
#if defined(_WIN32) || defined(_WIN64)
    mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
#else
    if(pthread_mutex_init(&mutex->handle, NULL) != 0)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        delete mutex;
        mutex = NULL;
    }
#endif
    return mutex;
}

bool com_mutex_destroy(Mutex* mutex)
{
    if(mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if(mutex->handle != NULL)
    {
        CloseHandle(mutex->handle);
    }
#else
    pthread_mutex_destroy(&mutex->handle);
#endif
    delete mutex;
    return true;
}

bool com_mutex_lock(Mutex* mutex)
{
    if(mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
    int ret = -1;
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s LOCKED", mutex->name, com_thread_get_name().c_str());
#endif
#if defined(_WIN32) || defined(_WIN64)
    if(mutex->handle == NULL)
    {
        LOG_E("mutex->handle is NULL");
        return false;
    }
    ret = WaitForSingleObject(mutex->handle, INFINITE);
#else
    ret = pthread_mutex_lock(&mutex->handle);
#endif
    if(ret != 0)
    {
        LOG_E("pthread_mutex_lock lock failed,ret=%d", ret);
        return false;
    }
    return true;
}

bool com_mutex_unlock(Mutex* mutex)
{
    if(mutex == NULL)
    {
        LOG_E("%s:%d failed", __FUNC__, __LINE__);
        return false;
    }
    int ret = -1;
#if defined(_WIN32) || defined(_WIN64)
    if(mutex->handle == NULL)
    {
        LOG_E("mutex->handle is NULL");
        return false;
    }
    ret = ReleaseSemaphore(mutex->handle, 1, NULL);
#else
    ret = pthread_mutex_unlock(&mutex->handle);
#endif
    if(ret != 0)
    {
        LOG_E("pthread_mutex_lock lock failed, ret=%d", ret);
        return false;
    }
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s UNLOCKED",
          mutex->name, com_thread_get_name().c_str());
#endif
    return true;
}

double com_cycle_perimeter(double diameter)
{
    return diameter * COM_PI / 180.0;
}

double com_gps_distance_m(double lon_a, double lat_a, double lon_b,
                          double lat_b)
{
    double radLat1 = com_cycle_perimeter(lat_a);
    double radLat2 = com_cycle_perimeter(lat_b);
    double a = radLat1 - radLat2;
    double b = com_cycle_perimeter(lon_a) - com_cycle_perimeter(lon_b);
    double distance_km = 2 * asin(sqrt(pow(sin(a / 2), 2) +
                                       cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2)));
    distance_km = distance_km * COM_EARTH_RADIUS_KM;
    double distance_m = (round(distance_km * 10000) / 10000) * 1000;
    return distance_m;
}

static bool com_gps_is_outsize_china(double longitude, double latitude)
{
    if(longitude < 72.004 || longitude > 137.8347)
    {
        return true;
    }
    if(latitude < 0.8293 || latitude > 55.8271)
    {
        return true;
    }
    return false;
}

static double gps_transform_latitude(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 *
                 sqrt(x > 0 ? x : -x);
    ret += (20.0 * sin(6.0 * x * COM_PI) + 20.0 * sin(2.0 * x * COM_PI)) * 2.0 /
           3.0;
    ret += (20.0 * sin(y * COM_PI) + 40.0 * sin(y / 3.0 * COM_PI)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * COM_PI) + 320 * sin(y * COM_PI / 30.0)) * 2.0 /
           3.0;
    return ret;
}

static double gps_transform_longitude(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(
                     x > 0 ? x : -x);
    ret += (20.0 * sin(6.0 * x * COM_PI) + 20.0 * sin(2.0 * x * COM_PI)) * 2.0 /
           3.0;
    ret += (20.0 * sin(x * COM_PI) + 40.0 * sin(x / 3.0 * COM_PI)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * COM_PI) + 300.0 * sin(x / 30.0 * COM_PI)) * 2.0 /
           3.0;
    return ret;
}

GPS com_gps_wgs84_to_gcj02(double longitude, double latitude)
{
    GPS gps;
    memset(&gps, 0, sizeof(GPS));
    if(com_gps_is_outsize_china(longitude, latitude))
    {
        gps.latitude = latitude;
        gps.longitude = longitude;
        return gps;
    }
    double dLat = gps_transform_latitude(longitude - 105.0, latitude - 35.0);
    double dLon = gps_transform_longitude(longitude - 105.0, latitude - 35.0);
    double radLat = latitude / 180.0 * COM_PI;
    double magic = sin(radLat);
    magic = 1 - COM_EARTH_OBLATENESS * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((COM_EARTH_RADIUS_M * (1 - COM_EARTH_OBLATENESS)) /
                             (magic * sqrtMagic) * COM_PI);
    dLon = (dLon * 180.0) / (COM_EARTH_RADIUS_M / sqrtMagic * cos(radLat) * COM_PI);
    gps.latitude = latitude + dLat;
    gps.longitude = longitude + dLon;
    return gps;
}

GPS com_gps_gcj02_to_wgs84(double longitude, double latitude)
{
    GPS gps;
    GPS gps_temp;
    GPS gps_shift;
    memset(&gps, 0, sizeof(GPS));
    gps.latitude = latitude;
    gps.longitude = longitude;
    memset(&gps_temp, 0, sizeof(GPS));
    memset(&gps_shift, 0, sizeof(GPS));
    while(true)
    {
        gps_temp = com_gps_wgs84_to_gcj02(gps.longitude, gps.latitude);
        gps_shift.latitude = latitude - gps_temp.latitude;
        gps_shift.longitude = longitude - gps_temp.longitude;
        // 1e-7 ~ centimeter level accuracy
        if(fabs(gps_shift.latitude) < 1e-7 && fabs(gps_shift.longitude) < 1e-7)
        {
            // Result of experiment:
            //   Most of the time 2 iterations would be enough
            //   for an 1e-8 accuracy (milimeter level).
            //
            return gps;
        }
        gps.latitude += gps_shift.latitude;
        gps.longitude += gps_shift.longitude;
    }
    return gps;
}

GPS com_gps_bd09_to_gcj02(double longitude, double latitude)
{
    double x = longitude - 0.0065, y = latitude - 0.006;
    double z = sqrt(x * x + y * y) - 0.00002 * sin(y * COM_PI_GPS_BD);
    double theta = atan2(y, x) - 0.000003 * cos(x * COM_PI_GPS_BD);
    GPS gps;
    memset(&gps, 0, sizeof(GPS));
    gps.latitude = z * sin(theta);
    gps.longitude = z * cos(theta);
    return gps;
}

GPS com_gps_gcj02_to_bd09(double longitude, double latitude)
{
    double x = longitude;
    double y = latitude;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * COM_PI_GPS_BD);
    double theta = atan2(y, x) + 0.000003 * cos(x * COM_PI_GPS_BD);
    GPS gps;
    memset(&gps, 0, sizeof(GPS));
    gps.latitude = z * sin(theta) + 0.006;
    gps.longitude = z * cos(theta) + 0.0065;
    return gps;
}

//ip是大端结构
uint32 com_string_to_ip(const char* ip_str)
{
    uint32 ip[4];
    int ret = sscanf(ip_str, "%u.%u.%u.%u",
                     &ip[0], &ip[1], &ip[2], &ip[3]);
    if(ret != 4)
    {
        return 0;
    }
    if(ip[0] > 255 || ip[1] > 255 || ip[2] > 255 || ip[3] > 255)
    {
        return 0;
    }
    uint32 val = 0;
    val = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
    return val;
}

std::string com_ip_to_string(uint32 ip)
{
    static char str[64];
    snprintf(str, sizeof(str),
             "%u.%u.%u.%u", (ip) & 0xFF, ((ip) >> 8) & 0xFF,
             ((ip) >> 16) & 0xFF, ((ip) >> 24) & 0xFF);
    return str;
}

std::string com_get_bin_name()
{
    mutex_app_path.lock();
    static std::string name;
    if(name.empty())
    {
#if defined(_WIN32) || defined(_WIN64)
        wchar_t name_buf[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, name_buf, sizeof(name_buf) / sizeof(wchar_t));
        name = com_wstring_to_utf8(name_buf).toString();
        wchar_t* pos = wcsrchr(name_buf, L'\\');
        if(pos != NULL)
        {
            pos++;
            name = com_wstring_to_utf8(pos).toString();
        }
        else
        {
            name = com_wstring_to_utf8(name_buf).toString();
        }
#elif defined(__APPLE__)
        char buf[256];
        memset(buf, 0, sizeof(buf));
        int ret = proc_name(getpid(), buf, sizeof(buf));
        if(ret > 0)
        {
            name = buf;
        }
#else
        char buf[256];
        memset(buf, 0, sizeof(buf));
        int ret = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if(ret < 0 || (ret >= (int)sizeof(buf) - 1))
        {
            mutex_app_path.unlock();
            return std::string();
        }
        for(int i = ret; i >= 0; i--)
        {
            if(buf[i] == '/')
            {
                name = buf + i + 1;
                break;
            }
        }
#endif
    }
    mutex_app_path.unlock();
    return name;
}

std::string com_get_bin_path()
{
    mutex_app_path.lock();
    static std::string path;
    if(path.empty())
    {
#if defined(_WIN32) || defined(_WIN64)
        wchar_t name_buf[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, name_buf, sizeof(name_buf) / sizeof(wchar_t));
        wchar_t* pos = wcsrchr(name_buf, L'\\');
        if(pos)
        {
            path = com_wstring_to_utf8(std::wstring(name_buf).assign(name_buf, wcslen(name_buf) - wcslen(pos) + 1)).toString();
        }
#elif defined(__APPLE__)
        char buf[PROC_PIDPATHINFO_MAXSIZE];
        memset(buf, 0, sizeof(buf));
        int ret = proc_pidpath(getpid(), buf, sizeof(buf));
        if(ret > 0)
        {
            buf[sizeof(buf) - 1] = '\0';
            path = buf;

            std::string prefix = ".app/";
            auto iter = path.find(prefix);
            if(iter != std::string::npos)
            {
                path = path.substr(0, iter + prefix.size());
            }
            path = com_path_dir(path.c_str());
        }
#else
        char buf[256];
        memset(buf, 0, sizeof(buf));
        int ret = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if(ret < 0 || (ret >= (int)sizeof(buf) - 1))
        {
            mutex_app_path.unlock();
            return std::string();
        }
        for(int i = ret; i >= 0; i--)
        {
            if(buf[i] == '/')
            {
                buf[i + 1] = '\0';
                path = buf;
                break;
            }
        }
#endif
    }
    mutex_app_path.unlock();
    return path;
}

std::string com_get_cwd()
{
    std::string dir;
    char tmp[1024];
#if defined(_WIN32) || defined(_WIN64)
    if(_getcwd(tmp, sizeof(tmp)) != NULL)
#else
    if(getcwd(tmp, sizeof(tmp)) != NULL)
#endif
    {
        dir = tmp;
    }
    return dir;
}

void com_set_cwd(const char* dir)
{
    if(dir != NULL)
    {
#ifdef _WIN32
        _chdir(dir);
#else
        chdir(dir);
#endif
    }
}

uint64 com_ptr_to_number(const void* ptr)
{
    uint64 val = 0;
    memcpy(&val, &ptr, sizeof(void*));
    return val;
}

void* com_number_to_ptr(const uint64 val)
{
    void* ptr = NULL;
    memcpy(&ptr, &val, sizeof(void*));
    return ptr;
}

std::string com_uuid_generator()
{
    std::string val = com_string_format("%d-%llu-%llu-%llu-%u",
                                        com_process_get_pid(),
                                        com_thread_get_tid(),
                                        com_time_rtc_us(),
                                        com_time_cpu_us(),
                                        com_rand(0, 0xFFFFFFFF));
    return CPPMD5::Digest(val.data(), val.size()).toHexString(false);
}

//计算最大公约数
int com_gcd(int x, int y)
{
    return y ? com_gcd(y, x % y) : x;
}

int com_user_get_uid(const char* user)
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return -1;
#else
    if(user == NULL || 0 == strlen(user))
    {
        return getuid();
    }
    struct passwd* pw = getpwnam(user);
    if(pw == NULL)
    {
        return -1;
    }

    return pw->pw_uid;
#endif
}

int com_user_get_gid(const char* user)
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return -1;
#else
    if(user == NULL || 0 == strlen(user))
    {
        return getgid();
    }
    struct passwd* pw = getpwnam(user);
    if(pw == NULL)
    {
        return -1;
    }

    return pw->pw_gid;
#endif
}

std::string com_user_get_name(int uid)
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return std::string();
#else
    struct passwd* pw = getpwuid(uid);
    if(pw == NULL || pw->pw_name == NULL)
    {
        return std::string();
    }

    return pw->pw_name;
#endif
}

std::string com_user_get_home(const char* user)
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return std::string();
#else
    if(user == NULL || user[0] == '\0')
    {
        return std::string();
    }

    struct passwd* pw = getpwnam(user);
    if(pw == NULL)
    {
        return std::string();
    }

    if(pw->pw_dir != NULL)
    {
        return pw->pw_dir;
    }

    struct passwd* p = getpwuid(pw->pw_uid);
    if(p == NULL)
    {
        return std::string();
    }

    return p->pw_dir;
#endif
}

std::string com_user_get_home(const std::string& user)
{
    return com_user_get_home(user.c_str());
}

int com_user_get_uid_logined()
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return 0;
#else
    struct passwd pwd;
    struct passwd* result = NULL;
    char buf[1024];
    memset(&pwd, 0, sizeof(pwd));
    memset(buf, 0, sizeof(buf));
    int ret = getpwuid_r(com_user_get_uid(com_user_get_name_logined().c_str()), &pwd, buf,
                         sizeof(buf), &result);
    if(ret != 0 || result == NULL)
    {
        return -1;
    }
    return result->pw_uid;
#endif
}

int com_user_get_gid_logined()
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return 0;
#else
    struct passwd pwd;
    struct passwd* result = NULL;
    char buf[1024];
    memset(&pwd, 0, sizeof(pwd));
    memset(buf, 0, sizeof(buf));
    int ret = getpwuid_r(com_user_get_uid(com_user_get_name_logined().c_str()), &pwd, buf,
                         sizeof(buf), &result);
    if(ret != 0 || result == NULL)
    {
        return -1;
    }
    return result->pw_gid;
#endif
}

std::string com_user_get_name_logined()
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return std::string();
#else
    char buf[128] = {0};
    int ret = getlogin_r(buf, sizeof(buf));
    if(ret != 0 || buf[0] == '\0')
    {
        const char* user = getenv("USER");
        if(user == NULL)
        {
            return std::string();
        }
        return user;
    }
    buf[sizeof(buf) - 1] = '\0';
    return buf;
#endif
}

std::string com_user_get_home_logined()
{
#if defined(_WIN32) || defined(_WIN64)
    LOG_E("api not support yet");
    return std::string();
#else
    const char* snap_user_data = getenv("SNAP_USER_DATA");
    if(snap_user_data != NULL)
    {
        return std::string(snap_user_data);
    }

    const char* home_dir = getenv("HOME");
    if(home_dir != NULL)
    {
        return home_dir;
    }
    std::vector<char> buf(16384);

    struct passwd pw;
    struct passwd* result = NULL;
    memset(&pw, 0, sizeof(struct passwd));
    getpwuid_r(getuid(), &pw, &buf[0], buf.size(), &result);
    if(result == NULL || result->pw_dir == NULL)
    {
        return std::string();
    }
    return result->pw_dir;
#endif
}

std::string com_user_get_display_logined()
{
#if __linux__ == 1
    const char* display = getenv("DISPLAY");
    if(display == NULL)
    {
        return std::string();
    }
    return display;
#else
    LOG_E("api not support yet");
    return std::string();
#endif
}

std::string com_user_get_language()
{
    std::string local_lang;
#if defined(_WIN32) || defined(_WIN64)
    DWORD id = GetUserDefaultUILanguage();
    wchar_t lang_name[64];
    GetLocaleInfoW(id, LOCALE_SISO639LANGNAME, lang_name, sizeof(lang_name) / sizeof(wchar_t));
    wchar_t lang_country[64];
    GetLocaleInfoW(id, LOCALE_SISO3166CTRYNAME, lang_country, sizeof(lang_country) / sizeof(wchar_t));
    return com_wstring_to_utf8(com_wstring_format(L"%s-%s", lang_name, lang_country)).toString();
#else
    try
    {
        std::locale val("");
        local_lang = val.name();
        com_string_to_lower(local_lang);
    }
    catch(std::exception& e)
    {
        LOG_E("%s", e.what());
    }
#endif

    return local_lang;
}

CPPBytes::CPPBytes()
{
    buf.clear();
}

CPPBytes::CPPBytes(int reserve_size)
{
    if(reserve_size > 0)
    {
        buf.reserve(reserve_size);
    }
}

CPPBytes::CPPBytes(const CPPBytes& bytes)
{
    if(this == &bytes)
    {
        return;
    }
    buf = bytes.buf;
}

CPPBytes::CPPBytes(CPPBytes&& bytes)
{
    if(this == &bytes)
    {
        return;
    }
    buf.swap(bytes.buf);
}

CPPBytes::CPPBytes(const uint8* data, int data_size)
{
    if(data_size > 0 && data != NULL)
    {
        buf.insert(buf.end(), data, data + data_size);
    }
}

CPPBytes::CPPBytes(const char* data)
{
    int len = com_string_len(data);
    if(len > 0)
    {
        buf.insert(buf.end(), data, data + len);
    }
}

CPPBytes::CPPBytes(const std::string& data)
{
    if(data.empty() == false)
    {
        buf.insert(buf.end(), data.data(), data.data() + data.size());
    }
}

CPPBytes::CPPBytes(const char* data, int data_size)
{
    if(data_size > 0 && data != NULL)
    {
        buf.insert(buf.end(), data, data + data_size);
    }
}

CPPBytes::~CPPBytes()
{
    buf.clear();
}

CPPBytes& CPPBytes::operator=(const CPPBytes& bytes)
{
    if(this == &bytes)
    {
        return *this;
    }
    buf = bytes.buf;
    return *this;
}

CPPBytes& CPPBytes::operator=(CPPBytes&& bytes)
{
    if(this == &bytes)
    {
        return *this;
    }
    buf.swap(bytes.buf);
    return *this;
}

CPPBytes& CPPBytes::operator+(uint8 val)
{
    buf.push_back(val);
    return *this;
}

CPPBytes& CPPBytes::operator+=(uint8 val)
{
    buf.push_back(val);
    return *this;
}

bool CPPBytes::operator==(CPPBytes& bytes)
{
    if(this == &bytes)
    {
        return true;
    }
    if(bytes.getDataSize() != this->getDataSize())
    {
        return false;
    }
    for(int i = 0; i < bytes.getDataSize(); i++)
    {
        if(bytes.getData()[i] != this->getData()[i])
        {
            return false;
        }
    }
    return true;
}

bool CPPBytes::operator!=(CPPBytes& bytes)
{
    if(this == &bytes)
    {
        return false;
    }
    if(bytes.getDataSize() != this->getDataSize())
    {
        return true;
    }
    for(int i = 0; i < bytes.getDataSize(); i++)
    {
        if(bytes.getData()[i] != this->getData()[i])
        {
            return true;
        }
    }
    return false;
}

void CPPBytes::reserve(int size)
{
    if(size > 0)
    {
        buf.reserve(size);
    }
}

void CPPBytes::clear()
{
    buf.clear();
}

uint8* CPPBytes::getData()
{
    if(empty())
    {
        return NULL;
    }
    return &buf[0];
}

const uint8* CPPBytes::getData() const
{
    if(empty())
    {
        return NULL;
    }
    return &buf[0];
}

int CPPBytes::getDataSize() const
{
    return buf.size();
}

bool CPPBytes::empty() const
{
    return (buf.size() <= 0);
}

CPPBytes& CPPBytes::append(uint8 val)
{
    buf.push_back(val);
    return *this;
}

CPPBytes& CPPBytes::append(const uint8* data, int data_size)
{
    if(data_size > 0 && data != NULL)
    {
        buf.insert(buf.end(), data, data + data_size);
    }
    return *this;
}

CPPBytes& CPPBytes::append(const char* data)
{
    int len = com_string_len(data);
    if(len > 0)
    {
        buf.insert(buf.end(), data, data + len);
    }
    return *this;
}

CPPBytes& CPPBytes::append(const std::string& data)
{
    return append((uint8*)data.data(), data.size());
}

CPPBytes& CPPBytes::append(const CPPBytes& bytes)
{
    if(this == &bytes)
    {
        CPPBytes tmp = bytes;
        buf.insert(buf.end(), tmp.getData(), tmp.getData() + tmp.getDataSize());
    }
    else
    {
        buf.insert(buf.end(), bytes.getData(), bytes.getData() + bytes.getDataSize());
    }
    return *this;
}

CPPBytes& CPPBytes::insert(int pos, uint8 val)
{
    if(pos < 0 || pos >= (int)buf.size())
    {
        return *this;
    }
    buf.insert(buf.begin() + pos, val);
    return *this;
}

CPPBytes& CPPBytes::insert(int pos, const uint8* data, int data_size)
{
    if(pos < 0 || pos >= (int)buf.size() || data == NULL || data_size <= 0)
    {
        return *this;
    }
    buf.insert(buf.begin() + pos, data, data + data_size);
    return *this;
}

CPPBytes& CPPBytes::insert(int pos, const char* data)
{
    if(pos < 0 || pos >= (int)buf.size())
    {
        return *this;
    }
    int len = com_string_len(data);
    if(len <= 0)
    {
        return *this;
    }
    buf.insert(buf.begin() + pos, data, data + len);
    return *this;
}

CPPBytes& CPPBytes::insert(int pos, CPPBytes& bytes)
{
    if(pos < 0 || pos >= (int)buf.size() || this == &bytes)
    {
        return *this;
    }
    buf.insert(buf.begin() + pos, bytes.buf.begin(), bytes.buf.end());
    return *this;
}

uint8 CPPBytes::getAt(int pos) const
{
    if(pos < 0 || pos >= (int)buf.size())
    {
        return 0;
    }
    return buf[pos];
}

void CPPBytes::setAt(int pos, uint8 val)
{
    if(pos < 0 || pos >= (int)buf.size())
    {
        return;
    }
    buf[pos] = val;
}

void CPPBytes::removeAt(int pos)
{
    if(pos < 0 || pos >= (int)buf.size())
    {
        return;
    }
    buf.erase(buf.begin() + pos);
}

void CPPBytes::removeHead(int count)
{
    if(count <= 0)
    {
        return;
    }
    if(count > (int)buf.size())
    {
        count = buf.size();
    }
    buf.erase(buf.begin(), buf.begin() + count);
}

void CPPBytes::removeTail(int count)
{
    if(count <= 0)
    {
        return;
    }
    while(count > 0 && buf.size() > 0)
    {
        buf.pop_back();
        count--;
    }
}

bool CPPBytes::toFile(const char* file)
{
    if(buf.size() <= 0)
    {
        return false;
    }
    com_dir_create(com_path_dir(file).c_str());
    FILE* f = com_file_open(file, "wb+");
    if(f == NULL)
    {
        return false;
    }
    int ret = com_file_write(f, getData(), getDataSize());
    com_file_flush(f);
    com_file_close(f);
    return (ret == (int)buf.size());
}

std::string CPPBytes::toString() const
{
    std::string str;
    if(buf.size() > 0)
    {
        str.append((char*)&buf[0], buf.size());
    }
    return str;
}

std::string CPPBytes::toHexString(bool upper) const
{
    std::string str;
    if(buf.size() > 0)
    {
        str = com_bytes_to_hexstring(&buf[0], buf.size());
        if(upper)
        {
            com_string_to_upper(str);
        }
    }
    return str;
}

CPPBytes CPPBytes::FromHexString(const char* hex_str)
{
    return com_hexstring_to_bytes(hex_str);
}

CPPMutex::CPPMutex(const char* name)
{
    if(name != NULL)
    {
        this->name = name;
    }
}

CPPMutex::~CPPMutex()
{
}

void CPPMutex::setName(const char* name)
{
    if(name != NULL)
    {
        this->name = name;
    }
}

const char* CPPMutex::getName()
{
    return name.c_str();
}

void CPPMutex::lock()
{
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s LOCKED", name.c_str(), com_thread_get_name().c_str());
#endif
    mutex.lock();
}

void CPPMutex::unlock()
{
    mutex.unlock();
#ifdef __DEBUG_MUTEX__
    LOG_D("mutex=%s thread=%s UNLOCKED", name.c_str(), com_thread_get_name().c_str());
#endif
}

std::mutex* CPPMutex::getMutex()
{
    return &mutex;
}

AutoMutex::AutoMutex(Mutex* mutex)
{
    this->mutex_cpp = NULL;
    this->mutex_c = mutex;
    com_mutex_lock(this->mutex_c);
}

AutoMutex::AutoMutex(std::mutex* mutex)
{
    this->mutex_c = NULL;
    this->mutex_cpp = mutex;
    if(this->mutex_cpp != NULL)
    {
        this->mutex_cpp->lock();
    }
}

AutoMutex::AutoMutex(CPPMutex& mutex)
{
    this->mutex_c = NULL;
    this->mutex_cpp = mutex.getMutex();
    if(this->mutex_cpp != NULL)
    {
        this->mutex_cpp->lock();
    }
}

AutoMutex::~AutoMutex()
{
    if(this->mutex_c != NULL)
    {
        com_mutex_unlock(this->mutex_c);
    }
    if(this->mutex_cpp != NULL)
    {
        mutex_cpp->unlock();
    }
}

CPPSem::CPPSem(const char* name)
{
    com_sem_init(&sem, name);
}

CPPSem::~CPPSem()
{
    com_sem_uninit(&sem);
}

void CPPSem::setName(const char* name)
{
    if(name == NULL)
    {
        return;
    }
    sem.name = name;
}

const char* CPPSem::getName()
{
    return sem.name.c_str();
}

bool CPPSem::post()
{
    return com_sem_post(&sem);
}

bool CPPSem::wait(int timeout_ms)
{
    return com_sem_wait(&sem, timeout_ms);
}

CPPCondition::CPPCondition(const char* name)
{
    if(name != NULL)
    {
        this->name = name;
    }
}

CPPCondition::~CPPCondition()
{
}

void CPPCondition::setName(const char* name)
{
    this->name = name;
}

const char* CPPCondition::getName()
{
    return name.c_str();
}

bool CPPCondition::notifyOne()
{
    std::unique_lock<std::mutex> lock(mutex_cv);
    condition.notify_one();
    return true;
}

bool CPPCondition::notifyAll()
{
    std::unique_lock<std::mutex> lock(mutex_cv);
    condition.notify_all();
    return true;
}

bool CPPCondition::wait(int timeout_ms)
{
    std::unique_lock<std::mutex> lock(mutex_cv);
    if(timeout_ms > 0)
    {
        if(condition.wait_for(lock, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout)
        {
            return false;
        }
    }
    else
    {
        condition.wait(lock);
    }
    return true;
}

Message::Message()
{
    id = 0;
}

Message::Message(uint32 id)
{
    this->id = id;
}

Message::Message(const Message& msg)
{
    if(this == &msg)
    {
        return;
    }
    this->id = msg.id;
    this->datas = msg.datas;
}

Message::Message(Message&& msg)
{
    if(this == &msg)
    {
        return;
    }
    this->id = msg.id;
    this->datas.swap(msg.datas);
}

Message::~Message()
{
    datas.clear();
}

Message& Message::operator=(const Message& msg)
{
    if(this != &msg)
    {
        this->id = msg.id;
        this->datas = msg.datas;
    }
    return *this;
}

Message& Message::operator=(Message&& msg)
{
    if(this != &msg)
    {
        this->id = msg.id;
        this->datas.swap(msg.datas);
    }
    return *this;
}

void Message::reset()
{
    id = 0;
    datas.clear();
}

uint32 Message::getID() const
{
    return id;
}

Message& Message::setID(uint32 id)
{
    this->id = id;
    return *this;
}

bool Message::isKeyExist(const char* key) const
{
    if(key == NULL || datas.count(key) == 0)
    {
        return false;
    }
    return true;
}

bool Message::isEmpty() const
{
    return datas.empty();
}

Message& Message::set(const char* key, const std::string& val)
{
    if(key != NULL)
    {
        datas[key] = val;
    }
    return *this;
}

Message& Message::set(const char* key, const char* val)
{
    if(key != NULL && val != NULL)
    {
        datas[key] = val;
    }
    return *this;
}

Message& Message::set(const char* key, const uint8* val, int val_size)
{
    if(key != NULL && val != NULL && val_size > 0)
    {
        std::string data;
        data.assign((char*)val, val_size);
        datas[key] = data;
    }
    return *this;
}

Message& Message::set(const char* key, const CPPBytes& bytes)
{
    if(key != NULL && bytes.empty() == false)
    {
        std::string data;
        data.assign((char*)bytes.getData(), bytes.getDataSize());
        datas[key] = data;
    }
    return *this;
}

Message& Message::set(const char* key, const std::vector<std::string>& array)
{
    if(key != NULL && array.empty() == false)
    {
        std::string data;
        data.reserve(array.size() * 64);
        for(size_t i = 0; i < array.size(); i++)
        {
            data += com_bytes_to_hexstring((uint8*)array[i].c_str(), array[i].length()) + ";";
        }
        if(data.back() == ';')
        {
            data.pop_back();
        }
        datas[key] = data;
    }
    return *this;
}

Message& Message::set(const char* key, const Message& msg)
{
    if(key != NULL)
    {
        datas[key] = msg.toJSON();
    }
    return *this;
}

Message& Message::setPtr(const char* key, const void* ptr)
{
    set(key, (uint64)ptr);
    return *this;
}

void Message::remove(const char* key)
{
    if(key == NULL)
    {
        return;
    }
    std::map<std::string, std::string>::iterator it = datas.find(key);
    if(it != datas.end())
    {
        datas.erase(it);
    }
}

void Message::removeAll()
{
    datas.clear();
}

std::vector<std::string> Message::getAllKeys() const
{
    std::vector<std::string> keys;
    for(auto it = datas.begin(); it != datas.end(); it++)
    {
        keys.push_back(it->first);
    }
    return keys;
}

bool Message::getBool(const char* key, bool default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas.at(key);
    com_string_to_lower(val);
    if(val == "true")
    {
        return true;
    }
    else if(val == "false")
    {
        return false;
    }
    else
    {
        return (strtol(val.c_str(), NULL, 10) == 1);
    }
}

char Message::getChar(const char* key, char default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas.at(key);
    if(val.empty())
    {
        return default_val;
    }
    return val[0];
}

float Message::getFloat(const char* key, float default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas.at(key);
    return strtof(val.c_str(), NULL);
}

double Message::getDouble(const char* key, double default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas.at(key);
    return strtod(val.c_str(), NULL);
}

int8 Message::getInt8(const char* key, int8 default_val) const
{
    return (int8)getInt64(key, default_val);
}

int16 Message::getInt16(const char* key, int16 default_val) const
{
    return (int16)getInt64(key, default_val);
}

int32 Message::getInt32(const char* key, int32 default_val) const
{
    return (int32)getInt64(key, default_val);
}

int64 Message::getInt64(const char* key, int64 default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    std::string val = datas.at(key);
    return strtoll(val.c_str(), NULL, 10);
}

uint8 Message::getUInt8(const char* key, uint8 default_val) const
{
    return (uint8)getUInt64(key, default_val);
}

uint16 Message::getUInt16(const char* key, uint16 default_val) const
{
    return (uint16)getUInt64(key, default_val);
}

uint32 Message::getUInt32(const char* key, uint32 default_val) const
{
    return (uint32)getUInt64(key, default_val);
}

uint64 Message::getUInt64(const char* key, uint64 default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    const std::string& val = datas.at(key);
    return strtoull(val.c_str(), NULL, 10);
}

std::string Message::getString(const char* key, std::string default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    return datas.at(key);
}

CPPBytes Message::getBytes(const char* key) const
{
    CPPBytes bytes;
    if(isKeyExist(key) == false)
    {
        return bytes;
    }
    const std::string& val = datas.at(key);
    return CPPBytes((uint8*)val.data(), val.length());
}

uint8* Message::getBytes(const char* key, int& size)
{
    if(isKeyExist(key) == false)
    {
        return NULL;
    }
    const std::string& val = datas.at(key);
    size = val.size();
    return (uint8*)val.data();
}

void* Message::getPtr(const char* key, void* default_val) const
{
    if(isKeyExist(key) == false)
    {
        return default_val;
    }
    return (void*)getUInt64(key);
}

std::vector<std::string> Message::getStringArray(const char* key) const
{
    if(isKeyExist(key) == false)
    {
        return std::vector<std::string>();
    }
    std::vector<std::string> vals = com_string_split(datas.at(key).c_str(), ";");
    std::vector<std::string> array;
    for(size_t i = 0; i < vals.size(); i++)
    {
        array.push_back(com_hexstring_to_bytes(vals[i].c_str()).toString());
    }
    return array;
}

Message Message::getMessage(const char* key) const
{
    if(isKeyExist(key) == false)
    {
        return Message();
    }
    return Message::FromJSON(datas.at(key).c_str());
}

std::string Message::toJSON(bool pretty_style) const
{
    CJsonObject cjson;
    cjson.Add("__JSON_FIELD_MESSAGE_ID__", id);
    for(auto it = datas.cbegin(); it != datas.cend(); it++)
    {
        cjson.Add(it->first, it->second);
    }
    if(pretty_style)
    {
        return cjson.ToFormattedString();
    }
    else
    {
        return cjson.ToString();
    }
}

Message Message::FromJSON(const char* json)
{
    if(json == NULL)
    {
        return Message();
    }
    CJsonObject cjson(json);
    Message msg;
    std::string key;
    std::string value;
    while(cjson.GetKey(key))
    {
        if(cjson.Get(key, value))
        {
            if(key == "__JSON_FIELD_MESSAGE_ID__")
            {
                msg.id = strtoul(value.c_str(), NULL, 10);
            }
            else
            {
                msg.set(key.c_str(), value);
            }
        }
    }
    return msg;
}

Message Message::FromJSON(const std::string& json)
{
    return FromJSON(json.c_str());
}

ByteStreamReader::ByteStreamReader(FILE* fp)
{
    this->fp = fp;
}

ByteStreamReader::ByteStreamReader(const char* file, bool is_file)
{
    this->fp = com_file_open(file, "rb");
    fp_internal = true;
}

ByteStreamReader::ByteStreamReader(const char* buffer)
{
    this->buffer.append(buffer);
}

ByteStreamReader::ByteStreamReader(const std::string& buffer)
{
    this->buffer.append(buffer.c_str());
}

ByteStreamReader::ByteStreamReader(const uint8* buffer, int buffer_size)
{
    this->buffer.append(buffer, buffer_size);
}

ByteStreamReader::ByteStreamReader(const CPPBytes& buffer)
{
    this->buffer.append(buffer);
}

ByteStreamReader::~ByteStreamReader()
{
    if(fp_internal)
    {
        com_file_close(fp);
        fp = NULL;
    }
}

int64 ByteStreamReader::find(const char* key, int offset)
{
    return find((const uint8*)key, com_string_len(key), offset);
}

int64 ByteStreamReader::find(const uint8* key, int key_size, int offset)
{
    if(key == NULL || key_size <= 0)
    {
        return -1;
    }
    if(fp != NULL)
    {
        if(offset > 0)
        {
            com_file_seek_set(fp, offset);
        }
        return com_file_find(fp, key, key_size);
    }
    if(offset > 0)
    {
        buffer_pos = offset;
    }
    int match_count = 0;
    while(buffer_pos >= 0 && buffer_pos < buffer.getDataSize())
    {
        uint8 val = buffer.getAt(buffer_pos++);
        if(val != key[match_count])
        {
            match_count = 0;
            continue;
        }
        match_count++;
        if(match_count == key_size)
        {
            buffer_pos -= key_size;
            return buffer_pos;
        }
    }
    return -3;
}

int64 ByteStreamReader::rfind(const char* key, int offset)
{
    return rfind((const uint8*)key, com_string_len(key), offset);
}

int64 ByteStreamReader::rfind(const uint8* key, int key_size, int offset)
{
    if(key == NULL || key_size <= 0)
    {
        return -1;
    }
    if(fp != NULL)
    {
        if(offset < 0)
        {
            com_file_seek_set(fp, offset);
        }
        return com_file_rfind(fp, key, key_size);
    }
    if(offset < 0)
    {
        buffer_pos = buffer.getDataSize() - offset;
    }
    int match_count = 0;
    while(buffer_pos >= 0 && buffer_pos < buffer.getDataSize())
    {
        uint8 val = buffer.getAt(buffer_pos--);
        if(val != key[match_count])
        {
            match_count = 0;
            continue;
        }
        match_count++;
        if(match_count == key_size)
        {
            buffer_pos += key_size;
            return buffer_pos;
        }
    }
    return -3;
}

bool ByteStreamReader::readLine(std::string& line)
{
    if(fp != NULL)
    {
        return com_file_readline(fp, line);
    }
    std::string line_tmp;
    for(int64 i = buffer_pos; i < buffer.getDataSize(); i++)
    {
        uint8 val = buffer.getAt(i);
        if(val == '\n')
        {
            buffer_pos = i + 1;
            line = line_tmp;
            return true;
        }
        else if(val == '\r')
        {
        }
        else
        {
            line_tmp.push_back(val);
        }
    }
    line = line_tmp;
    return (line_tmp.empty() == false);
}

CPPBytes ByteStreamReader::read(int size)
{
    if(fp != NULL)
    {
        return com_file_read(fp, size);
    }

    if(size <= 0 || buffer_pos < 0 || buffer_pos >= buffer.getDataSize())
    {
        return CPPBytes();
    }
    if(buffer_pos + size > buffer.getDataSize())
    {
        size = buffer.getDataSize() - buffer_pos;
    }
    CPPBytes result;
    result.append(buffer.getData() + buffer_pos, size);
    buffer_pos += size;
    return result;
}

int ByteStreamReader::read(uint8* buf, int buf_size)
{
    if(fp != NULL)
    {
        return com_file_read(fp, buf, buf_size);
    }

    if(buf == NULL || buf_size <= 0 || buffer_pos < 0 || buffer_pos >= buffer.getDataSize())
    {
        return -1;
    }
    if(buffer_pos + buf_size > buffer.getDataSize())
    {
        buf_size = buffer.getDataSize() - buffer_pos;
    }
    memcpy(buf, buffer.getData() + buffer_pos, buf_size);
    buffer_pos += buf_size;
    return buf_size;
}

CPPBytes ByteStreamReader::readUntil(const char* key)
{
    return readUntil((const uint8*)key, com_string_len(key));
}

CPPBytes ByteStreamReader::readUntil(const uint8* key, int key_size)
{
    if(key == NULL || key_size <= 0)
    {
        return CPPBytes();
    }
    if(fp != NULL)
    {
        return com_file_read_until(fp, key, key_size);
    }
    CPPBytes result;
    int match_count = 0;
    while(buffer_pos >= 0 && buffer_pos < buffer.getDataSize())
    {
        uint8 val = buffer.getAt(buffer_pos++);
        if(val != key[match_count])
        {
            match_count = 0;
            continue;
        }
        result.append(val);
        match_count++;
        if(match_count == key_size)
        {
            result.removeTail(key_size);
            buffer_pos -= key_size;
            break;
        }
    }
    return result;
}

CPPBytes ByteStreamReader::readUntil(std::function<bool(uint8)> func)
{
    if(func == NULL)
    {
        return CPPBytes();
    }
    if(fp != NULL)
    {
        return com_file_read_until(fp, func);
    }
    CPPBytes result;
    while(buffer_pos >= 0 && buffer_pos < buffer.getDataSize())
    {
        uint8 val = buffer.getAt(buffer_pos++);
        if(func(val))
        {
            buffer_pos--;
            break;
        }
        else
        {
            result.append(val);
        }
    }
    return result;
}

int64 ByteStreamReader::getPos()
{
    if(fp != NULL)
    {
        return com_file_seek_get(fp);
    }
    return buffer_pos;
}

void ByteStreamReader::setPos(int64 pos)
{
    if(fp != NULL)
    {
        com_file_seek_set(fp, pos);
    }
    else if(pos >= 0 && pos < buffer.getDataSize())
    {
        buffer_pos = pos;
    }
}

void ByteStreamReader::stepPos(int count)
{
    if(fp != NULL)
    {
        com_file_seek_step(fp, count);
    }
    else if(buffer_pos + count >= 0 && buffer_pos + count < buffer.getDataSize())
    {
        buffer_pos += count;
    }
}

void ByteStreamReader::reset()
{
    com_file_seek_head(fp);
    buffer_pos = 0;
}

