#ifndef __BCP_GPS_H__
#define __BCP_GPS_H__

#include "bcp_com.h"
#include "minmea.h"

typedef struct
{
    NMEA_DATA_TYPE type;
    union
    {
        struct minmea_sentence_rmc rmc;
        struct minmea_sentence_gga gga;
        struct minmea_sentence_gst gst;
        struct minmea_sentence_gsv gsv;
        struct minmea_sentence_vtg vtg;
        struct minmea_sentence_zda zda;
    } data;
} BCP_GPS;

bool bcp_nmea_parse(const char* nmea_line, BCP_GPS* gps);

#endif /* __BCP_GPS_H__ */