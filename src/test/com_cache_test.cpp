#include "com.h"

void com_cache_unit_test_suit(void** state)
{
    com_file_remove("./test.db");
    CacheManager cache_manager;
    cache_manager.start("./test.db");
    cache_manager.clear(true);
    cache_manager.setFlushCount(10);
    cache_manager.setFlushInterval(10);
    for (int i = 0; i < 10000; i++)
    {
        Cache cache(std::to_string(i).c_str(), (uint8*)"1234", 5);
        cache_manager.append(cache);
    }

    ASSERT_INT_EQUAL(cache_manager.getCacheCount(), 10000);
    Cache cache_x(std::to_string(10001).c_str(), (uint8*)"5678", 5);
    cache_manager.append(cache_x);
    cache_manager.flushToDisk();
    Cache cache = cache_manager.getFirst();
    ASSERT_NOT_NULL(cache.getData());
    ASSERT_STR_EQUAL((char*)cache.getData(), "1234");
    cache = cache_manager.getByUUID(std::to_string(1).c_str());
    ASSERT_NOT_NULL(cache.getData());
    ASSERT_STR_EQUAL((char*)cache.getData(), "1234");
    cache = cache_manager.getLast();
    ASSERT_NOT_NULL(cache.getData());
    ASSERT_STR_EQUAL((char*)cache.getData(), "5678");
    ASSERT_INT_EQUAL(cache_manager.getCacheCount(), 10001);
    cache_manager.removeFirst(10);
    ASSERT_INT_EQUAL(cache_manager.getCacheCount(), 10001 - 10);
    cache_manager.removeLast(10);
    ASSERT_INT_EQUAL(cache_manager.getCacheCount(), 10001 - 10 - 10);
    com_file_remove("./test.db");

    CacheManager cache_manager2;
    cache_manager2.start("./test_d.db");
    cache_manager2.clear(true);
    for (int i = 0; i < 10000; i++)
    {
        Cache cache(std::to_string(i).c_str(), (uint8*)"1234", 5);
        cache_manager2.append(cache);
        if (i % 100 == 0)
        {
            LOG_D("progress %d%%", (i * 100) / 10000);
        }
    }

    //0,1,2,3,4,5,6,7,8,9,10
    cache_manager2.dilution(2);
    ASSERT_INT_EQUAL(cache_manager2.getCacheCount(), 6666);

    //1,2,4,5,7,8,10
    cache_manager2.dilution(2);
    ASSERT_INT_EQUAL(cache_manager2.getCacheCount(), 4444);

    //2,4,7,8
    cache_manager2.dilution(3);
    ASSERT_INT_EQUAL(cache_manager2.getCacheCount(), 3333);

    //4,7,8
    std::vector<Cache> vals = cache_manager2.getFirst(3);
    ASSERT_INT_EQUAL(vals.size(), 3);
    ASSERT_STR_EQUAL(vals[0].getUUID().c_str(), "4");
    ASSERT_STR_EQUAL(vals[1].getUUID().c_str(), "7");
    ASSERT_STR_EQUAL(vals[2].getUUID().c_str(), "8");

    cache_manager.clear(true);
    cache_manager2.clear(true);

    cache.setUUID("uuid");
    cache.setExpireTime(com_time_rtc_ms() + 5);
    cache.setRetryCount(4);
    cache.setData((uint8*)"data", 4);
    cache_manager.append(cache);

    Cache cache_bak = cache_manager.getByUUID("uuid");
    ASSERT_STR_EQUAL(cache_bak.getUUID().c_str(), "uuid");
    ASSERT_INT_EQUAL(cache_bak.getRetryCount(), 4);
    ASSERT_STR_EQUAL(cache_bak.toString().c_str(), "data");
    ASSERT_INT_EQUAL(cache_manager.getCacheCount(), 1);

    cache_manager.decreaseRetryCount("uuid", 4);
    ASSERT_INT_EQUAL(cache_manager.getCacheCount(), 0);
    cache_bak = cache_manager.getByUUID("uuid");
    ASSERT_TRUE(cache_bak.empty());

    com_file_remove("./test_d.db");
}