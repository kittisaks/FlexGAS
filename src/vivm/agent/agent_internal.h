#ifndef _AGENT_INTERNAL_H
#define _AGENT_INTERNAL_H

#include "ts_map.h"
#include "sched_internal.h"
#include <pthread.h>
#include <unistd.h>

#define PE_PENDING 0
#define PE_SPAWNED 1

typedef struct {
    sched_job      job;
    sched_group    group;
    sched_device   device;
    pid_t          pid;
    uid_t          uid;
    job_id         jid;
    group_id       gid;
    host_id        hid;
    device_id      did;
    pe_id          peid;
    int            rec_level;
    sched_comp     comp;
    ivm_comp       comp_desc_exec;
    pthread_t      responder;
    volatile char         * notif;
    uint32_t       notif_idx;
} prosp_pe;

extern job_table job_tab;

typedef ts_map<pid_t, prosp_pe> prosp_pe_table;

extern prosp_pe_table prosp_pe_tab;

#endif //_AGENT_INTERNAL_H

