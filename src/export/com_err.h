#ifndef __BCP_ERR_H__
#define __BCP_ERR_H__

typedef enum : uint16
{
    ERR_OK = 0,
    ERR_FAILED = 1,
    ERR_TIMEOUT = 2,
    ERR_CONN = 3,
    ERR_ARG = 4,
    ERR_MALLOC = 5,
    ERR_IGNORE_ACK = 6,
} ERRCODE;

#endif /* __BCP_ERR_H__ */