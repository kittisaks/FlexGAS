#ifndef _SCHED_INTERNAL_H
#define _SCHED_INTERNAL_H

#include "sched_api_export.h"
#include "sched_agent_export.h"
#include "channel.h"
#include "ts_map.h"
#include <vector>
#include <map>
#include <string>

/*****************************************************************************
 * Data structure mainly used by SchedLocalHandler class
 ****************************************************************************/

/**
 * Job --> Group --> Device --> PeList
 *
 * Job    - Job submitted from a user
 * Group  - A collection of physical compute resources executing a computation
 * Device - Compute device executing used for executing a computation
 * PeList - A collection of process elements/tasks executing a 
 *          computation on a device
 */

/**
 * The scheduler and agent must be able to handle both types of data
 * structures: master-/mirror-copies.
 */

//Intra-application scheduling
typedef struct sched_pe_st     sched_pe_t,     * sched_pe;
typedef struct sched_device_st sched_device_t, * sched_device;
typedef struct sched_group_st  sched_group_t,  * sched_group;
typedef struct sched_job_st    sched_job_t,    * sched_job;
typedef struct sched_mem_et_st sched_mem_et_t, * sched_mem_et;

typedef std::map<group_id, sched_group>     group_table;
typedef group_table::iterator               group_iter;
typedef std::map<device_id, sched_device>   device_table;
typedef device_table::iterator              device_iter;
typedef std::map<pe_id, sched_pe>           pe_table;
typedef pe_table::iterator                  pe_iter;
typedef std::map<std::string, sched_comp>   comp_table;
typedef comp_table::iterator                comp_iter;
typedef std::map<std::string, sched_mem_et> memory_table;
typedef memory_table::iterator              meme_iter;

/* Event notification */
typedef struct {
    pthread_mutex_t          * mutex;
    pthread_cond_t           * cond;
    sched_event_mask           mask;
    std::vector<sched_event>   queue;
} sched_tnotif_t, * sched_tnotif;

typedef enum {
    mem_master,
    mem_mirror,
    mem_record  //mem_record exists only in the sched.
} sched_mem_copy_type;

typedef struct {
    sched_device device;
    ch_mr        mr;
} sched_device_mr;

/* Memory Entry - describe the attributes, localtion of a data structure     */
struct sched_mem_et_st {

    char                  iden[MAX_STR_LEN]; 
    /* The decorated identifier of the structure     */

    sched_mem_copy_type   copy_type; 
    /* The field indicating whether the master-copy
        of the structure is located                  */

    void                * local_ptr;
    /* The local reference to the structure          */

    ch_mr                 mr;
    /* Memory region describing low-level low-level
       memory structure (see channel.h and lm.h).
       A memory regions is meant to be used as an
       instance for exchanging memory information
       between processes.                            */

    job_id                jid;
    group_id              gid;
    /* Global identifiers of the job and group to    
       to which the structure belongs                */

    device_id             master_did;
    /* Global identifier of the device to which
       the master-copy of the structure belongs      */ 

    sched_job             job;
    /* The job to which the structure belongs.
       The reference to the job may not be the
       same on different compute nodes/resources.    */

    sched_group           group;
    /* The group to which the structure belongs      
       The reference to the group may not be the
       same on different compute nodes/resources.    */

    sched_device          master_device; 
    /* Physical location of the master-copy of 
       data structure in term of 'sched_device'    
       The reference to the device may not be the
       same on different compute nodes/resource.     */

    std::vector<sched_device_mr> remote_device;
    /* List of sched_devices holding the mirror-
       copies of the data structure. The reference
       to the device may not be the same on
       different compute nodes/resource. This is
       valid only for the device that holds the 
       master-copy, otherwise NULL.                  */

    pthread_mutex_t       entry_mtx;
};

struct sched_pe_st {
    pid_t           pid;
    uid_t           uid;
    job_id          jid;
    group_id        gid;
    host_id         hid;
    device_id       did;
    pe_id           peid;

    sched_comp      comp;
    sched_device    device;
    sched_group     group;
    sched_job       job;

    int             is_killed;
};

struct sched_device_st {
    job_id          jid;
    group_id        gid;
    device_id       did;
    pid_t           pid;
    uid_t           uid;

    typedef struct {
        ch              service_ch;
        ch_init_attr    service_attr;
        ch              request_ch;
        ch_init_attr    request_attr;
        pthread_mutex_t pem_mtx;
    } internal;
    internal        intrn;
 
    typedef struct {
        ivm_device_type type;
        char            host[MED_STR_LEN];
        char            service[MED_STR_LEN];
        ch_link         link;
        uint32_t        phy_dev_id;
    } phy_info;
    phy_info        phy;

    pe_table        pe_tab;
    sched_group     group;
    sched_job       job;
};

struct sched_group_st {
    job_id             jid;
    group_id           gid;
    uid_t              uid;
    pid_t              pid;
    volatile device_id did_cnt;
    volatile pe_id     peid_cnt;
    int                rec_level;

    typedef struct {
        pthread_mutex_t   pem_mtx;
        pthread_mutex_t   mm_mtx;
        pthread_mutex_t   cm_mtx;
        sched_tnotif      handle;
    } internal;
    internal           intrn;

    //Computation Management
    comp_table         comp_tab;
    //TODO: We will enable launching nested launching by redirecting
    //      requests from agents to sched.

    //Process Management
    device_table    device_tab;
    pe_table        pe_tab;        /**/
    pe_table        killed_pe_tab;
    sched_job       job;

    //Memory Management
    memory_table    mem_tab;
};

struct sched_job_st {
    job_id            jid;
    uid_t             uid;
    pid_t             pid;
    volatile group_id gid_cnt;
    int               argc;
    char              argv[MAX_STR_LEN];
    char              path[MAX_STR_LEN];
    char              bin[MIN_STR_LEN];

    typedef struct {
        pthread_mutex_t   pem_mtx;
        pthread_mutex_t   mm_mtx;
        pthread_mutex_t   cm_mtx;
        sched_tnotif      handle;
    } internal;
    internal          intrn;

    //Computation Management
    comp_table        comp_tab;

    //Process Management
    group_table       group_tab;

    //Memory Management
    memory_table      mem_tab;
};

//Inter-application scheduling
typedef std::map<job_id, sched_job> job_table;
typedef job_table::iterator         job_iter;

extern char               local_hostname[MED_STR_LEN];
extern sched_tnotif       sys_handle;
extern job_table          job_tab;
extern pthread_mutex_t    job_tab_mutex;
extern volatile job_id    job_id_cnt;

//Insertion is a top-down operation.
//Deletion is a top-down operation.
//Because scheduling action, registration, finalization, an instance will
//always provide information back to the server.

#include "sched_helpers.h"

#endif //_SCHED_INTERNAL_H

