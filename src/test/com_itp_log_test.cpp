#include "com_itp_log.h"
#include "com_log.h"
#include "com_test.h"

void com_itp_log_unit_test_suit(void** state)
{
    LogMessageCommon co;
    co.event_type = 1;
    
    LogMessageDlpImFile im;
    im.init(co);
    im.file_path = "sss";
    LOG_I("sss=%s", im.toJson().c_str());
    const char* env = getenv("READER");
    if(env != NULL)
    {
        LOG_I("this is reader");
        ITPLogReader reader;
        reader.setPath("SKYGUAD_ITPLOG");
        com_sleep_s(200);
        reader.destroy();
    }
    else
    {
        LOG_I("this is writer");
        GetITPLogWriter().setPath("SKYGUAD_ITPLOG");
        for(int i = 0; i < 100; i++)
        {
            iLOG_SYS("this is itp system message");
            com_sleep_ms(10);
        }
    }
}

