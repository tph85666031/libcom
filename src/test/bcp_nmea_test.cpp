#include "bcp.h"

void bcp_nmea_unit_test_suit(void** state)
{
    BCP_GPS gps;
    FILE* file = bcp_file_open("./gps.txt", "r");
    if (file == NULL)
    {
        LOG_W("file not found: ./gps.txt");
        return;
    }
    char line[4096];
    while (bcp_file_readline(file, line, sizeof(line)))
    {
        if (bcp_nmea_parse(line, &gps) == false)
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

    bcp_file_close(file);
}
