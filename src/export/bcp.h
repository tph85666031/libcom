#ifndef __BCP_H__
#define __BCP_H__

#include "bcp_com.h"
#include "bcp_cache.h"
#include "bcp_config.h"
#include "bcp_crc.h"
#include "bcp_sqlite.h"
#include "bcp_err.h"
#include "bcp_file.h"
#include "bcp_kv.h"
#include "bcp_kvs.h"
#include "bcp_md5.h"
#include "bcp_nmea.h"
#include "bcp_stack.h"
#include "bcp_log.h"
#include "bcp_serializer.h"
#include "bcp_base64.h"
#include "bcp_url.h"
#include "bcp_socket.h"
#include "bcp_thread.h"
#include "bcp_task.h"
#include "bcp_timer.h"
#include "bcp_ipc_tcp.h"
#include "bcp_ipc_ud.h"
#include "bcp_mem.h"
#include "bcp_dns.h"

#include "bcp_test.h"

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
