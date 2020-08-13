#ifndef __BCP_H__
#define __BCP_H__

#include "com_com.h"
#include "com_cache.h"
#include "com_config.h"
#include "com_crc.h"
#include "com_sqlite.h"
#include "com_err.h"
#include "com_file.h"
#include "com_kv.h"
#include "com_kvs.h"
#include "com_md5.h"
#include "com_nmea.h"
#include "com_stack.h"
#include "com_log.h"
#include "com_serializer.h"
#include "com_base64.h"
#include "com_url.h"
#include "com_socket.h"
#include "com_thread.h"
#include "com_task.h"
#include "com_timer.h"
#include "com_ipc_tcp.h"
#include "com_ipc_ud.h"
#include "com_mem.h"
#include "com_dns.h"

#include "com_test.h"

#include "CJsonObject.h"
#include "minmea.h"
#include "sqlite3.h"
#include "tinyxml2.h"

class BcpInitializer
{
public:
    BcpInitializer();
    ~BcpInitializer();
    static void ManualInit();
};

#endif /* __BCP_H__ */
