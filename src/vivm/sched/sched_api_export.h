#ifndef _SCHEDULER_API_EXPORT_H
#define _SCHEDULER_API_EXPORT_H

#include "sched_req_common.h"

#define UNINIT_PID  0
#define UNINIT_PEID 0
#define UNINIT_DID  0
#define UNINIT_HID  0
#define UNINTI_GID  0
#define UNINIT_JID  0

#define SCHED_API_ROOT_PEID 0xffffffff
#define SCHED_API_ROOT_DID  0xffffffff
#define SCHED_API_ROOT_HID  0xffffffff
#define SCHED_API_ROOT_GID  0xffffffff

typedef enum {
    saa_register_pe,
    saa_unregister_pe,
    saa_register_comp,
    saa_configure_comp,
    saa_launch_comp,
    saa_mem_alloc,
    saa_mem_map,
    saa_mem_put,
    saa_mem_get,
    saa_mem_update
//    ,saa_spawn_pe //This will be used for PEs to spawn a next PE to draw work.
} saa_req_type;

typedef struct {
    saa_req_type      type;

    //saa_register_pe
    int               argc;
    char              argv[MAX_STR_LEN];
    char              path[MAX_STR_LEN];
    char              bin[MIN_STR_LEN];
    ivm_comp          comp_desc_exec;
    int               rec_level;

    //saa_register_comp & saa_configure_comp
    char              comp_id[MED_STR_LEN];
    ivm_comp          comp_fn_ptr;
    ivm_comp_config   comp_config;
    ivm_device_type   device_type;
    uint32_t          phy_dev_id;

    //saa_mem_alloc & saa_mem_put & saa_mem_get
    int               mem_flags;
    char              mem_id[MED_STR_LEN];
    void            * mem_addr;
    uint64_t          mem_offset;
    uint64_t          mem_size;
    ch_mr_t           mr;

} saa_req_data;

typedef struct sched_req_st<saa_req_data> saa_req;

#endif //_SCHED_API_EXPORT_H

