#ifndef __COM_CRC_H__
#define __COM_CRC_H__

#include "com_base.h"

COM_EXPORT uint8 com_crc8(uint8* data, int data_size, uint8 init_val = 0);
COM_EXPORT uint16 com_crc16(uint8* data, int data_size, uint16 init_val = 0);
COM_EXPORT uint32 com_crc32(uint8* data, int data_size, uint32 init_val = 0);
COM_EXPORT uint64 com_crc64(uint8* data, int data_size, uint64 init_val = 0);

#endif /* __COM_CRC_H__ */
