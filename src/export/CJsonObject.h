/*******************************************************************************
    Project:  neb
    @file     CJsonObject.hpp
    @brief    Json
    @author   bwarliao
    @date:    2014-7-16
    @note
    Modify history:
 ******************************************************************************/

#ifndef CJSONOBJECT_HPP_
#define CJSONOBJECT_HPP_

#include <stdio.h>
#include <stddef.h>
#if defined(__APPLE__)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <string>
#include <list>
#include <map>
#include <type_traits>
#include "com_base.h"
#include <unordered_map>
#ifdef __cplusplus
extern "C" {
#endif
#include "cJSON.h"
#ifdef __cplusplus
}
#endif


class COM_EXPORT CJsonObject
{
public:     // method of ordinary json object or json array
    CJsonObject();
    CJsonObject(const char* strJson);
    CJsonObject(const std::string& strJson);
    CJsonObject(const CJsonObject* pJsonObject);
    CJsonObject(const CJsonObject& oJsonObject);
    CJsonObject(CJsonObject&& oJsonObject);
    virtual ~CJsonObject();

    CJsonObject& operator=(const CJsonObject& oJsonObject);
    CJsonObject& operator=(CJsonObject&& oJsonObject);
    bool operator==(const CJsonObject& oJsonObject) const;
    bool Parse(const char* strJson);
    bool Parse(const std::string& strJson);
    void Clear();
    bool IsEmpty() const;
    bool IsArray() const;
    std::string ToString() const;
    std::string ToFormattedString() const;
    const std::string& GetErrMsg() const
    {
        return(m_strErrMsg);
    }
    void EnableEmptyString();
    void DisableEmptyString();

public:     // method of ordinary json object
    bool AddEmptySubObject(const std::string& strKey);
    bool AddEmptySubArray(const std::string& strKey);
    bool GetKey(std::string& strKey) const;
    void ResetTraversing() const;
    CJsonObject& operator[](const std::string& strKey);
    std::string operator()(const std::string& strKey) const;
    bool KeyExist(const std::string& strKey) const;
    bool Get(const std::string& strKey, CJsonObject& oJsonObject) const;
    bool Get(const std::string& strKey, char* charArray) const;
    bool Get(const std::string& strKey, const char* charArray) const;
    bool Get(const std::string& strKey, CPPBytes& bytes) const;
    bool Get(const std::string& strKey, std::string& strValue) const;
    bool Get(const std::string& strKey, char& iValue) const;
    bool Get(const std::string& strKey, int8& iValue) const;
    bool Get(const std::string& strKey, uint8& uiValue) const;
    bool Get(const std::string& strKey, int16& iValue) const;
    bool Get(const std::string& strKey, uint16& uiValue) const;
    bool Get(const std::string& strKey, int32& iValue) const;
    bool Get(const std::string& strKey, uint32& uiValue) const;
    bool Get(const std::string& strKey, int64& llValue) const;
    bool Get(const std::string& strKey, uint64& ullValue) const;
    bool Get(const std::string& strKey, bool& bValue) const;
    bool Get(const std::string& strKey, float& fValue) const;
    bool Get(const std::string& strKey, double& dValue) const;
    template<class T>
    bool Get(const std::string& strKey, std::vector<T>& list) const
    {
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        for(int i = 0; i < json_sub.GetArraySize(); i++)
        {
            T val;
            if(json_sub.Get(i, val))
            {
                list.push_back(val);
            }
        }
        return true;
    }
    template<class T>
    bool Get(const std::string& strKey, std::set<T>& list) const
    {
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        for(int i = 0; i < json_sub.GetArraySize(); i++)
        {
            T val;
            if(json_sub.Get(i, val))
            {
                list.insert(val);
            }
        }
        return true;
    }
    template<class T>
    bool Get(const std::string& strKey, std::deque<T>& list) const
    {
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        for(int i = 0; i < json_sub.GetArraySize(); i++)
        {
            T val;
            if(json_sub.Get(i, val))
            {
                list.push_back(val);
            }
        }
        return true;
    }
    template<class T>
    bool Get(const std::string& strKey, std::queue<T>& list) const
    {
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        for(int i = 0; i < json_sub.GetArraySize(); i++)
        {
            T val;
            if(json_sub.Get(i, val))
            {
                list.push(val);
            }
        }
        return true;
    }
    template<class T>
    bool Get(const std::string& strKey, std::list<T>& list) const
    {
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        for(int i = 0; i < json_sub.GetArraySize(); i++)
        {
            T val;
            if(json_sub.Get(i, val))
            {
                list.push_back(val);
            }
        }
        return true;
    }
    template<class T>
    bool Get(const std::string& strKey, std::map<std::string, T>& list) const
    {
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        std::string key;
        json_sub.ResetTraversing();
        while(json_sub.GetKey(key))
        {
            T val;
            if(json_sub.Get(key, val))
            {
                list[key] = val;
            }
        }
        return true;
    }
    template<class T>
    bool Get(const std::string& strKey, std::unordered_map<std::string, T>& list) const
    {
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        std::string key;
        json_sub.ResetTraversing();
        while(json_sub.GetKey(key))
        {
            T val;
            if(json_sub.Get(key, val))
            {
                list[key] = val;
            }
        }
        return true;
    }
    template<typename T, void (T::*)(const char*) = &T::fromJson>
    bool Get(const std::string& strKey, T& value)
    {
        AddEmptySubObject(strKey);
        CJsonObject json_sub;
        if(Get(strKey, json_sub) == false)
        {
            return false;
        }
        value.fromJson(json_sub.ToString().c_str());
        return true;
    }
    int GetValueType(const std::string& strKey) const;
    bool IsNull(const std::string& strKey) const;
    bool Add(const std::string& strKey, const CJsonObject& oJsonObject);
    bool Add(const std::string& strKey, CJsonObject&& oJsonObject);
    bool Add(const std::string& strKey, const char* charArray);
    bool Add(const std::string& strKey, const std::string& strValue);
    bool Add(const std::string& strKey, char cValue);
    bool Add(const std::string& strKey, int8 iValue);
    bool Add(const std::string& strKey, uint8 uiValue);
    bool Add(const std::string& strKey, int16 iValue);
    bool Add(const std::string& strKey, uint16 uiValue);
    bool Add(const std::string& strKey, int32 iValue);
    bool Add(const std::string& strKey, uint32 uiValue);
    bool Add(const std::string& strKey, int64 llValue);
    bool Add(const std::string& strKey, uint64 ullValue);
    bool Add(const std::string& strKey, const bool bValue);
    bool Add(const std::string& strKey, float fValue);
    bool Add(const std::string& strKey, double dValue);
    bool Add(const std::string& strKey, const CPPBytes& bytes);
    template<class T>
    bool Add(const std::string& strKey, const std::vector<T>& list)
    {
        AddEmptySubArray(strKey);
        for(auto it = list.begin(); it != list.end(); it++)
        {
            (*this)[strKey].Add(*it);
        }
        return true;
    }
    template<class T>
    bool Add(const std::string& strKey, const std::set<T>& list)
    {
        AddEmptySubArray(strKey);
        for(auto it = list.begin(); it != list.end(); it++)
        {
            (*this)[strKey].Add(*it);
        }
        return true;
    }
    template<class T>
    bool Add(const std::string& strKey, const std::deque<T>& list)
    {
        AddEmptySubArray(strKey);
        for(auto it = list.begin(); it != list.end(); it++)
        {
            (*this)[strKey].Add(*it);
        }
        return true;
    }
    template<class T>
    bool Add(const std::string& strKey, const std::queue<T>& list)
    {
        std::queue<T> tmp = list;
        AddEmptySubArray(strKey);
        while(tmp.size() > 0)
        {
            (*this)[strKey].Add(tmp.front());
            tmp.pop();
        }
        return true;
    }
    template<class T>
    bool Add(const std::string& strKey, const std::list<T>& list)
    {
        AddEmptySubArray(strKey);
        for(auto it = list.begin(); it != list.end(); it++)
        {
            (*this)[strKey].Add(*it);
        }
        return true;
    }
    template<class T>
    bool Add(const std::string& strKey, const std::map<std::string, T>& list)
    {
        AddEmptySubObject(strKey);
        for(auto it = list.begin(); it != list.end(); it++)
        {
            (*this)[strKey].Add(it->first, it->second);
        }
        return true;
    }
    template<class T>
    bool Add(const std::string& strKey, const std::unordered_map<std::string, T>& list)
    {
        AddEmptySubObject(strKey);
        for(auto it = list.begin(); it != list.end(); it++)
        {
            (*this)[strKey].Add(it->first, it->second);
        }
        return true;
    }
    template<typename T, std::string(T::*)(bool) const = &T::toJson>
    bool Add(const std::string& strKey, const T& value)
    {
        CJsonObject json_sub(value.toJson());
        return Add(strKey, json_sub);
    }
    bool AddNull(const std::string& strKey);    // add null like this:   "key":null
    bool Delete(const std::string& strKey);
    bool Replace(const std::string& strKey, const CJsonObject& oJsonObject);
    bool Replace(const std::string& strKey, CJsonObject&& oJsonObject);
    bool Replace(const std::string& strKey, const char* strValue);
    bool Replace(const std::string& strKey, const std::string& strValue);
    bool Replace(const std::string& strKey, int32 iValue);
    bool Replace(const std::string& strKey, uint32 uiValue);
    bool Replace(const std::string& strKey, int64 llValue);
    bool Replace(const std::string& strKey, uint64 ullValue);
    bool Replace(const std::string& strKey, const bool bValue);
    bool Replace(const std::string& strKey, float fValue);
    bool Replace(const std::string& strKey, double dValue);
    bool ReplaceWithNull(const std::string& strKey);    // replace value with null
    template <typename T>
    bool ReplaceAdd(const std::string& strKey, T&& value)
    {
        if(KeyExist(strKey))
        {
            return(Replace(strKey, std::forward<T>(value)));
        }
        return(Add(strKey, std::forward<T>(value)));
    }
    template<typename T, std::string (T::*)(bool) const = &T::toJson>
    bool ReplaceAdd(const std::string& strKey, T& value)
    {
        CJsonObject json_sub(value.toJson());
        return ReplaceAdd(strKey, json_sub);
    }

public:     // method of json array
    int GetArraySize() const;
    CJsonObject& operator[](unsigned int uiWhich);
    std::string operator()(unsigned int uiWhich) const;
    bool Get(int iWhich, CJsonObject& oJsonObject) const;
    bool Get(int iWhich, std::string& strValue) const;
    bool Get(int iWhich, int32& iValue) const;
    bool Get(int iWhich, uint32& uiValue) const;
    bool Get(int iWhich, int64& llValue) const;
    bool Get(int iWhich, uint64& ullValue) const;
    bool Get(int iWhich, bool& bValue) const;
    bool Get(int iWhich, float& fValue) const;
    bool Get(int iWhich, double& dValue) const;
    template<typename T, void (T::*)(const char*) = &T::fromJson>
    bool Get(int iWhich, T& value) const
    {
        CJsonObject json_sub;
        if(Get(iWhich, json_sub) == false)
        {
            return false;
        }
        value.fromJson(json_sub.ToString().c_str());
        return true;
    }
    int GetValueType(int iWhich) const;
    bool IsNull(int iWhich) const;
    bool Add(const CJsonObject& oJsonObject);
    bool Add(CJsonObject&& oJsonObject);
    bool Add(const std::string& strValue);
    bool Add(const char* strValue);
    bool Add(int32 iValue);
    bool Add(uint32 uiValue);
    bool Add(int64 llValue);
    bool Add(uint64 ullValue);
    bool Add(const bool bValue);
    bool Add(float fValue);
    bool Add(double dValue);
    bool Add(const CPPBytes& bytes);
    template<typename T, std::string (T::*)(bool) const = &T::toJson>
    bool Add(const T& value)
    {
        CJsonObject json_sub(value.toJson());
        return Add(json_sub);
    }
    bool AddNull();   // add a null value
    bool AddAsFirst(const CJsonObject& oJsonObject);
    bool AddAsFirst(CJsonObject&& oJsonObject);
    bool AddAsFirst(const std::string& strValue);
    bool AddAsFirst(int32 iValue);
    bool AddAsFirst(uint32 uiValue);
    bool AddAsFirst(int64 llValue);
    bool AddAsFirst(uint64 ullValue);
    bool AddAsFirst(const bool bValue);
    bool AddAsFirst(float fValue);
    bool AddAsFirst(double dValue);
    template<typename T, std::string (T::*)(bool) const = &T::toJson>
    bool AddAsFirst(const T& value)
    {
        CJsonObject json_sub(value.toJson());
        return AddAsFirst(json_sub);
    }
    bool AddNullAsFirst();     // add a null value
    bool Delete(int iWhich);
    bool Replace(int iWhich, const CJsonObject& oJsonObject);
    bool Replace(int iWhich, CJsonObject&& oJsonObject);
    bool Replace(int iWhich, const std::string& strValue);
    bool Replace(int iWhich, int32 iValue);
    bool Replace(int iWhich, uint32 uiValue);
    bool Replace(int iWhich, int64 llValue);
    bool Replace(int iWhich, uint64 ullValue);
    bool Replace(int iWhich, const bool bValue);
    bool Replace(int iWhich, float fValue);
    bool Replace(int iWhich, double dValue);
    bool ReplaceWithNull(int iWhich);      // replace with a null value
    template<typename T, std::string (T::*)(bool) const = &T::toJson>
    bool Replace(int iWhich, T& value)
    {
        CJsonObject json_sub(value.toJson());
        return Replace(iWhich, json_sub);
    }

private:
    CJsonObject(cJSON* pJsonData);

private:
    cJSON* m_pJsonData;
    cJSON* m_pExternJsonDataRef;
    mutable cJSON* m_pKeyTravers;
    const char* mc_pError;
    std::string m_strErrMsg;
    std::unordered_map<unsigned int, CJsonObject*> m_mapJsonArrayRef;
    std::unordered_map<std::string, CJsonObject*>::iterator m_object_iter;
    std::unordered_map<std::string, CJsonObject*> m_mapJsonObjectRef;
    std::unordered_map<unsigned int, CJsonObject*>::iterator m_array_iter;
};

#endif /* CJSONHELPER_HPP_ */
