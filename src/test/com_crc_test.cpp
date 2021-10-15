#include "com.h"

void com_crc_unit_test_suit(void** state)
{
    uint8_t crc8 = com_crc8((uint8_t*)"123456", 6);
    LOG_D("crc8=0x%X", crc8);

    uint16_t crc16 = com_crc16((uint8_t*)"123456", 6);
    ASSERT_INT_EQUAL(crc16, 0x20E4);

    uint32_t crc32 = com_crc32((uint8_t*)"123456", 6);
    ASSERT_INT_EQUAL(crc32, 0x972D361);

    uint64_t crc64 = com_crc64((uint8_t*)"123456789012345678", 6);
    LOG_D("crc64=0x%llX", crc64);
}