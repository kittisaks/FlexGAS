#ifndef _SCHED_REQ_COMMON
#define _SCHED_REQ_COMMON

#include "channel.h"
#include "ivm.h"
#include "sched_common.h"
#include <stdint.h>
#include <unistd.h>

typedef uint32_t host_id, req_id;

typedef enum {
    local = 0x0000,
    remote
} req_type;

typedef enum {
    success = 0x0000, 
    initial,
    in_progress, 
    failed
} req_status;

/**
 * This serves as a template or a basic structure for internal request
 * made by various daemons. The size of the structure should not exceed
 * the size of MTU.
 */
template <typename T> struct sched_req_st {
    //Request information
    req_type       type;
    req_id         req_iden;
    req_status     status;

    //PEs information
    pid_t          pid;
    uid_t          uid;
    char           host[MED_STR_LEN];
    pe_id          peid;
    device_id      did;
    host_id        hid;
    group_id       gid;
    job_id         jid;

    //Request-specific information
    T              data;
};


#endif //_SCHED_REQ_COMMON

