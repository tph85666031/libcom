#include "com_serializer.h"
#include "com_log.h"
#include "com_test.h"

typedef struct
{
    int16 x;
    std::string y;
    uint8 buf[4];

    META_B(x, y, CPPBytes(buf, sizeof(buf)));
} BB;

typedef struct
{
    uint8 x;
    std::string y;
    uint64 z;
    const char* cc;
    BB b;

    META_B(x, y, '\0', z, cc, '\0', b.toBytes());
} CC;

typedef struct
{
    int x = 0;
    std::string y;
    uint8 z = 0;
    char cc[128] = {};
    std::vector<std::string> list;
    META_J(x, y, z, cc, list);
    void reset()
    {
        x = rand() % 100;
        y = "this is Y";
        z = rand() % 100;
        strcpy(cc, "this is cc");
        list.push_back("list1");
        list.push_back("list2");
        list.push_back("list3");
    }
} DD;

typedef struct
{
    bool a = false;
    char b = '\0';
    int8 c = 0;
    uint8 d = 0;
    int16 e = 0;
    uint16 f = 0;
    int32 g = 0;
    uint32 h = 0;
    int64 i = 0;
    uint64 j = 0;
    char k[12] = {};
    const char* l = NULL;
    const char* m = NULL;
    std::string n;
    std::vector<int> o;
    std::set<int> p;
    std::deque<int> q;
    std::queue<int> r;
    std::list<int> s;
    std::map<std::string, int> t;
    std::unordered_map<std::string, int> u;
    DD x;
    void reset()
    {
        srand(com_time_rtc_s());
        a = rand() % 1;
        b = rand() % 127;
        c = rand() % 256;
        d = rand() % 256;
        e = rand();
        f = rand();
        g = rand();
        h = rand();
        i = rand();
        j = rand();
        strcpy(k, "1234567");
        l = "this is L";
        m = "this is M";
        n = "this is N";
        o.push_back(11);
        o.push_back(12);
        o.push_back(13);
        p.insert(21);
        p.insert(22);
        p.insert(23);
        q.push_back(31);
        q.push_back(32);
        q.push_back(33);
        r.push(41);
        r.push(42);
        r.push(43);
        s.push_back(51);
        s.push_back(52);
        s.push_back(53);
        t["m1"] = 61;
        t["m2"] = 62;
        t["m3"] = 63;
        u["m1"] = 71;
        u["m2"] = 72;
        u["m3"] = 73;
        x.reset();
    }
    META_J(a, b, c, d, e, f, g, h, j, k, l, m, n, o, p, q, r, s, t, u, x);
} EE;

typedef struct
{
    int g = 0;
    std::vector<EE> e_list;
    META_J(e_list, g);
    void reset()
    {
        g = rand();
        EE e1;
        e1.reset();
        e_list.push_back(e1);
    };
} FF;

CPPBytes getT1()
{
    CPPBytes b;
    return b;
}

CPPBytes getT2()
{
    return CPPBytes();
}

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
    CPPBytes bytes = c.toBytes();
    LOG_I("val=%s", bytes.toHexString(true).c_str());
    ASSERT_STR_EQUAL(bytes.toHexString(true).c_str(), "01313233000102030405060708343536005AA5373839F0F1F2F3");
    LOG_D("gg=%s", typeid((int8)c.x).name());

    DD d1;
    d1.x = 1;
    d1.y = "yyyy";
    //d1.cc = "ccc";
    strcpy(d1.cc, "cccc");
    d1.z = 8;
    for(int i = 0; i < 5; i++)
    {
        d1.list.push_back(std::to_string(i));
    }
    std::string json = d1.toJson();
    LOG_I("json=%s", json.c_str());

    DD d2;
    d2.x = 10;
    d2.z = 2;
    d2.fromJson(json.c_str());
    ASSERT_INT_EQUAL(d2.x, 1);
    ASSERT_INT_EQUAL(d2.z, 8);
    ASSERT_STR_EQUAL(d2.y.c_str(), "yyyy");

    EE e1;
    e1.reset();
    LOG_I("json_e1=%s", e1.toJson().c_str());

    EE e2;
    e2.a = true;
    e2.b = 22;
    e2.fromJson(e1.toJson().c_str());
    LOG_I("json_e2=%s", e2.toJson().c_str());

    FF f1;
    f1.reset();
    LOG_I("json_f1=%s", f1.toJson().c_str());

    FF f2;
    f2.fromJson(f1.toJson().c_str());
    LOG_I("json_f2=%s", f2.toJson().c_str());
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
    for(int i = 0; i < sizeof(oval_array); i++)
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


    CPPBytes bytes;
    bytes.append(val_u8);
    bytes.append(val_array, sizeof(val_array));
    bytes.append(bytes);

    ASSERT_INT_EQUAL(val_u8, bytes.getData()[0]);
    ASSERT_MEM_EQUAL(val_array, bytes.getData() + 1, sizeof(val_array));
    ASSERT_INT_EQUAL(val_u8, *(bytes.getData() + 1 + sizeof(val_array)));
    ASSERT_MEM_EQUAL(val_array, bytes.getData() + 1 + sizeof(val_array) + 1, sizeof(val_array));

    CPPBytes x((uint8*)"123", 4);
    x.getData()[0] = '3';
    ASSERT_STR_EQUAL(x.toString().c_str(), "323");
}
