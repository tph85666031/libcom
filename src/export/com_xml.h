#ifndef __COM_XML_H__
#define __COM_XML_H__

#include "com_base.h"

class COM_EXPORT ComXmlParser
{
public:
    ComXmlParser();
    ComXmlParser(const char* file);
    ComXmlParser(const ComBytes& content);
    virtual ~ComXmlParser();

    bool load(const char* file);
    bool load(const ComBytes& content);
    bool save();
    bool saveAs(const char* file);

    int64 getTextAsNumber(const char* path, int64 default_val = 0);
    double getTextAsDouble(const char* path, double default_val = 0);
    bool getTextAsBool(const char* path, bool default_val = false);
    std::string getText(const char* path);

    int64 getAttributeAsNumber(const char* path, const char* attribute_name, int64 default_val = 0);
    double getAttributeAsDouble(const char* path, const char* attribute_name, double default_val = 0);
    bool getAttributeAsBool(const char* path, const char* attribute_name, bool default_val = false);
    std::string getAttribute(const char* path, const char* attribute_name);

    template<class T>
    bool setText(const char* path, const T val)
    {
        return setText(path, std::to_string(val).c_srt());
    };
    bool setText(const char* path, const char* value);
    bool setAttribute(const char* path, const char* attribute_name, const char* value);
    bool isNodeExists(const char* path);
    std::string extractText();
private:
    std::string extractElementText(void* xml_element);
    void* getNode(const char* path);
    void* createNode(const char* path);
    std::string pathRefine(const char* path);
private:
    void* ctx;
    std::string file;
};
typedef ComXmlParser DEPRECATED("Use ComXmlParser instead") CPPXmlParser;

COM_EXPORT std::string com_plist_to_json(const std::string& xml_content);
COM_EXPORT std::string com_plist_to_json(const char* xml_content);

COM_EXPORT std::string com_plist_from_json(const std::string& json_content);
COM_EXPORT std::string com_plist_from_json(const char* json_content);

#endif /* __COM_XML_H__ */

