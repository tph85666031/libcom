#include "httplib.h"
#include "com_log.h"

using namespace httplib;

void com_http_unit_test_suit(void** state)
{
    //httplib
    Server server;
    std::string app_name = "/" + com_get_bin_name();
    server.Get(app_name + "/test", [&](const Request & request, Response & response)
    {
        std::string arg1 = request.get_param_value("arg1");
        std::string arg2 = request.get_param_value("arg2");
        std::string arg3 = request.get_param_value("arg3");
        LOG_I("arg1=%s,arg2=%s,arg3=%s", arg1.c_str(), arg2.c_str(), arg3.c_str());
        response.set_content("hello", "text/plain");
    });
    server.listen("0.0.0.0", 8088);
}

