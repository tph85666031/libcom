#include "bcp.h"

typedef struct
{
    int16 x;
    std::string y;
    uint8 buf[4];

    META(x, y, ByteArray(buf, sizeof(buf)));
} BB;

typedef struct
{
    uint8 x;
    std::string y;
    uint64 z;
    const char* cc;
    BB b;

    META(x, y, '\0', z, cc, '\0', b.toBytes());
} CC;

void tuple_serializer_test()
{
    CC c;
    c.x = 0x01;
    c.y = "123";
    c.z = 0x0102030405060708;
    c.cc = "456";
    c.b.x = 0x5AA5;
    c.b.y = "789";
    c.b.buf[0] = 0xF0;
    c.b.buf[1] = 0xF1;
    c.b.buf[2] = 0xF2;
    c.b.buf[3] = 0xF3;
    ByteArray bytes = c.toBytes();
    LOG_D("val=%s", bytes.toHexString(true).c_str());
    ASSERT_STR_EQUAL(bytes.toHexString(true).c_str(), "01313233000102030405060708343536005AA5373839F0F1F2F3");

    LOG_D("gg=%s", typeid((int8)c.x).name());
}

void com_serializer_unit_test_suit(void** state)
{
    tuple_serializer_test();
    Serializer s;

    uint8 val_u8 = 8;
    uint16 val_u16 = 16;
    uint32 val_u32 = 32;
    uint64 val_u64 = 64;

    int8 val_s8 = -8;
    int16 val_s16 = -16;
    int32 val_s32 = -32;
    int64 val_s64 = -64;

    std::string val_str = "std::string";
    const char* val_char = "val_char";

    uint8 val_array[7];
    memset(val_array, 3, sizeof(val_array));

    s.append(val_u8);
    s.append(val_u16);
    s.append(val_u32);
    s.append(val_u64);

    s.append(val_s8);
    s.append(val_s16);
    s.append(val_s32);
    s.append(val_s64);

    s.append(val_str).append('\0');
    s.append(val_char).append('\0');
    s.append(val_array, sizeof(val_array));

    s = Serializer(s.toBytes().getData(), s.toBytes().getDataSize());

    uint8 oval_u8 = 0;
    uint16 oval_u16 = 0;
    uint32 oval_u32 = 0;
    uint64 oval_u64 = 0;

    int8 oval_s8 = 0;
    int16 oval_s16 = 0;
    int32 oval_s32 = 0;
    int64 oval_s64 = 0;

    std::string oval_str;
    char oval_char[1024 + 1];

    uint8 oval_array[sizeof(val_array)];

    s.detach(oval_u8);
    s.detach(oval_u16);
    s.detach(oval_u32);
    s.detach(oval_u64);
    s.detach(oval_s8);
    s.detach(oval_s16);
    s.detach(oval_s32);
    s.detach(oval_s64);

    s.detach(oval_str);
    s.detach(oval_char, sizeof(oval_char));
    s.detach(oval_array, sizeof(oval_array));

#if 0
    printf("%d,%d,%d,%d,%d,%d,%lld,%lld\n",
           oval_s8, oval_u8,
           oval_s16, oval_u16,
           oval_s32, oval_u32,
           oval_s64, oval_u64);

    printf("%s,%s\n", oval_str.c_str(), oval_char);
    for (int i = 0; i < sizeof(oval_array); i++)
    {
        printf("%02X ", oval_array[i]);
    }
    printf("\n");
#endif

    ASSERT_INT_EQUAL(val_s8, oval_s8);
    ASSERT_INT_EQUAL(val_s16, oval_s16);
    ASSERT_INT_EQUAL(val_s32, oval_s32);
    ASSERT_INT_EQUAL(val_s64, oval_s64);

    ASSERT_INT_EQUAL(val_u8, oval_u8);
    ASSERT_INT_EQUAL(val_u16, oval_u16);
    ASSERT_INT_EQUAL(val_u32, oval_u32);
    ASSERT_INT_EQUAL(val_u64, oval_u64);

    ASSERT_STR_EQUAL(val_str.c_str(), oval_str.c_str());
    ASSERT_STR_EQUAL(val_char, oval_char);

    ASSERT_MEM_EQUAL(val_array, oval_array, sizeof(val_array));


    ByteArray bytes;
    bytes.append(val_u8);
    bytes.append(val_array, sizeof(val_array));
    bytes.append(bytes);

    ASSERT_INT_EQUAL(val_u8, bytes.getData()[0]);
    ASSERT_MEM_EQUAL(val_array, bytes.getData() + 1, sizeof(val_array));
    ASSERT_INT_EQUAL(val_u8, *(bytes.getData() + 1 + sizeof(val_array)));
    ASSERT_MEM_EQUAL(val_array, bytes.getData() + 1 + sizeof(val_array) + 1, sizeof(val_array));

    ByteArray x((uint8*)"123", 4);
    x.getData()[0] = '3';
    ASSERT_STR_EQUAL(x.toString().c_str(), "323");
}
