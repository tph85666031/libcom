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

#ifndef cJSON__h
#define cJSON__h

#include <stdint.h>
typedef int int32;
typedef unsigned int uint32;
typedef signed long long   int64;
typedef unsigned long long uint64;

#ifdef __cplusplus
extern "C"
{
#endif

/* cJSON_x Types: */
#define cJSON_x_False 0
#define cJSON_x_True 1
#define cJSON_x_NULL 2
#define cJSON_x_Int 3
#define cJSON_x_Double 4
#define cJSON_x_String 5
#define cJSON_x_Array 6
#define cJSON_x_Object 7

#define cJSON_x_IsReference 256

/* The cJSON_x structure: */
typedef struct cJSON_x
{
    struct cJSON_x *next, *prev; /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON_x *child; /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

    int type; /* The type of the item, as above. */

    char *valuestring; /* The item's string, if type==cJSON_x_String */
    uint64 valueint; /* The item's number, if type==cJSON_Number */
    double valuedouble; /* The item's number, if type==cJSON_Number */
    int sign;   /* sign of valueint, 1(unsigned), -1(signed) */

    char *string; /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} cJSON_x;

typedef struct cJSON_Hooks_x
{
    void *(*malloc_fn)(size_t sz);
    void (*free_fn)(void *ptr);
} cJSON_Hooks_x;

extern const char *cJSON_GetErrorPtr_x(void);

/* Supply malloc, realloc and free functions to cJSON_x */
extern void cJSON_InitHooks_x(cJSON_Hooks_x* hooks);

/* Supply a block of JSON, and this returns a cJSON_x object you can interrogate. Call cJSON_Delete_x when finished. */
extern cJSON_x *cJSON_Parse_x(const char *value);
/* Render a cJSON_x entity to text for transfer/storage. Free the char* when finished. */
extern char *cJSON_Print_x(cJSON_x *item);
/* Render a cJSON_x entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char *cJSON_PrintUnformatted_x(cJSON_x *item);
/* Delete a cJSON_x entity and all subentities. */
extern void cJSON_Delete_x(cJSON_x *c);

/* Returns the number of items in an array (or object). */
extern int cJSON_GetArraySize_x(cJSON_x *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern cJSON_x *cJSON_GetArrayItem_x(cJSON_x *array, int item);
/* Get item "string" from object. Case insensitive. */
extern cJSON_x *cJSON_GetObjectItem_x(cJSON_x *object, const char *string);

/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse_x() returns 0. 0 when cJSON_Parse_x() succeeds. */
extern const char *cJSON_GetErrorPtr(void);

/* These calls create a cJSON_x item of the appropriate type. */
extern cJSON_x *cJSON_CreateNull_x(void);
extern cJSON_x *cJSON_CreateTrue_x(void);
extern cJSON_x *cJSON_CreateFalse_x(void);
extern cJSON_x *cJSON_CreateBool_x(int b);
extern cJSON_x *cJSON_CreateDouble_x(double num, int sign);
extern cJSON_x *cJSON_CreateInt_x(uint64 num, int sign);
extern cJSON_x *cJSON_CreateString_x(const char *string);
extern cJSON_x *cJSON_CreateArray_x(void);
extern cJSON_x *cJSON_CreateObject_x(void);

/* These utilities create an Array of count items. */
extern cJSON_x *cJSON_CreateIntArray_x(int *numbers, int sign, int count);
extern cJSON_x *cJSON_CreateFloatArray_x(float *numbers, int count);
extern cJSON_x *cJSON_CreateDoubleArray_x(double *numbers, int count);
extern cJSON_x *cJSON_CreateStringArray_x(const char **strings, int count);

/* Append item to the specified array/object. */
extern void cJSON_AddItemToArray_x(cJSON_x *array, cJSON_x *item);
extern void cJSON_AddItemToArrayHead_x(cJSON_x *array, cJSON_x *item);    /* add by Bwar on 2015-01-28 */
extern void cJSON_AddItemToObject_x(cJSON_x *object, const char *string,
                cJSON_x *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON_x to a new cJSON_x, but don't want to corrupt your existing cJSON_x. */
extern void cJSON_AddItemReferenceToArray_x(cJSON_x *array, cJSON_x *item);
extern void cJSON_AddItemReferenceToObject_x(cJSON_x *object, const char *string,
                cJSON_x *item);

/* Remove/Detatch items from Arrays/Objects. */
extern cJSON_x *cJSON_DetachItemFromArray_x(cJSON_x *array, int which);
extern void cJSON_DeleteItemFromArray_x(cJSON_x *array, int which);
extern cJSON_x *cJSON_DetachItemFromObject_x(cJSON_x *object, const char *string);
extern void cJSON_DeleteItemFromObject_x(cJSON_x *object, const char *string);

/* Update array items. */
extern void cJSON_ReplaceItemInArray_x(cJSON_x *array, int which, cJSON_x *newitem);
extern void cJSON_ReplaceItemInObject_x(cJSON_x *object, const char *string,
                cJSON_x *newitem);

#define cJSON_AddNullToObject_x(object,name)	cJSON_AddItemToObject_x(object, name, cJSON_CreateNull_x())
#define cJSON_AddTrueToObject_x(object,name)	cJSON_AddItemToObject_x(object, name, cJSON_CreateTrue_x())
#define cJSON_AddFalseToObject_x(object,name)		cJSON_AddItemToObject_x(object, name, cJSON_CreateFalse_x())
//#define cJSON_AddNumberToObject_x(object,name,n)	cJSON_AddItemToObject_x(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject_x(object,name,s)	cJSON_AddItemToObject_x(object, name, cJSON_CreateString_x(s))


#ifdef __cplusplus
}
#endif

#endif
