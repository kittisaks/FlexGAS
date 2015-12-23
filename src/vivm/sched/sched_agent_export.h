#ifndef _SCHEDULER_AGENT_EXPORT_H
#define _SCHEDULER_AGENT_EXPORT_H

#include "ivm.h"
#include "sched_req_common.h"
#include "sched_api.h"

#define MAX_PEID_CNT 32

typedef enum {
    sag_agent_register_job,
    sag_agent_register_group,
    sag_agent_register_device,
    sag_agent_register_jgd,
    sag_agent_unregister_device,
    sag_agent_unregister_group,
    sag_agent_unregister_job,
    sag_agent_spawn_pes,
    sag_agent_kill_pes,
    sag_agent_query_memory,
    sag_sched_register_agent,
    sag_sched_unregister_agent,
    sag_sched_notify_pe_done,
    sag_sched_add_memory_entry,
    sag_sched_remove_memory_entry,
    sag_sched_query_memory,
    sag_sched_launch_comp
} sag_req_type;

typedef struct {

    sag_req_type    type;

    //sag_agent_register_job
    int             argc;
    char            argv[MAX_STR_LEN];
    char            path[MAX_STR_LEN];
    char            bin[MIN_STR_LEN];

    //sag_agent_register_group & sag_agent_unregister_group
    group_id        gid;
    sched_comp_t    comp; //sag_spawn_pes
    int             rec_level;

    //sag_agent_register_device & sag_agent_unregister_device &
    //sag_sched_query_memory
    device_id       did;
    char            host[MED_STR_LEN];
    ch_link         link;
    ivm_device_type device_type;
    int             phy_dev_id;

    //sag_agent_spawn_pes
    int             num_pes;
    ivm_device_type device_type_pes;
    uint32_t        phy_dev_id_pes;
    host_id         hid;
    pe_id           s_peid;
    pe_id           e_peid;

    //sag_sched_add_memory_entry & sag_query_memory
    int             mem_flags;
    char            mem_id[MED_STR_LEN];
    uint64_t        mem_size;
    bool            mr_valid;

    //sag_launch_comp
    char            comp_id[MED_STR_LEN];
    ivm_comp_config comp_config;

} sag_req_data;

typedef struct sched_req_st<sag_req_data> sag_req;

#endif //_SCHED_AGENT_EXPORT_H

