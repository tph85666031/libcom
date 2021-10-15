#ifndef __COM_CRC_H__
#define __COM_CRC_H__

#include "com_base.h"

uint8_t com_crc8(uint8_t* data, int data_size, uint8_t init_val = 0);
uint16_t com_crc16(uint8_t* data, int data_size, uint16_t init_val = 0);
uint32_t com_crc32(uint8_t* data, int data_size, uint32_t init_val = 0);
uint64_t com_crc64(uint8_t* data, int data_size, uint64_t init_val = 0);

#endif /* __COM_CRC_H__ */
