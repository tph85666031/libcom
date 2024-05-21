#include "com_xml.h"
#include "com_serializer.h"
#include "com_log.h"
#include "com_file.h"
#include "com_test.h"

static const char* xml_content = "<? xml version = \"1.0\" encoding=\"utf-8\"?>"\
                                 "    <root>"\
                                 "        <logging>"\
                                 "            <level>TRACE</level>"\
                                 "            <console>"\
                                 "                <enabled>true</enabled>"\
                                 "            </console>"\
                                 "            <syslog>"\
                                 "                <enabled c=\"123\">true</enabled>"\
                                 "            </syslog>"\
                                 "            <eventLog>"\
                                 "                <enabled>true</enabled>"\
                                 "            </eventLog>"\
                                 "            <file>"\
                                 "                <enabled>true</enabled>"\
                                 "                <FileName>%APP_USER_HOME%/.skyguard/lep/log/EndpointClientGUI_%APP_DISPLAY%_x.log</FileName>"\
                                 "                <FileNameError>%APP_USER_HOME%/.skyguard/lep/log/EndpointClientGUI_%APP_DISPLAY%_xE.log</FileNameError>"\
                                 "                <MaxSize>10MB</MaxSize>"\
                                 "                <MaxBackupIndex>10</MaxBackupIndex>"\
                                 "            </file>"\
                                 "            <coredump>"\
                                 "                <enabled>false</enabled>"\
                                 "            </coredump>"\
                                 "        </logging>"\
                                 "        <exceptionHandler>"\
                                 "            <DumpType>0</DumpType>"\
                                 "            <DumpPath>%APP_USER_HOME%/.skyguard/lep/log/EndpointClientGUI_%APP_DISPLAY%_x.dmp</DumpPath>"\
                                 "            <PreventSetUnhandledException>true</PreventSetUnhandledException>"\
                                 "            <DisableWindowsErrorReportingDialog>true</DisableWindowsErrorReportingDialog>"\
                                 "        </exceptionHandler>"\
                                 "    </root>";

void com_xml_unit_test_suit(void** state)
{
    com_file_writef("./1.xml", xml_content, strlen(xml_content));
    ComXmlParser parser("./1.xml");
    std::string text = parser.getText("root/logging/console/enabled");
    std::string attr = parser.getAttribute("root/logging/syslog/enabled", "c");
    ASSERT_STR_EQUAL(text.c_str(), "true");
    ASSERT_STR_EQUAL(attr.c_str(), "123");

    parser.setText("root/logging/console/enabled", "false");
    parser.setAttribute("root/logging/console/enabled", "a", "abc");
    parser.setAttribute("root/logging/syslog/enabled", "c", "456");

    text = parser.getText("root/logging/console/enabled");
    attr = parser.getAttribute("root/logging/syslog/enabled", "c");
    ASSERT_STR_EQUAL(text.c_str(), "false");
    ASSERT_STR_EQUAL(attr.c_str(), "456");

    attr = parser.getAttribute("root/logging/console/enabled", "a");
    ASSERT_STR_EQUAL(attr.c_str(), "abc");

    ASSERT_TRUE(parser.save());
    com_file_remove("./1.xml");

    parser = ComXmlParser();
    parser.setText("a/b/c/d/e/", "value_e");
    parser.setAttribute("a/b", "attr_a", "a");
    text = parser.getText("a/b/c/d/e");
    attr = parser.getAttribute("a/b", "attr_a");
    ASSERT_TRUE(parser.saveAs("./2.xml"));
    ASSERT_STR_EQUAL(text.c_str(), "value_e");
    ASSERT_STR_EQUAL(attr.c_str(), "a");

    com_file_remove("./2.xml");
}

class A
{
public:
    bool operator==(const A& a)
    {
        if(a.p1 == this->p1
                && a.p2 == this->p2
                && a.p3 == this->p3)
        {
            return true;
        }
        return false;
    }
    int p1;
    std::string p2;
    bool p3;
    META_J(p1, p2, p3);
};

class X
{
public:
    bool operator==(const X& x)
    {
        if(x.Label == this->Label
                && x.RunAtLoad == this->RunAtLoad
                && x.StartInterval == this->StartInterval
                && sub_dict == x.sub_dict
                && x.ProgramArguments.size() == this->ProgramArguments.size())
        {
            for(size_t i = 0; i < x.ProgramArguments.size(); i++)
            {
                if(x.ProgramArguments[i] != this->ProgramArguments[i])
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
public:
    std::string Label;
    std::vector<std::string> ProgramArguments;
    bool RunAtLoad;
    int StartInterval;
    A sub_dict;

    META_J(Label, ProgramArguments, RunAtLoad, StartInterval, StartInterval, sub_dict);
};

void com_plist_unit_test_suit(void** state)
{
    X x1;
    x1.Label = "xxxxxLabel";
    x1.ProgramArguments.push_back("arg1");
    x1.ProgramArguments.push_back("arg2");
    x1.ProgramArguments.push_back("arg3");
    x1.RunAtLoad = false;
    x1.StartInterval = 333;
    x1.sub_dict.p1 = 98;
    x1.sub_dict.p2 = "this is str p2";
    x1.sub_dict.p3 = true;

    X x2;
    x2.fromJson(com_plist_to_json(com_plist_from_json(x1.toJson())).c_str());

    LOG_I("plist=\n%s", com_plist_from_json(x1.toJson()).c_str());

    ASSERT_TRUE(x1 == x2);

    std::string pist_content = com_plist_from_json(com_file_readall("/data/1.json").toString());
    LOG_I("pist_content=%s", pist_content.c_str());
}


