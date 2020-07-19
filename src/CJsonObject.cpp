/*******************************************************************************
 * Project:  neb
 * @file     CJsonObject.cpp
 * @brief
 * @author   bwarliao
 * @date:    2014-7-16
 * @note
 * Modify history:
 ******************************************************************************/

#include "CJsonObject.h"

CJsonObject::CJsonObject()
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    // m_pJsonData = cJSON_CreateObject_x();
}

CJsonObject::CJsonObject(const std::string& strJson)
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    Parse(strJson);
}

CJsonObject::CJsonObject(const CJsonObject* pJsonObject)
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    if(pJsonObject)
    {
        Parse(pJsonObject->ToString());
    }
}

CJsonObject::CJsonObject(const CJsonObject& oJsonObject)
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    Parse(oJsonObject.ToString());
}

CJsonObject::~CJsonObject()
{
    Clear();
}

CJsonObject& CJsonObject::operator=(const CJsonObject& oJsonObject)
{
    Parse(oJsonObject.ToString().c_str());
    return(*this);
}

bool CJsonObject::operator==(const CJsonObject& oJsonObject) const
{
    return(this->ToString() == oJsonObject.ToString());
}

bool CJsonObject::AddEmptySubObject(const std::string& strKey)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateObject_x();
    if(pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("create sub empty object error!");
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::AddEmptySubArray(const std::string& strKey)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateArray_x();
    if(pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("create sub empty array error!");
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::GetKey(std::string& strKey)
{
    if(IsArray())
    {
        return(false);
    }
    if(m_listKeys.size() == 0)
    {
        cJSON_x* pFocusData = NULL;
        if(m_pJsonData != NULL)
        {
            pFocusData = m_pJsonData;
        }
        else if(m_pExternJsonDataRef != NULL)
        {
            pFocusData = m_pExternJsonDataRef;
        }
        else
        {
            return(false);
        }

        cJSON_x *c = pFocusData->child;
        while(c)
        {
            m_listKeys.push_back(c->string);
            c = c->next;
        }
        m_itKey = m_listKeys.begin();
    }

    if(m_itKey == m_listKeys.end())
    {
        strKey = "";
        m_itKey = m_listKeys.begin();
        return(false);
    }
    else
    {
        strKey = *m_itKey;
        ++m_itKey;
        return(true);
    }
}

void CJsonObject::ResetTraversing()
{
    m_itKey = m_listKeys.begin();
}

CJsonObject& CJsonObject::operator[](const std::string& strKey)
{
    std::map<std::string, CJsonObject*>::iterator iter;
    iter = m_mapJsonObjectRef.find(strKey);
    if(iter == m_mapJsonObjectRef.end())
    {
        cJSON_x* pJsonStruct = NULL;
        if(m_pJsonData != NULL)
        {
            if(m_pJsonData->type == cJSON_x_Object)
            {
                pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
            }
        }
        else if(m_pExternJsonDataRef != NULL)
        {
            if(m_pExternJsonDataRef->type == cJSON_x_Object)
            {
                pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
            }
        }
        if(pJsonStruct == NULL)
        {
            CJsonObject* pJsonObject = new CJsonObject();
            m_mapJsonObjectRef.insert(std::pair<std::string, CJsonObject*>(strKey, pJsonObject));
            return(*pJsonObject);
        }
        else
        {
            CJsonObject* pJsonObject = new CJsonObject(pJsonStruct);
            m_mapJsonObjectRef.insert(std::pair<std::string, CJsonObject*>(strKey, pJsonObject));
            return(*pJsonObject);
        }
    }
    else
    {
        return(*(iter->second));
    }
}

CJsonObject& CJsonObject::operator[](unsigned int uiWhich)
{
    std::map<unsigned int, CJsonObject*>::iterator iter;
    iter = m_mapJsonArrayRef.find(uiWhich);
    if(iter == m_mapJsonArrayRef.end())
    {
        cJSON_x* pJsonStruct = NULL;
        if(m_pJsonData != NULL)
        {
            if(m_pJsonData->type == cJSON_x_Array)
            {
                pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, uiWhich);
            }
        }
        else if(m_pExternJsonDataRef != NULL)
        {
            if(m_pExternJsonDataRef->type == cJSON_x_Array)
            {
                pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, uiWhich);
            }
        }
        if(pJsonStruct == NULL)
        {
            CJsonObject* pJsonObject = new CJsonObject();
            m_mapJsonArrayRef.insert(std::pair<unsigned int, CJsonObject*>(uiWhich, pJsonObject));
            return(*pJsonObject);
        }
        else
        {
            CJsonObject* pJsonObject = new CJsonObject(pJsonStruct);
            m_mapJsonArrayRef.insert(std::pair<unsigned int, CJsonObject*>(uiWhich, pJsonObject));
            return(*pJsonObject);
        }
    }
    else
    {
        return(*(iter->second));
    }
}

std::string CJsonObject::operator()(const std::string& strKey) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(std::string(""));
    }
    if(pJsonStruct->type == cJSON_x_String)
    {
        return(pJsonStruct->valuestring);
    }
    else if(pJsonStruct->type == cJSON_x_Int)
    {
        char szNumber[128] = {0};
        if(pJsonStruct->sign == -1)
        {
            if((int64)pJsonStruct->valueint <= (int64)INT_MAX && (int64)pJsonStruct->valueint >= (int64)INT_MIN)
            {
                snprintf(szNumber, sizeof(szNumber), "%d", (int32)pJsonStruct->valueint);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%lld", (int64)pJsonStruct->valueint);
            }
        }
        else
        {
            if(pJsonStruct->valueint <= (uint64)UINT_MAX)
            {
                snprintf(szNumber, sizeof(szNumber), "%u", (uint32)pJsonStruct->valueint);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%llu", pJsonStruct->valueint);
            }
        }
        return(std::string(szNumber));
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        char szNumber[128] = {0};
        if(fabs(pJsonStruct->valuedouble) < 1.0e-6 || fabs(pJsonStruct->valuedouble) > 1.0e9)
        {
            snprintf(szNumber, sizeof(szNumber), "%e", pJsonStruct->valuedouble);
        }
        else
        {
            snprintf(szNumber, sizeof(szNumber), "%f", pJsonStruct->valuedouble);
        }
    }
    else if(pJsonStruct->type == cJSON_x_False)
    {
        return(std::string("false"));
    }
    else if(pJsonStruct->type == cJSON_x_True)
    {
        return(std::string("true"));
    }
    return(std::string(""));
}

std::string CJsonObject::operator()(unsigned int uiWhich) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, uiWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, uiWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(std::string(""));
    }
    if(pJsonStruct->type == cJSON_x_String)
    {
        return(pJsonStruct->valuestring);
    }
    else if(pJsonStruct->type == cJSON_x_Int)
    {
        char szNumber[128] = {0};
        if(pJsonStruct->sign == -1)
        {
            if((int64)pJsonStruct->valueint <= (int64)INT_MAX && (int64)pJsonStruct->valueint >= (int64)INT_MIN)
            {
                snprintf(szNumber, sizeof(szNumber), "%d", (int32)pJsonStruct->valueint);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%lld", (int64)pJsonStruct->valueint);
            }
        }
        else
        {
            if(pJsonStruct->valueint <= (uint64)UINT_MAX)
            {
                snprintf(szNumber, sizeof(szNumber), "%u", (uint32)pJsonStruct->valueint);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%llu", pJsonStruct->valueint);
            }
        }
        return(std::string(szNumber));
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        char szNumber[128] = {0};
        if(fabs(pJsonStruct->valuedouble) < 1.0e-6 || fabs(pJsonStruct->valuedouble) > 1.0e9)
        {
            snprintf(szNumber, sizeof(szNumber), "%e", pJsonStruct->valuedouble);
        }
        else
        {
            snprintf(szNumber, sizeof(szNumber), "%f", pJsonStruct->valuedouble);
        }
    }
    else if(pJsonStruct->type == cJSON_x_False)
    {
        return(std::string("false"));
    }
    else if(pJsonStruct->type == cJSON_x_True)
    {
        return(std::string("true"));
    }
    return(std::string(""));
}

bool CJsonObject::Parse(const std::string& strJson)
{
    Clear();
    m_pJsonData = cJSON_Parse_x(strJson.c_str());
    if(m_pJsonData == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr_x();
        return(false);
    }
    return(true);
}

void CJsonObject::Clear()
{
    m_pExternJsonDataRef = NULL;
    if(m_pJsonData != NULL)
    {
        cJSON_Delete_x(m_pJsonData);
        m_pJsonData = NULL;
    }
    for(std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.begin();
            iter != m_mapJsonArrayRef.end(); ++iter)
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
    }
    m_mapJsonArrayRef.clear();
    for(std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.begin();
            iter != m_mapJsonObjectRef.end(); ++iter)
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
    }
    m_mapJsonObjectRef.clear();
    m_listKeys.clear();
}

bool CJsonObject::IsEmpty() const
{
    if(m_pJsonData != NULL)
    {
        return(false);
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::IsArray() const
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }

    if(pFocusData == NULL)
    {
        return(false);
    }

    if(pFocusData->type == cJSON_x_Array)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

std::string CJsonObject::ToString() const
{
    char* pJsonString = NULL;
    std::string strJsonData = "";
    if(m_pJsonData != NULL)
    {
        pJsonString = cJSON_PrintUnformatted_x(m_pJsonData);
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pJsonString = cJSON_PrintUnformatted_x(m_pExternJsonDataRef);
    }
    if(pJsonString != NULL)
    {
        strJsonData = pJsonString;
        free(pJsonString);
    }
    return(strJsonData);
}

std::string CJsonObject::ToFormattedString() const
{
    char* pJsonString = NULL;
    std::string strJsonData = "";
    if(m_pJsonData != NULL)
    {
        pJsonString = cJSON_Print_x(m_pJsonData);
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pJsonString = cJSON_Print_x(m_pExternJsonDataRef);
    }
    if(pJsonString != NULL)
    {
        strJsonData = pJsonString;
        free(pJsonString);
    }
    return(strJsonData);
}


bool CJsonObject::Get(const std::string& strKey, CJsonObject& oJsonObject) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    char* pJsonString = cJSON_Print_x(pJsonStruct);
    std::string strJsonData = pJsonString;
    free(pJsonString);
    if(oJsonObject.Parse(strJsonData))
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

bool CJsonObject::Get(const std::string& strKey, std::string& strValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type != cJSON_x_String)
    {
        return(false);
    }
    strValue = pJsonStruct->valuestring;
    return(true);
}

bool CJsonObject::Get(const std::string& strKey, int32& iValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        iValue = (int32)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        iValue = (int32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(const std::string& strKey, uint32& uiValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        uiValue = (uint32)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        uiValue = (uint32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(const std::string& strKey, int64& llValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        llValue = (int64)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        llValue = (int64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(const std::string& strKey, uint64& ullValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        ullValue = (uint64)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        ullValue = (uint64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(const std::string& strKey, bool& bValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type > cJSON_x_True)
    {
        return(false);
    }
    bValue = pJsonStruct->type;
    return(true);
}

bool CJsonObject::Get(const std::string& strKey, float& fValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Double)
    {
        fValue = (float)(pJsonStruct->valuedouble);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Int)
    {
        fValue = (float)(pJsonStruct->valueint);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(const std::string& strKey, double& dValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Double)
    {
        dValue = pJsonStruct->valuedouble;
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Int)
    {
        dValue = (double)(pJsonStruct->valueint);
        return(true);
    }
    return(false);
}

bool CJsonObject::IsNull(const std::string& strKey) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pJsonData, strKey.c_str());
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Object)
        {
            pJsonStruct = cJSON_GetObjectItem_x(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type != cJSON_x_NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, const CJsonObject& oJsonObject)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_Parse_x(oJsonObject.ToString().c_str());
    if(pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr_x();
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, const std::string& strValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateString_x(strValue.c_str());
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, int32 iValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)iValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, uint32 uiValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)uiValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, int64 llValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)llValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, uint64 ullValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x(ullValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, bool bValue, bool bValueAgain)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateBool_x(bValue);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, float fValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)fValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Add(const std::string& strKey, double dValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)dValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::AddNull(const std::string& strKey)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateNull_x();
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Delete(const std::string& strKey)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_DeleteItemFromObject_x(pFocusData, strKey.c_str());
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_listKeys.clear();
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, const CJsonObject& oJsonObject)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_Parse_x(oJsonObject.ToString().c_str());
    if(pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr_x();
        return(false);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, const std::string& strValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateString_x(strValue.c_str());
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, int32 iValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)iValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, uint32 uiValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)uiValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, int64 llValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)llValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, uint64 ullValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)ullValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, bool bValue, bool bValueAgain)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateBool_x(bValue);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, float fValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)fValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(const std::string& strKey, double dValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)dValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::ReplaceWithNull(const std::string& strKey)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateNull_x();
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<std::string, CJsonObject*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if(iter != m_mapJsonObjectRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    cJSON_ReplaceItemInObject_x(pFocusData, strKey.c_str(), pJsonStruct);
    if(cJSON_GetObjectItem_x(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

int CJsonObject::GetArraySize()
{
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            return(cJSON_GetArraySize_x(m_pJsonData));
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            return(cJSON_GetArraySize_x(m_pExternJsonDataRef));
        }
    }
    return(0);
}

bool CJsonObject::Get(int iWhich, CJsonObject& oJsonObject) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    char* pJsonString = cJSON_Print_x(pJsonStruct);
    std::string strJsonData = pJsonString;
    free(pJsonString);
    if(oJsonObject.Parse(strJsonData))
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

bool CJsonObject::Get(int iWhich, std::string& strValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type != cJSON_x_String)
    {
        return(false);
    }
    strValue = pJsonStruct->valuestring;
    return(true);
}

bool CJsonObject::Get(int iWhich, int32& iValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        iValue = (int32)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        iValue = (int32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(int iWhich, uint32& uiValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        uiValue = (uint32)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        uiValue = (uint32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(int iWhich, int64& llValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        llValue = (int64)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        llValue = (int64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(int iWhich, uint64& ullValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Int)
    {
        ullValue = (uint64)(pJsonStruct->valueint);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Double)
    {
        ullValue = (uint64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(int iWhich, bool& bValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type > cJSON_x_True)
    {
        return(false);
    }
    bValue = pJsonStruct->type;
    return(true);
}

bool CJsonObject::Get(int iWhich, float& fValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Double)
    {
        fValue = (float)(pJsonStruct->valuedouble);
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Int)
    {
        fValue = (float)(pJsonStruct->valueint);
        return(true);
    }
    return(false);
}

bool CJsonObject::Get(int iWhich, double& dValue) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type == cJSON_x_Double)
    {
        dValue = pJsonStruct->valuedouble;
        return(true);
    }
    else if(pJsonStruct->type == cJSON_x_Int)
    {
        dValue = (double)(pJsonStruct->valueint);
        return(true);
    }
    return(false);
}

bool CJsonObject::IsNull(int iWhich) const
{
    cJSON_x* pJsonStruct = NULL;
    if(m_pJsonData != NULL)
    {
        if(m_pJsonData->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pJsonData, iWhich);
        }
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_x_Array)
        {
            pJsonStruct = cJSON_GetArrayItem_x(m_pExternJsonDataRef, iWhich);
        }
    }
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    if(pJsonStruct->type != cJSON_x_NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(const CJsonObject& oJsonObject)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_Parse_x(oJsonObject.ToString().c_str());
    if(pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr_x();
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    unsigned int uiLastIndex = (unsigned int)cJSON_GetArraySize_x(pFocusData) - 1;
    for(std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.begin();
            iter != m_mapJsonArrayRef.end();)
    {
        if(iter->first >= uiLastIndex)
        {
            if(iter->second != NULL)
            {
                delete(iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}

bool CJsonObject::Add(const std::string& strValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateString_x(strValue.c_str());
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(int32 iValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)iValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(uint32 uiValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)uiValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(int64 llValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)llValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(uint64 ullValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)ullValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(int iAnywhere, bool bValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateBool_x(bValue);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(float fValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)fValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Add(double dValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)dValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddNull()
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateNull_x();
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArray_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(const CJsonObject& oJsonObject)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_Parse_x(oJsonObject.ToString().c_str());
    if(pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr_x();
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    for(std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.begin();
            iter != m_mapJsonArrayRef.end();)
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter++);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(const std::string& strValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateString_x(strValue.c_str());
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(int32 iValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)iValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(uint32 uiValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)uiValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(int64 llValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)llValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(uint64 ullValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)ullValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(int iAnywhere, bool bValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateBool_x(bValue);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(float fValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)fValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddAsFirst(double dValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)dValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::AddNullAsFirst()
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if(m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray_x();
        pFocusData = m_pJsonData;
    }

    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateNull_x();
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize_x(pFocusData);
    cJSON_AddItemToArrayHead_x(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize_x(pFocusData);
    if(iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Delete(int iWhich)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_DeleteItemFromArray_x(pFocusData, iWhich);
    for(std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.begin();
            iter != m_mapJsonArrayRef.end();)
    {
        if(iter->first >= (unsigned int)iWhich)
        {
            if(iter->second != NULL)
            {
                delete(iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, const CJsonObject& oJsonObject)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_Parse_x(oJsonObject.ToString().c_str());
    if(pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr_x();
        return(false);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, const std::string& strValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateString_x(strValue.c_str());
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, int32 iValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)iValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, uint32 uiValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)uiValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, int64 llValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)((uint64)llValue), -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, uint64 ullValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateInt_x((uint64)ullValue, 1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, bool bValue, bool bValueAgain)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateBool_x(bValue);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, float fValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)fValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::Replace(int iWhich, double dValue)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateDouble_x((double)dValue, -1);
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool CJsonObject::ReplaceWithNull(int iWhich)
{
    cJSON_x* pFocusData = NULL;
    if(m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if(pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if(pFocusData->type != cJSON_x_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_x* pJsonStruct = cJSON_CreateNull_x();
    if(pJsonStruct == NULL)
    {
        return(false);
    }
    std::map<unsigned int, CJsonObject*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if(iter != m_mapJsonArrayRef.end())
    {
        if(iter->second != NULL)
        {
            delete(iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    cJSON_ReplaceItemInArray_x(pFocusData, iWhich, pJsonStruct);
    if(cJSON_GetArrayItem_x(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

CJsonObject::CJsonObject(cJSON_x* pJsonData)
    : m_pJsonData(NULL), m_pExternJsonDataRef(pJsonData)
{
}


