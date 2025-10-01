#include "com_netlink.h"
#include "com_log.h"
#include "com_test.h"

class MyComNetLinkGeneric: public ComNetLinkGeneric
{
public:
    void onMessage(int sender_id, const uint8* data, int data_size)
    {
        LOG_I("got message from %d,str=%d:%s", sender_id, data_size, (char*)data);
    }
};

void com_netlink_unit_test_suit(void** state)
{
#if __linux__==1
    MyComNetLinkGeneric lnkg;
    lnkg.openLink("generic_test", 133);
    lnkg.sendMessage("message from user", sizeof("message from user"));
    getchar();
#endif
}

