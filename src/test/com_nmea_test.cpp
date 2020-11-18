#include "com_nmea.h"
#include "com_file.h"
#include "com_log.h"

void com_nmea_unit_test_suit(void** state)
{
    COM_GPS gps;
    FILE* file = com_file_open("./gps.txt", "r");
    if (file == NULL)
    {
        LOG_W("file not found: ./gps.txt");
        return;
    }
    char line[4096];
    while (com_file_readline(file, line, sizeof(line)))
    {
        if (com_nmea_parse(line, &gps) == false)
        {
            LOG_W("parse error: %s", line);
            continue;
        }
        switch (gps.type)
        {
        case MINMEA_SENTENCE_RMC:
            LOG_D("rmc parsed");
            break;
        case MINMEA_SENTENCE_GGA:
            LOG_D("gga parsed");
            break;
        case MINMEA_SENTENCE_GST:
            LOG_D("gst parsed");
            break;
        case MINMEA_SENTENCE_GSV:
            LOG_D("gsv parsed");
            break;
        case MINMEA_SENTENCE_VTG:
            LOG_D("vtg parsed");
            break;
        case MINMEA_SENTENCE_ZDA:
            LOG_D("zda parsed");
            break;
        default:
            LOG_W("parse error: %s", line);
            break;
        }
    }

    com_file_close(file);
}
