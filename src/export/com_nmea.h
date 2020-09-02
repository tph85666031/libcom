#ifndef __BCP_GPS_H__
#define __BCP_GPS_H__

#include "com_base.h"
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
} COM_GPS;

bool com_nmea_parse(const char* nmea_line, COM_GPS* gps);

#endif /* __BCP_GPS_H__ */