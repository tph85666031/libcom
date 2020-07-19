/*
 Copyright (c) 2009 Dave Gamble

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

/* cJSON_x */
/* JSON parser in C. */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON_x.h"

#ifndef INT_MAX
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)
#define UINT_MAX   4294967295U
#endif

static const char *ep;

const char *cJSON_GetErrorPtr_x(void)
{
    return ep;
}

static int cJSON_strcasecmp_x(const char *s1, const char *s2)
{
    if (!s1)
        return (s1 == s2) ? 0 : 1;
    if (!s2)
        return 1;
    for (; tolower(*s1) == tolower(*s2); ++s1, ++s2)
        if (*s1 == 0)
            return 0;
    return tolower(*(const unsigned char *)s1)
                    - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc_x)(size_t sz) = malloc;
static void (*cJSON_free_x)(void *ptr) = free;

static char* cJSON_strdup_x(const char* str)
{
    size_t len;
    char* copy;

    len = strlen(str) + 1;
    if (!(copy = (char*) cJSON_malloc_x(len)))
        return 0;
    memcpy(copy, str, len);
    return copy;
}

void cJSON_InitHooks_x(cJSON_Hooks_x* hooks)
{
    if (!hooks)
    { /* Reset hooks */
        cJSON_malloc_x = malloc;
        cJSON_free_x = free;
        return;
    }

    cJSON_malloc_x = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
    cJSON_free_x = (hooks->free_fn) ? hooks->free_fn : free;
}

/* Internal constructor. */
static cJSON_x *cJSON_New_Item_x(void)
{
    cJSON_x* node = (cJSON_x*) cJSON_malloc_x(sizeof(cJSON_x));
    if (node)
        memset(node, 0, sizeof(cJSON_x));
    return node;
}

/* Delete a cJSON_x structure. */
void cJSON_Delete_x(cJSON_x *c)
{
    cJSON_x *next;
    while (c)
    {
        next = c->next;
        if (!(c->type & cJSON_x_IsReference) && c->child)
            cJSON_Delete_x(c->child);
        if (!(c->type & cJSON_x_IsReference) && c->valuestring)
            cJSON_free_x(c->valuestring);
        if (c->string)
            cJSON_free_x(c->string);
        cJSON_free_x(c);
        c = next;
    }
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number_x(cJSON_x *item, const char *num)
{
    long double n = 0, scale = 0;
    int subscale = 0, signsubscale = 1;
    item->sign = 1;

    /* Could use sscanf for this? */
    if (*num == '-')
        item->sign = -1, num++; /* Has sign? */
    if (*num == '0')
        num++; /* is zero */
    if (*num >= '1' && *num <= '9')
        do
            n = (n * 10.0) + (*num++ - '0');
        while (*num >= '0' && *num <= '9'); /* Number? */
    if (*num == '.' && num[1] >= '0' && num[1] <= '9')
    {
        num++;
        do
            n = (n * 10.0) + (*num++ - '0'), scale--;
        while (*num >= '0' && *num <= '9');
    } /* Fractional part? */
    if (*num == 'e' || *num == 'E') /* Exponent? */
    {
        num++;
        if (*num == '+')
            num++;
        else if (*num == '-')
            signsubscale = -1, num++; /* With sign? */
        while (*num >= '0' && *num <= '9')
            subscale = (subscale * 10) + (*num++ - '0'); /* Number? */
    }

    if (scale == 0 && subscale == 0)
    {
        item->valuedouble = (double)(item->sign * n);
        item->valueint = (uint64)(item->sign * (uint64)n);
        item->type = cJSON_x_Int;
    }
    else
    {
        n = item->sign * n * pow(10.0, (scale + subscale * signsubscale)); /* number = +/- number.fraction * 10^+/- exponent */
        item->valuedouble = (double)n;
        item->valueint = (uint64)n;
        item->type = cJSON_x_Double;
    }
    return num;
}

/* Render the number nicely from the given item into a string. */
static char *print_double_x(cJSON_x *item)
{
    char *str;
    double d = item->valuedouble;
    str = (char*) cJSON_malloc_x(64); /* This is a nice tradeoff. */
    if (str)
    {
        if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9)
            sprintf(str, "%lf", d);
        else
            sprintf(str, "%f", d);
    }
    return str;
}

static char *print_int_x(cJSON_x *item)
{
    char *str;
    str = (char*) cJSON_malloc_x(22); /* 2^64+1 can be represented in 21 chars. */
    if (str)
    {
        if (item->sign == -1)
        {
            if ((int64)item->valueint <= (int64)INT_MAX && (int64)item->valueint >= (int64)INT_MIN)
            {
                sprintf(str, "%d", (int32)item->valueint);
            }
            else
            {
                sprintf(str, "%lld", (int64)item->valueint);
            }
        }
        else
        {
            if (item->valueint <= (uint64)UINT_MAX)
            {
                sprintf(str, "%u", (uint32)item->valueint);
            }
            else
            {
                sprintf(str, "%llu", item->valueint);
            }
        }
    }
    return str;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0,
                0xF8, 0xFC };
static const char *parse_string_x(cJSON_x *item, const char *str)
{
    const char *ptr = str + 1;
    char *ptr2;
    char *out;
    int len = 0;
    unsigned uc, uc2;
    if (*str != '\"')
    {
        ep = str;
        return 0;
    } /* not a string! */

    while (*ptr != '\"' && *ptr && ++len)
        if (*ptr++ == '\\')
            ptr++; /* Skip escaped quotes. */

    out = (char*) cJSON_malloc_x(len + 1); /* This is how long we need for the string, roughly. */
    if (!out)
        return 0;

    ptr = str + 1;
    ptr2 = out;
    while (*ptr != '\"' && *ptr)
    {
        if (*ptr != '\\')
            *ptr2++ = *ptr++;
        else
        {
            ptr++;
            switch (*ptr)
            {
            case 'b':
                *ptr2++ = '\b';
                break;
            case 'f':
                *ptr2++ = '\f';
                break;
            case 'n':
                *ptr2++ = '\n';
                break;
            case 'r':
                *ptr2++ = '\r';
                break;
            case 't':
                *ptr2++ = '\t';
                break;
            case 'u': /* transcode utf16 to utf8. */
                sscanf(ptr + 1, "%4x", &uc);
                ptr += 4; /* get the unicode char. */

                if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0)
                    break;	// check for invalid.

                if (uc >= 0xD800 && uc <= 0xDBFF)	// UTF16 surrogate pairs.
                {
                    if (ptr[1] != '\\' || ptr[2] != 'u')
                        break;	// missing second-half of surrogate.
                    sscanf(ptr + 3, "%4x", &uc2);
                    ptr += 6;
                    if (uc2 < 0xDC00 || uc2 > 0xDFFF)
                        break;	// invalid second-half of surrogate.
                    uc = 0x10000 | ((uc & 0x3FF) << 10) | (uc2 & 0x3FF);
                }

                len = 4;
                if (uc < 0x80)
                    len = 1;
                else if (uc < 0x800)
                    len = 2;
                else if (uc < 0x10000)
                    len = 3;
                ptr2 += len;

                switch (len)
                {
                case 4:
                    *--ptr2 = ((uc | 0x80) & 0xBF);
                    uc >>= 6;
                case 3:
                    *--ptr2 = ((uc | 0x80) & 0xBF);
                    uc >>= 6;
                case 2:
                    *--ptr2 = ((uc | 0x80) & 0xBF);
                    uc >>= 6;
                case 1:
                    *--ptr2 = (uc | firstByteMark[len]);
                }
                ptr2 += len;
                break;
            default:
                *ptr2++ = *ptr;
                break;
            }
            ptr++;
        }
    }
    *ptr2 = 0;
    if (*ptr == '\"')
        ptr++;
    item->valuestring = out;
    item->type = cJSON_x_String;
    return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr_x(const char *str)
{
    const char *ptr;
    char *ptr2, *out;
    int len = 0;
    unsigned char token;

    if (!str)
        return cJSON_strdup_x("");
    ptr = str;
    while ((token = *ptr) && ++len)
    {
        if (strchr("\"\\\b\f\n\r\t", token))
            len++;
        else if (token < 32)
            len += 5;
        ptr++;
    }

    out = (char*) cJSON_malloc_x(len + 3);
    if (!out)
        return 0;

    ptr2 = out;
    ptr = str;
    *ptr2++ = '\"';
    while (*ptr)
    {
        if ((unsigned char) *ptr > 31 && *ptr != '\"' && *ptr != '\\')
            *ptr2++ = *ptr++;
        else
        {
            *ptr2++ = '\\';
            switch (token = *ptr++)
            {
            case '\\':
                *ptr2++ = '\\';
                break;
            case '\"':
                *ptr2++ = '\"';
                break;
            case '\b':
                *ptr2++ = 'b';
                break;
            case '\f':
                *ptr2++ = 'f';
                break;
            case '\n':
                *ptr2++ = 'n';
                break;
            case '\r':
                *ptr2++ = 'r';
                break;
            case '\t':
                *ptr2++ = 't';
                break;
            default:
                sprintf(ptr2, "u%04x", token);
                ptr2 += 5;
                break; /* escape and print */
            }
        }
    }
    *ptr2++ = '\"';
    *ptr2++ = 0;
    return out;
}
/* Invote print_string_ptr_x (which is useful) on an item. */
static char *print_string_x(cJSON_x *item)
{
    return print_string_ptr_x(item->valuestring);
}

/* Predeclare these prototypes. */
static const char *parse_value_x(cJSON_x *item, const char *value);
static char *print_value_x(cJSON_x *item, int depth, int fmt);
static const char *parse_array_x(cJSON_x *item, const char *value);
static char *print_array_x(cJSON_x *item, int depth, int fmt);
static const char *parse_object_x(cJSON_x *item, const char *value);
static char *print_object_x(cJSON_x *item, int depth, int fmt);

/* Utility to jump whitespace and cr/lf */
static const char *skip_x(const char *in)
{
    while (in && *in && (unsigned char) *in <= 32)
        in++;
    return in;
}

/* Parse an object - create a new root, and populate. */
cJSON_x *cJSON_Parse_x(const char *value)
{
    cJSON_x *c = cJSON_New_Item_x();
    ep = 0;
    if (!c)
        return 0; /* memory fail */

    if (!parse_value_x(c, skip_x(value)))
    {
        cJSON_Delete_x(c);
        return 0;
    }
    return c;
}

/* Render a cJSON_x item/entity/structure to text. */
char *cJSON_Print_x(cJSON_x *item)
{
    return print_value_x(item, 0, 1);
}
char *cJSON_PrintUnformatted_x(cJSON_x *item)
{
    return print_value_x(item, 0, 0);
}

/* Parser core - when encountering text, process appropriately. */
static const char *parse_value_x(cJSON_x *item, const char *value)
{
    if (!value)
        return 0; /* Fail on null. */
    if (!strncmp(value, "null", 4))
    {
        item->type = cJSON_x_NULL;
        return value + 4;
    }
    if (!strncmp(value, "false", 5))
    {
        item->type = cJSON_x_False;
        return value + 5;
    }
    if (!strncmp(value, "true", 4))
    {
        item->type = cJSON_x_True;
        item->valueint = 1;
        return value + 4;
    }
    if (*value == '\"')
    {
        return parse_string_x(item, value);
    }
    if (*value == '-' || (*value >= '0' && *value <= '9'))
    {
        return parse_number_x(item, value);
    }
    if (*value == '[')
    {
        return parse_array_x(item, value);
    }
    if (*value == '{')
    {
        return parse_object_x(item, value);
    }

    ep = value;
    return 0; /* failure. */
}

/* Render a value to text. */
static char *print_value_x(cJSON_x *item, int depth, int fmt)
{
    char *out = 0;
    if (!item)
        return 0;
    switch ((item->type) & 255)
    {
    case cJSON_x_NULL:
        out = cJSON_strdup_x("null");
        break;
    case cJSON_x_False:
        out = cJSON_strdup_x("false");
        break;
    case cJSON_x_True:
        out = cJSON_strdup_x("true");
        break;
    case cJSON_x_Int:
        out = print_int_x(item);
        break;
    case cJSON_x_Double:
        out = print_double_x(item);
        break;
    case cJSON_x_String:
        out = print_string_x(item);
        break;
    case cJSON_x_Array:
        out = print_array_x(item, depth, fmt);
        break;
    case cJSON_x_Object:
        out = print_object_x(item, depth, fmt);
        break;
    }
    return out;
}

/* Build an array from input text. */
static const char *parse_array_x(cJSON_x *item, const char *value)
{
    cJSON_x *child;
    if (*value != '[')
    {
        ep = value;
        return 0;
    } /* not an array! */

    item->type = cJSON_x_Array;
    value = skip_x(value + 1);
    if (*value == ']')
        return value + 1; /* empty array. */

    item->child = child = cJSON_New_Item_x();
    if (!item->child)
        return 0; /* memory fail */
    value = skip_x(parse_value_x(child, skip_x(value))); /* skip_x any spacing, get the value. */
    if (!value)
        return 0;

    while (*value == ',')
    {
        cJSON_x *new_item;
        if (!(new_item = cJSON_New_Item_x()))
            return 0; /* memory fail */
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip_x(parse_value_x(child, skip_x(value + 1)));
        if (!value)
            return 0; /* memory fail */
    }

    if (*value == ']')
        return value + 1; /* end of array */
    ep = value;
    return 0; /* malformed. */
}

/* Render an array to text */
static char *print_array_x(cJSON_x *item, int depth, int fmt)
{
    char **entries;
    char *out = 0, *ptr, *ret;
    int len = 5;
    cJSON_x *child = item->child;
    int numentries = 0, i = 0, fail = 0;

    /* How many entries in the array? */
    while (child)
        numentries++, child = child->next;
    /* Allocate an array to hold the values for each */
    entries = (char**) cJSON_malloc_x(numentries * sizeof(char*));
    if (!entries)
        return 0;
    memset(entries, 0, numentries * sizeof(char*));
    /* Retrieve all the results: */
    child = item->child;
    while (child && !fail)
    {
        ret = print_value_x(child, depth + 1, fmt);
        entries[i++] = ret;
        if (ret)
            len += strlen(ret) + 2 + (fmt ? 1 : 0);
        else
            fail = 1;
        child = child->next;
    }

    /* If we didn't fail, try to malloc the output string */
    if (!fail)
        out = (char*) cJSON_malloc_x(len);
    /* If that fails, we fail. */
    if (!out)
        fail = 1;

    /* Handle failure. */
    if (fail)
    {
        for (i = 0; i < numentries; i++)
            if (entries[i])
                cJSON_free_x(entries[i]);
        cJSON_free_x(entries);
        return 0;
    }

    /* Compose the output array. */
    *out = '[';
    ptr = out + 1;
    *ptr = 0;
    for (i = 0; i < numentries; i++)
    {
        strcpy(ptr, entries[i]);
        ptr += strlen(entries[i]);
        if (i != numentries - 1)
        {
            *ptr++ = ',';
            if (fmt)
                *ptr++ = ' ';
            *ptr = 0;
        }
        cJSON_free_x(entries[i]);
    }
    cJSON_free_x(entries);
    *ptr++ = ']';
    *ptr++ = 0;
    return out;
}

/* Build an object from the text. */
static const char *parse_object_x(cJSON_x *item, const char *value)
{
    cJSON_x *child;
    if (*value != '{')
    {
        ep = value;
        return 0;
    } /* not an object! */

    item->type = cJSON_x_Object;
    value = skip_x(value + 1);
    if (*value == '}')
        return value + 1; /* empty array. */

    item->child = child = cJSON_New_Item_x();
    if (!item->child)
        return 0;
    value = skip_x(parse_string_x(child, skip_x(value)));
    if (!value)
        return 0;
    child->string = child->valuestring;
    child->valuestring = 0;
    if (*value != ':')
    {
        ep = value;
        return 0;
    } /* fail! */
    value = skip_x(parse_value_x(child, skip_x(value + 1))); /* skip_x any spacing, get the value. */
    if (!value)
        return 0;

    while (*value == ',')
    {
        cJSON_x *new_item;
        if (!(new_item = cJSON_New_Item_x()))
            return 0; /* memory fail */
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip_x(parse_string_x(child, skip_x(value + 1)));
        if (!value)
            return 0;
        child->string = child->valuestring;
        child->valuestring = 0;
        if (*value != ':')
        {
            ep = value;
            return 0;
        } /* fail! */
        value = skip_x(parse_value_x(child, skip_x(value + 1))); /* skip_x any spacing, get the value. */
        if (!value)
            return 0;
    }

    if (*value == '}')
        return value + 1; /* end of array */
    ep = value;
    return 0; /* malformed. */
}

/* Render an object to text. */
static char *print_object_x(cJSON_x *item, int depth, int fmt)
{
    char **entries = 0, **names = 0;
    char *out = 0, *ptr, *ret, *str;
    int len = 7, i = 0, j;
    cJSON_x *child = item->child;
    int numentries = 0, fail = 0;
    /* Count the number of entries. */
    while (child)
        numentries++, child = child->next;
    /* Allocate space for the names and the objects */
    entries = (char**) cJSON_malloc_x(numentries * sizeof(char*));
    if (!entries)
        return 0;
    names = (char**) cJSON_malloc_x(numentries * sizeof(char*));
    if (!names)
    {
        cJSON_free_x(entries);
        return 0;
    }
    memset(entries, 0, sizeof(char*) * numentries);
    memset(names, 0, sizeof(char*) * numentries);

    /* Collect all the results into our arrays: */
    child = item->child;
    depth++;
    if (fmt)
        len += depth;
    while (child)
    {
        names[i] = str = print_string_ptr_x(child->string);
        entries[i++] = ret = print_value_x(child, depth, fmt);
        if (str && ret)
            len += strlen(ret) + strlen(str) + 2 + (fmt ? 2 + depth : 0);
        else
            fail = 1;
        child = child->next;
    }

    /* Try to allocate the output string */
    if (!fail)
        out = (char*) cJSON_malloc_x(len);
    if (!out)
        fail = 1;

    /* Handle failure */
    if (fail)
    {
        for (i = 0; i < numentries; i++)
        {
            if (names[i])
                cJSON_free_x(names[i]);
            if (entries[i])
                cJSON_free_x(entries[i]);
        }
        cJSON_free_x(names);
        cJSON_free_x(entries);
        return 0;
    }

    /* Compose the output: */
    *out = '{';
    ptr = out + 1;
    if (fmt)
        *ptr++ = '\n';
    *ptr = 0;
    for (i = 0; i < numentries; i++)
    {
        if (fmt)
            for (j = 0; j < depth; j++)
                *ptr++ = '\t';
        strcpy(ptr, names[i]);
        ptr += strlen(names[i]);
        *ptr++ = ':';
        if (fmt)
            *ptr++ = '\t';
        strcpy(ptr, entries[i]);
        ptr += strlen(entries[i]);
        if (i != numentries - 1)
            *ptr++ = ',';
        if (fmt)
            *ptr++ = '\n';
        *ptr = 0;
        cJSON_free_x(names[i]);
        cJSON_free_x(entries[i]);
    }

    cJSON_free_x(names);
    cJSON_free_x(entries);
    if (fmt)
        for (i = 0; i < depth - 1; i++)
            *ptr++ = '\t';
    *ptr++ = '}';
    *ptr++ = 0;
    return out;
}

/* Get Array size/item / object item. */
int cJSON_GetArraySize_x(cJSON_x *array)
{
    cJSON_x *c = array->child;
    int i = 0;
    while (c)
        i++, c = c->next;
    return i;
}
cJSON_x *cJSON_GetArrayItem_x(cJSON_x *array, int item)
{
    cJSON_x *c = array->child;
    while (c && item > 0)
        item--, c = c->next;
    return c;
}
cJSON_x *cJSON_GetObjectItem_x(cJSON_x *object, const char *string)
{
    cJSON_x *c = object->child;
    while (c && cJSON_strcasecmp_x(c->string, string))
        c = c->next;
    return c;
}

/* Utility for array list handling. */
static void suffix_object_x(cJSON_x *prev, cJSON_x *item)
{
    prev->next = item;
    item->prev = prev;
}
/* Utility for handling references. */
static cJSON_x *create_reference_x(cJSON_x *item)
{
    cJSON_x *ref = cJSON_New_Item_x();
    if (!ref)
        return 0;
    memcpy(ref, item, sizeof(cJSON_x));
    ref->string = 0;
    ref->type |= cJSON_x_IsReference;
    ref->next = ref->prev = 0;
    return ref;
}

/* Add item to array/object. */
void cJSON_AddItemToArray_x(cJSON_x *array, cJSON_x *item)
{
    cJSON_x *c = array->child;
    if (!item)
        return;
    if (!c)
    {
        array->child = item;
    }
    else
    {
        while (c && c->next)
            c = c->next;
        suffix_object_x(c, item);
    }
}

void cJSON_AddItemToArrayHead_x(cJSON_x *array, cJSON_x *item)
{
    cJSON_x *c = array->child;
    if (!item)
        return;
    if (!c)
    {
        array->child = item;
    }
    else
    {
        item->prev = c->prev;
        item->next = c;
        c->prev = item;
        array->child = item;
    }
}

void cJSON_AddItemToObject_x(cJSON_x *object, const char *string, cJSON_x *item)
{
    if (!item)
        return;
    if (item->string)
        cJSON_free_x(item->string);
    item->string = cJSON_strdup_x(string);
    cJSON_AddItemToArray_x(object, item);
}
void cJSON_AddItemReferenceToArray_x(cJSON_x *array, cJSON_x *item)
{
    cJSON_AddItemToArray_x(array, create_reference_x(item));
}
void cJSON_AddItemReferenceToObject_x(cJSON_x *object, const char *string,
                cJSON_x *item)
{
    cJSON_AddItemToObject_x(object, string, create_reference_x(item));
}

cJSON_x *cJSON_DetachItemFromArray_x(cJSON_x *array, int which)
{
    cJSON_x *c = array->child;
    while (c && which > 0)
        c = c->next, which--;
    if (!c)
        return 0;
    if (c->prev)
        c->prev->next = c->next;
    if (c->next)
        c->next->prev = c->prev;
    if (c == array->child)
        array->child = c->next;
    c->prev = c->next = 0;
    return c;
}
void cJSON_DeleteItemFromArray_x(cJSON_x *array, int which)
{
    cJSON_Delete_x(cJSON_DetachItemFromArray_x(array, which));
}
cJSON_x *cJSON_DetachItemFromObject_x(cJSON_x *object, const char *string)
{
    int i = 0;
    cJSON_x *c = object->child;
    while (c && cJSON_strcasecmp_x(c->string, string))
        i++, c = c->next;
    if (c)
        return cJSON_DetachItemFromArray_x(object, i);
    return 0;
}
void cJSON_DeleteItemFromObject_x(cJSON_x *object, const char *string)
{
    cJSON_Delete_x(cJSON_DetachItemFromObject_x(object, string));
}

/* Replace array/object items with new ones. */
void cJSON_ReplaceItemInArray_x(cJSON_x *array, int which, cJSON_x *newitem)
{
    cJSON_x *c = array->child;
    while (c && which > 0)
        c = c->next, which--;
    if (!c)
        return;
    newitem->next = c->next;
    newitem->prev = c->prev;
    if (newitem->next)
        newitem->next->prev = newitem;
    if (c == array->child)
        array->child = newitem;
    else
        newitem->prev->next = newitem;
    c->next = c->prev = 0;
    cJSON_Delete_x(c);
}
void cJSON_ReplaceItemInObject_x(cJSON_x *object, const char *string,
                cJSON_x *newitem)
{
    int i = 0;
    cJSON_x *c = object->child;
    while (c && cJSON_strcasecmp_x(c->string, string))
        i++, c = c->next;
    if (c)
    {
        newitem->string = cJSON_strdup_x(string);
        cJSON_ReplaceItemInArray_x(object, i, newitem);
    }
}

/* Create basic types: */
cJSON_x *cJSON_CreateNull_x()
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
        item->type = cJSON_x_NULL;
    return item;
}
cJSON_x *cJSON_CreateTrue_x()
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
        item->type = cJSON_x_True;
    return item;
}
cJSON_x *cJSON_CreateFalse_x()
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
        item->type = cJSON_x_False;
    return item;
}
cJSON_x *cJSON_CreateBool_x(int b)
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
        item->type = b ? cJSON_x_True : cJSON_x_False;
    return item;
}
cJSON_x *cJSON_CreateDouble_x(double num, int sign)
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
    {
        item->type = cJSON_x_Double;
        item->valuedouble = num;
        item->valueint = (uint64)num;
        item->sign = sign;
    }
    return item;
}
cJSON_x *cJSON_CreateInt_x(uint64 num, int sign)
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
    {
        item->type = cJSON_x_Int;
        item->valuedouble = (double)num;
        item->valueint = (uint64)num;
        item->sign = sign;
    }
    return item;
}
cJSON_x *cJSON_CreateString_x(const char *string)
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
    {
        item->type = cJSON_x_String;
        item->valuestring = cJSON_strdup_x(string);
    }
    return item;
}
cJSON_x *cJSON_CreateArray_x()
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
        item->type = cJSON_x_Array;
    return item;
}
cJSON_x *cJSON_CreateObject_x()
{
    cJSON_x *item = cJSON_New_Item_x();
    if (item)
        item->type = cJSON_x_Object;
    return item;
}

/* Create Arrays: */
cJSON_x *cJSON_CreateIntArray_x(int *numbers, int sign, int count)
{
    int i;
    cJSON_x *n = 0, *p = 0, *a = cJSON_CreateArray_x();
    for (i = 0; a && i < count; i++)
    {
        n = cJSON_CreateDouble_x((long double)((unsigned int)numbers[i]), sign);
        if (!i)
            a->child = n;
        else
            suffix_object_x(p, n);
        p = n;
    }
    return a;
}
cJSON_x *cJSON_CreateFloatArray_x(float *numbers, int count)
{
    int i;
    cJSON_x *n = 0, *p = 0, *a = cJSON_CreateArray_x();
    for (i = 0; a && i < count; i++)
    {
        n = cJSON_CreateDouble_x((long double)numbers[i], -1);
        if (!i)
            a->child = n;
        else
            suffix_object_x(p, n);
        p = n;
    }
    return a;
}
cJSON_x *cJSON_CreateDoubleArray_x(double *numbers, int count)
{
    int i;
    cJSON_x *n = 0, *p = 0, *a = cJSON_CreateArray_x();
    for (i = 0; a && i < count; i++)
    {
        n = cJSON_CreateDouble_x((long double)numbers[i], -1);
        if (!i)
            a->child = n;
        else
            suffix_object_x(p, n);
        p = n;
    }
    return a;
}
cJSON_x *cJSON_CreateStringArray_x(const char **strings, int count)
{
    int i;
    cJSON_x *n = 0, *p = 0, *a = cJSON_CreateArray_x();
    for (i = 0; a && i < count; i++)
    {
        n = cJSON_CreateString_x(strings[i]);
        if (!i)
            a->child = n;
        else
            suffix_object_x(p, n);
        p = n;
    }
    return a;
}

