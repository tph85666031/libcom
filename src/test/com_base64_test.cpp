#include "com_base64.h"
#include "com_test.h"

void com_base64_unit_test_suit(void** state)
{
    std::string result = Base64::Encode((uint8*)"1234567890", sizeof("1234567890") - 1);
    ASSERT_STR_EQUAL("MTIzNDU2Nzg5MA==", result.c_str());
    ASSERT_STR_EQUAL("1234567890", Base64::Decode("MTIzNDU2Nzg5MA").toString().c_str());

    result = Base64::Encode((uint8*)"w832y9gbvnspoVuu923_*)*)*@-", sizeof("w832y9gbvnspoVuu923_*)*)*@-") - 1);
    ASSERT_STR_EQUAL("dzgzMnk5Z2J2bnNwb1Z1dTkyM18qKSopKkAt", result.c_str());
    ASSERT_STR_EQUAL("w832y9gbvnspoVuu923_*)*)*@-", Base64::Decode(result.c_str()).toString().c_str());

    const char* hex = "11BA4013E73AFFD6BFC6BB01B7C7C74E61EAB8C1C49BBD3339";
    ComBytes bytes_org = com_hexstring_to_bytes(hex);
    result = Base64::Encode(bytes_org.getData(), bytes_org.getDataSize());
    ASSERT_STR_EQUAL("EbpAE+c6/9a/xrsBt8fHTmHquMHEm70zOQ==", result.c_str());
    ComBytes bytes = Base64::Decode(result.c_str());
    for (int i = 0; i < bytes.getDataSize(); i++)
    {
        ASSERT_INT_EQUAL(bytes_org.getAt(i), bytes.getData()[i]);
    }
    ASSERT_TRUE(bytes_org == bytes);
}
