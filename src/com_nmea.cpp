#include "minmea.h"
#include "com_nmea.h"
#include "com_log.h"
#include "com_file.h"

bool com_nmea_parse(const char* nmea_line, BCP_GPS* gps)
{
    if (nmea_line == NULL || gps == NULL)
    {
        return false;
    }
    gps->type = minmea_sentence_id(nmea_line, false);
    switch (gps->type)
    {
    case MINMEA_SENTENCE_RMC:
        return minmea_parse_rmc(&gps->data.rmc, nmea_line);

    case MINMEA_SENTENCE_GGA:
        return minmea_parse_gga(&gps->data.gga, nmea_line);

    case MINMEA_SENTENCE_GST:
        return minmea_parse_gst(&gps->data.gst, nmea_line);

    case MINMEA_SENTENCE_GSV:
        return minmea_parse_gsv(&gps->data.gsv, nmea_line);

    case MINMEA_SENTENCE_VTG:
        return minmea_parse_vtg(&gps->data.vtg, nmea_line);

    case MINMEA_SENTENCE_ZDA:
        return minmea_parse_zda(&gps->data.zda, nmea_line);
    default:
        LOG_W("sentence is not parsed: %s", nmea_line);
        break;
    }
    return false;
}

