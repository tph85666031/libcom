#include <algorithm>
#include "com_base.h"
#include "com_log.h"
#include "com_base64.h"
#include "com_xml.h"
#include "CJsonObject.h"
#include "tinyxml2.h"

using namespace tinyxml2;

#define XML_DECLARATION "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"

ComXmlParser::ComXmlParser()
{
    ctx = new XMLDocument();
    ((XMLDocument*)ctx)->Parse(XML_DECLARATION);
}

ComXmlParser::ComXmlParser(const char* file)
{
    ctx = new XMLDocument();
    if(file != NULL)
    {
        this->file = file;
        load(file);
    }
}

ComXmlParser::ComXmlParser(const void* data, int data_size)
{
    ctx = new XMLDocument();
    load(data, data_size);
}

ComXmlParser::ComXmlParser(const ComBytes& content)
{
    ctx = new XMLDocument();
    load(content);
}

ComXmlParser::~ComXmlParser()
{
    if(ctx != NULL)
    {
        delete(XMLDocument*)ctx;
    }
}

bool ComXmlParser::load(const char* file)
{
    if(ctx == NULL || file == NULL)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    std::string file_str = com_string_utf8_to_ansi(file);
#else
    std::string file_str = file;
#endif
    if(((XMLDocument*)ctx)->LoadFile(file_str.c_str()) != XML_SUCCESS)
    {
        LOG_E("failed to load xml file");
        return false;
    }
    return true;
}

bool ComXmlParser::load(const void* data, int data_size)
{
    if(ctx == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    if(((XMLDocument*)ctx)->Parse((const char*)data, data_size) != XML_SUCCESS)
    {
        LOG_E("failed to load xml content");
        return false;
    }
    return true;
}

bool ComXmlParser::load(const ComBytes& content)
{
    return load(content.getData(), content.getDataSize());
}

bool ComXmlParser::save()
{
    return saveAs(file.c_str());
}

bool ComXmlParser::saveAs(const char* file)
{
    if(file == NULL || file[0] == '\0')
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    std::string file_str = com_string_utf8_to_ansi(file);
#else
    std::string file_str = file;
#endif
    return (((XMLDocument*)ctx)->SaveFile(file_str.c_str(), false) == XML_SUCCESS);
}

int64 ComXmlParser::getTextAsNumber(const char* path, int64 default_val)
{
    std::string value = getText(path);
    if(value.empty())
    {
        return default_val;
    }

    return strtoll(value.c_str(), NULL, 10);
}

double ComXmlParser::getTextAsDouble(const char* path, double default_val)
{
    std::string value = getText(path);
    if(value.empty())
    {
        return default_val;
    }

    return strtod(value.c_str(), NULL);
}

bool ComXmlParser::getTextAsBool(const char* path, bool default_val)
{
    std::string value = getText(path);
    if(value.empty())
    {
        return default_val;
    }

    com_string_to_lower(value);
    com_string_trim(value);
    if(value == "true")
    {
        return true;
    }
    else if(value == "false")
    {
        return false;
    }
    else
    {
        return (strtol(value.c_str(), NULL, 10) == 1);
    }
}

std::string ComXmlParser::getText(const char* path)
{
    if(path == NULL)
    {
        LOG_E("arg incorrect");
        return std::string();
    }

    XMLElement* node = (XMLElement*)getNode(path);
    if(node == NULL)
    {
        return std::string();
    }

    const char* text = node->GetText();
    if(text == NULL)
    {
        return std::string();
    }
    return text;
}

int64 ComXmlParser::getAttributeAsNumber(const char* path, const char* attribute_name, int64 default_val)
{
    std::string value = getAttribute(path, attribute_name);
    if(value.empty())
    {
        return default_val;
    }

    return strtoll(value.c_str(), NULL, 10);
}

double ComXmlParser::getAttributeAsDouble(const char* path, const char* attribute_name, double default_val)
{
    std::string value = getAttribute(path, attribute_name);
    if(value.empty())
    {
        return default_val;
    }

    return strtod(value.c_str(), NULL);
}

bool ComXmlParser::getAttributeAsBool(const char* path, const char* attribute_name, bool default_val)
{
    std::string value = getAttribute(path, attribute_name);
    if(value.empty())
    {
        return default_val;
    }

    com_string_to_lower(value);
    com_string_trim(value);
    if(value == "true")
    {
        return true;
    }
    else if(value == "false")
    {
        return false;
    }
    else
    {
        return (strtol(value.c_str(), NULL, 10) == 1);
    }
}

std::string ComXmlParser::getAttribute(const char* path, const char* attribute_name)
{
    if(path == NULL || attribute_name == NULL)
    {
        LOG_E("arg incorrect");
        return std::string();
    }
    XMLElement* node = (XMLElement*)getNode(path);
    if(node == NULL)
    {
        return std::string();
    }

    const char* value = node->Attribute(attribute_name);
    if(value == NULL)
    {
        LOG_E("attribute not found, path=%s, node=%s, attribute=%s", path, node->Name(), attribute_name);
        return std::string();
    }
    return value;
}

void* ComXmlParser::getNode(const char* path)
{
    std::string path_str = pathRefine(path);
    if(path_str.empty())
    {
        LOG_E("path incorrect: %s", path);
        return NULL;
    }
    std::vector<std::string> items = com_string_split(path_str.c_str(), "/");
    if(items.empty())
    {
        LOG_E("path incorrrect:%s", path_str.c_str());
        return NULL;
    }
    XMLElement* root = ((XMLDocument*)ctx)->RootElement();
    if(root == NULL)
    {
        LOG_E("xml root not found:%s", path_str.c_str());
        return NULL;
    }
    if(com_string_equal(root->Name(), items[0].c_str()) == false)
    {
        LOG_E("path miss match: %s <--> %s", root->Name(), items[0].c_str());
        return NULL;
    }
    XMLElement* child = root;
    for(size_t i = 1; i < items.size(); i++)
    {
        child = child->FirstChildElement(items[i].c_str());
        if(child == NULL)
        {
            break;
        }
    }
    return child;
}

bool ComXmlParser::setText(const char* path, const char* value)
{
    if(path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    if(value == NULL)
    {
        XMLElement* node = (XMLElement*)getNode(path);
        if(node != NULL && node->Parent() != NULL)
        {
            node->Parent()->DeleteChild(node);
        }
        return true;
    }
    XMLElement* node = (XMLElement*)createNode(path);
    if(node == NULL)
    {
        return false;
    }
    node->SetText(value);
    return true;
}

bool ComXmlParser::setAttribute(const char* path, const char* attribute_name, const char* value)
{
    if(path == NULL || attribute_name == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    if(value == NULL)
    {
        XMLElement* node = (XMLElement*)getNode(path);
        if(node != NULL)
        {
            node->DeleteAttribute(attribute_name);
        }
        return true;
    }
    XMLElement* node = (XMLElement*)createNode(path);
    if(node == NULL)
    {
        return false;
    }
    node->SetAttribute(attribute_name, value);
    return true;
}

bool ComXmlParser::isNodeExists(const char* path)
{
    if(path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    XMLElement* node = (XMLElement*)getNode(path);
    if(node)
    {
        return true;
    }
    return false;
}

void* ComXmlParser::createNode(const char* path)
{
    std::string path_str = pathRefine(path);
    if(path_str.empty())
    {
        LOG_E("path incorrect: %s", path);
        return NULL;
    }
    std::vector<std::string> items = com_string_split(path_str.c_str(), "/");
    if(items.empty())
    {
        LOG_E("path incorrrect:%s", path_str.c_str());
        return NULL;
    }
    XMLElement* root = ((XMLDocument*)ctx)->RootElement();
    if(root == NULL || com_string_equal(root->Name(), items[0].c_str()) == false)
    {
        root = ((XMLDocument*)ctx)->NewElement(items[0].c_str());
        if(root == NULL)
        {
            LOG_E("failed to create node:%s", items[0].c_str());
            return NULL;
        }
        ((XMLDocument*)ctx)->InsertEndChild(root);
    }
    XMLElement* child = root;
    for(size_t i = 1; i < items.size(); i++)
    {
        bool found = false;
        //查看是否有节点存在
        XMLElement* ele = child->FirstChildElement();
        while(ele != NULL)
        {
            if(com_string_equal(ele->Name(), items[i].c_str()))
            {
                found = true;
                break;
            }
            ele = ele->NextSiblingElement();
        }

        if(found)
        {
            child = ele;
        }
        else
        {
            //不存在则新增一个
            child = child->InsertNewChildElement(items[i].c_str());
            if(child == NULL)
            {
                LOG_E("failed to cerate node:%s", items[i].c_str());
                return NULL;
            }
        }
    }
    return (void*)child;
}

std::string ComXmlParser::pathRefine(const char* path)
{
    if(path == NULL)
    {
        return std::string();
    }
    std::string path_str = path;
    path_str.erase(std::remove_if(path_str.begin(), path_str.end(), [](char c)
    {
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v')
        {
            return true;
        }
        return false;
    }), path_str.end());

    if(path_str.back() == '/')
    {
        path_str.pop_back();
    }

    if(path_str.front() == '/')
    {
        path_str.erase(0, 1);
    }

    return path_str;
}

std::string ComXmlParser::extractText()
{
    if(ctx == NULL)
    {
        return std::string();
    }
    return extractElementText(((XMLDocument*)ctx)->RootElement());
}

std::string ComXmlParser::extractElementText(void* xml_element)
{
    std::string result;
    if(xml_element == NULL)
    {
        return std::string();
    }
    while(xml_element != NULL)
    {
        const char* text = ((XMLElement*)xml_element)->GetText();
        if(text != NULL)
        {
            result.append(text);
            if(result.back() != '\n')
            {
                result.append("\n");
            }
        }
        result.append(extractElementText(((XMLElement*)xml_element)->FirstChildElement()));
        xml_element = ((XMLElement*)xml_element)->NextSiblingElement();
    }
    return result;
}

static void plist_other_to_json(CJsonObject& json, std::string& key, XMLElement* ele)
{
    if(com_string_equal(ele->Name(), "string"))
    {
        if(ele->GetText() != NULL)
        {
            json.Add(key, ele->GetText());
        }
        else
        {
            json.Add(key, std::string());
        }
    }
    else if(com_string_equal(ele->Name(), "integer") || com_string_equal(ele->Name(), "int") || com_string_equal(ele->Name(), "long"))
    {
        if(ele->GetText() != NULL)
        {
            int64 val = strtoll(ele->GetText(), NULL, 10);
            json.Add(key, val);
        }
    }
    else if(com_string_equal_ignore_case(ele->Name(), "true"))
    {
        json.Add(key, true);
    }
    else if(com_string_equal_ignore_case(ele->Name(), "false"))
    {
        json.Add(key, false);
    }
    else if(com_string_equal(ele->Name(), "bool"))
    {
        if(ele->GetText() != NULL)
        {
            if(com_string_equal_ignore_case(ele->GetText(), "true"))
            {
                json.Add(key, true);
            }
            else if(com_string_equal_ignore_case(ele->GetText(), "false"))
            {
                json.Add(key, false);
            }
            else
            {
                int val = strtol(ele->GetText(), NULL, 10);
                json.Add(key, (val == 1));
            }
        }
    }
    else if(com_string_equal(ele->Name(), "date"))
    {
    }
    else if(com_string_equal(ele->Name(), "data"))
    {
        json.Add(key, ComBase64::Decode(ele->GetText()));
    }
}

static void plist_other_to_json(CJsonObject& json, XMLElement* ele)
{
    if(com_string_equal(ele->Name(), "string"))
    {
        if(ele->GetText() != NULL)
        {
            json.Add(ele->GetText());
        }
        else
        {
            json.Add(std::string());
        }
    }
    else if(com_string_equal(ele->Name(), "integer") || com_string_equal(ele->Name(), "int") || com_string_equal(ele->Name(), "long"))
    {
        if(ele->GetText() != NULL)
        {
            int64 val = strtoll(ele->GetText(), NULL, 10);
            json.Add(val);
        }
    }
    else if(com_string_equal_ignore_case(ele->Name(), "true"))
    {
        json.Add(true);
    }
    else if(com_string_equal_ignore_case(ele->Name(), "false"))
    {
        json.Add(false);
    }
    else if(com_string_equal(ele->Name(), "bool"))
    {
        if(ele->GetText() != NULL)
        {
            if(com_string_equal_ignore_case(ele->GetText(), "true"))
            {
                json.Add(true);
            }
            else if(com_string_equal_ignore_case(ele->GetText(), "false"))
            {
                json.Add(false);
            }
            else
            {
                int val = strtol(ele->GetText(), NULL, 10);
                json.Add((val == 1));
            }
        }
    }
    else if(com_string_equal(ele->Name(), "date"))
    {
    }
    else if(com_string_equal(ele->Name(), "data"))
    {
        json.Add(ComBase64::Decode(ele->GetText()));
    }
}

static void plist_dict_to_json(CJsonObject& json, XMLElement* parent);
static void plist_array_to_json(CJsonObject& json, XMLElement* parent)
{
    std::string key;
    for(XMLElement* child = parent->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
    {
        if(com_string_equal(child->Name(), "key"))
        {
            if(child->GetText() != NULL)
            {
                key = child->GetText();
            }
        }
        else if(com_string_equal(child->Name(), "dict"))
        {
            CJsonObject json_sub;
            plist_dict_to_json(json_sub, child);
            json.Add(json_sub);
        }
        else if(com_string_equal(child->Name(), "array"))
        {
            LOG_W("not support");
        }
        else
        {
            plist_other_to_json(json, child);
        }
    }
}

static void plist_dict_to_json(CJsonObject& json, XMLElement* parent)
{
    if(parent == NULL)
    {
        return;
    }
    std::string key;
    for(XMLElement* child = parent->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
    {
        if(com_string_equal(child->Name(), "key"))
        {
            if(child->GetText() != NULL)
            {
                key = child->GetText();
            }
            continue;
        }
        if(key.empty())
        {
            continue;
        }
        if(com_string_equal(child->Name(), "dict"))
        {
            CJsonObject json_sub;
            plist_dict_to_json(json_sub, child);
            json.Add(key, json_sub);
        }
        else if(com_string_equal(child->Name(), "array"))
        {
            json.AddEmptySubArray(key);
            plist_array_to_json(json[key], child);
        }
        else
        {
            plist_other_to_json(json, key, child);
        }
        key.clear();
    }
}

static void plist_from_json_object(XMLDocument& doc, XMLElement* parent, CJsonObject& json);
static void plist_from_json_array(XMLDocument& doc, XMLElement* parent, CJsonObject& json)
{
    for(int i = 0; i < json.GetArraySize(); i++)
    {
        int type = json.GetValueType(i);
        if(type == cJSON_True)
        {
            //parent->InsertEndChild(doc.NewElement("true"));
            XMLElement* ele_val = doc.NewElement("bool");
            ele_val->SetText("true");

            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_False)
        {
            //parent->InsertEndChild(doc.NewElement("false"));
            XMLElement* ele_val = doc.NewElement("bool");
            ele_val->SetText("false");

            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_NULL)
        {
        }
        else if(type == cJSON_Int)
        {
            uint64 val;
            json.Get(i, val);

            XMLElement* ele_val = doc.NewElement("int");//integer
            ele_val->SetText(std::to_string(val).c_str());

            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_Double)
        {
            double val;
            json.Get(i, val);

            XMLElement* ele_val = doc.NewElement("real");
            ele_val->SetText(std::to_string(val).c_str());

            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_String)
        {
            std::string val;
            json.Get(i, val);

            XMLElement* ele_val = doc.NewElement("string");
            ele_val->SetText(val.c_str());

            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_Array)
        {
            CJsonObject json_sub_array;
            json.Get(i, json_sub_array);

            XMLElement* ele_val = doc.NewElement("array");
            plist_from_json_array(doc, ele_val, json_sub_array);

            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_Object)
        {
            CJsonObject json_sub_object;
            json.Get(i, json_sub_object);

            XMLElement* ele_val = doc.NewElement("dict");
            plist_from_json_object(doc, ele_val, json_sub_object);

            parent->InsertEndChild(ele_val);
        }
    }
}

static void plist_from_json_object(XMLDocument& doc, XMLElement* parent, CJsonObject& json)
{
    std::string key;
    while(json.GetKey(key))
    {
        int type = json.GetValueType(key);
        if(type == cJSON_True)
        {
            XMLElement* ele_key = doc.NewElement("key");
            ele_key->SetText(key.c_str());
            parent->InsertEndChild(ele_key);

            XMLElement* ele_val = doc.NewElement("bool");
            ele_val->SetText("true");
            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_False)
        {
            XMLElement* ele_key = doc.NewElement("key");
            ele_key->SetText(key.c_str());
            parent->InsertEndChild(ele_key);

            XMLElement* ele_val = doc.NewElement("bool");
            ele_val->SetText("false");
            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_NULL)
        {
        }
        else if(type == cJSON_Int)
        {
            uint64 val;
            json.Get(key, val);

            XMLElement* ele_key = doc.NewElement("key");
            ele_key->SetText(key.c_str());

            XMLElement* ele_val = doc.NewElement("int");//integer
            ele_val->SetText(std::to_string(val).c_str());

            parent->InsertEndChild(ele_key);
            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_Double)
        {
            double val;
            json.Get(key, val);

            XMLElement* ele_key = doc.NewElement("key");
            ele_key->SetText(key.c_str());

            XMLElement* ele_val = doc.NewElement("real");
            ele_val->SetText(std::to_string(val).c_str());

            parent->InsertEndChild(ele_key);
            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_String)
        {
            std::string val;
            json.Get(key, val);

            XMLElement* ele_key = doc.NewElement("key");
            ele_key->SetText(key.c_str());

            XMLElement* ele_val = doc.NewElement("string");
            ele_val->SetText(val.c_str());

            parent->InsertEndChild(ele_key);
            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_Array)
        {
            XMLElement* ele_key = doc.NewElement("key");
            ele_key->SetText(key.c_str());

            XMLElement* ele_val = doc.NewElement("array");
            plist_from_json_array(doc, ele_val, json[key]);

            parent->InsertEndChild(ele_key);
            parent->InsertEndChild(ele_val);
        }
        else if(type == cJSON_Object)
        {
            XMLElement* ele_key = doc.NewElement("key");
            ele_key->SetText(key.c_str());

            XMLElement* ele_val = doc.NewElement("dict");
            plist_from_json_object(doc, ele_val, json[key]);

            parent->InsertEndChild(ele_key);
            parent->InsertEndChild(ele_val);
        }
    }
}

std::string com_plist_to_json(const std::string& xml_content)
{
    return com_plist_to_json(xml_content.c_str());
}

std::string com_plist_to_json(const char* xml_content)
{
    XMLDocument doc;
    if(doc.Parse(xml_content) != XML_SUCCESS)
    {
        return std::string();
    }

    XMLElement* root = doc.RootElement();
    if(root == NULL)
    {
        return std::string();
    }

    CJsonObject json;
    if(com_string_equal(root->Name(), "dict"))
    {
        plist_dict_to_json(json, root);
    }
    else
    {
        for(XMLElement* child = root->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
        {
            if(com_string_equal(child->Name(), "dict"))
            {
                plist_dict_to_json(json, child);
                break;
            }
        }
    }

    return  json.ToFormattedString();
}

std::string com_plist_from_json(const std::string& json_content)
{
    return com_plist_from_json(json_content.c_str());
}

std::string com_plist_from_json(const char* json_content)
{
    if(json_content == NULL)
    {
        return std::string();
    }

    XMLDocument doc;
    doc.Parse(XML_DECLARATION);

    //XMLElement* ele_plist = doc.NewElement("plist");
    //ele_plist->SetAttribute("version", "1.0");
    XMLElement* ele_dict = doc.NewElement("dict");
    //ele_plist->InsertEndChild(ele_dict);
    doc.InsertEndChild(ele_dict);

    CJsonObject json(json_content);
    plist_from_json_object(doc, ele_dict, json);

    XMLPrinter printer;
    doc.Print(&printer);
    if(printer.CStr() == NULL)
    {
        return std::string();
    }
    return printer.CStr();
}

