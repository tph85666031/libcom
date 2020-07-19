#ifndef __BCP_CRC_H__
#define __BCP_CRC_H__

#include "bcp_com.h"

uint8 bcp_crc8(uint8* data, int data_size, uint8 init_val = 0);
uint16 bcp_crc16(uint8* data, int data_size, uint16 init_val = 0);
uint32 bcp_crc32(uint8* data, int data_size, uint32 init_val = 0);
uint64 bcp_crc64(uint8* data, int data_size, uint64 init_val = 0);

#endif /* __BCP_CRC_H__ */
