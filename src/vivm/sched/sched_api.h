#ifndef _SCHED_API_H
#define _SCHED_API_H

#include "ivm.h"
#include "sched_common.h"
#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/******************************************************************************
 * Data types used by Scheduling Library (SL)
 *****************************************************************************/

#define SCHED_INST_SYS            0x10000000
#define SCHED_INST_JOB            0x20000000
#define SCHED_INST_GROUP          0x40000000
//#define SCHED_JOB_ANY             0xffffffff
//#define SCHED_GROUP_ANY           0xffffffff

#define SYS_EVENT_START_JOB       0x10000001
#define SYS_EVENT_PAUSE_JOB       0x10000002
#define SYS_EVENT_STOP_JOB        0x10000004
#define JOB_EVENT_START_COMP      0x20000001
#define JOB_EVENT_PAUSE_COMP      0x20000002
#define JOB_EVENT_STOP_COMP       0x20000004
#define JOB_EVENT_EXEC_COMPLETE   0x20000008
#define GROUP_EVENT_PE_ENTER      0x40000001
#define GROUP_EVENT_PE_EXIT       0x40000002
#define GROUP_EVENT_PE_RESTARTED  0x40000004
#define GROUP_EVENT_PE_KILLED     0x40000008
#define GROUP_EVENT_EXEC_COMPLETE 0x40000010

typedef struct   sched_job_st    sched_job_t,     * sched_job;
typedef struct   sched_group_st  sched_group_t,   * sched_group;
typedef struct   sched_device_st sched_device_t,  * sched_device;
typedef struct   sched_pe_st     sched_pe_t,      * sched_pe;
typedef struct   sched_comp_st   sched_comp_t,    * sched_comp;
typedef uint32_t sched_event_mask;
typedef uint32_t sched_event_type;
typedef uint32_t sched_instance;

typedef enum {
    sched_event_wait_none,
    sched_event_busy_wait,
    sched_event_sleep_wait
} sched_event_ack_type;

struct sched_comp_st {
    char            comp_id[MED_STR_LEN];
    ivm_comp        ptr;
    int             is_configured;
    ivm_comp_config config;
};

typedef struct {
    sched_event_type       type;
    uid_t                  uid;
    pid_t                  pid;
    job_id                 jid;
    group_id               gid;
    host_id                hid;
    device_id              did;
    pe_id                  peid;
    sched_job              job;
    sched_group            group;
    sched_device           device;
    sched_comp             comp;
    sched_event_ack_type   ack_type;
    int                  * ack;
    pthread_cond_t       * sack;
    pthread_mutex_t      * sack_mtx;
} sched_event;

typedef enum {
    intrn_ref_job_table,
} intrn_ref;

typedef enum {
    sched_link_rdma,
    sched_link_inet
} sched_link;

//Function types

#define SCHED_MAIN_NAME "sched_main"

typedef int (* fnt_sched_main) ();

typedef int (* fnt_schedGetDeviceList) ();

typedef int (* fnt_schedGetInternalReference)
    (intrn_ref, void **);

typedef int (* fnt_schedCreateSchedHandle) (void **);

typedef int (* fnt_schedRegisterHandle) 
    (void *, sched_instance, job_id, group_id, sched_event_mask);

typedef int (* fnt_schedWaitEvent) (void * handle, sched_event *);

typedef int (* fnt_schedPollEvent) (void * handle, sched_event *);

typedef int (* fnt_schedAckEvent) (sched_event);

typedef int (* fnt_schedPrintf) (const char *, ...);

typedef int (* fnt_schedPrintfe) (const char *, ...);

typedef int (* fnt_schedStartComputation) ();

typedef int (* fnt_schedCreateDevice) (const char *, sched_link,
    ivm_device_type, int, sched_group, sched_device *);

typedef int (* fnt_schedDestroyDevice) (sched_device);

typedef int (* fnt_schedDestroyGroup) (sched_group);

typedef int (* fnt_schedDestroyJob) (sched_job);

typedef int (* fnt_schedSpawnPes) (sched_device, sched_comp,
    int, sched_pe *);

typedef int (* fnt_schedKillPes) (sched_pe);

typedef int (* fnt_schedKillGroup) ();

typedef int (* fnt_schedRespawnPes) (sched_device, sched_pe);

/*****************************************************************************
 * Application Programming Interface used by Scheduling Library (SL)
 * ===========================================================================
 * Note: The scheduling API is designed to mainly support scheduling libraries
 *       written with a multi-threading style. It is recommeded to write a
 *       scheduling library such that different threads handle different jobs
 *       and groups (1-to-1 mapping of Job/Group and thread).
 ****************************************************************************/

#if !defined(_SCHED_MAIN) && !defined(_SCHED)

int schedGetDeviceList();

int schedGetInternalReference(intrn_ref ref_type, void ** ref);

int schedCreateSchedHandle(void ** handle);

int schedRegisterHandle(void * handle, sched_instance inst,
    job_id jid, group_id gid, sched_event_mask mask);

int schedWaitEvent(void * handle, sched_event * event);

int schedPollEvent(void * handle, sched_event * event);

int schedAckEvent(sched_event event);

int schedPrintf(const char * format, ...);

int schedPrintfe(const char * format, ...);

//Use built-in load balancing mechanism (DS-LB)
//TODO: Should this be included???
int schedStartComputation();

//Manually perform load balance at a fine-grain

int schedCreateDevice(const char * host, sched_link link,
    ivm_device_type dev_type, int phy_dev_id, 
    sched_group group, sched_device * device);

int schedDestroyDevice(sched_device device);

int schedDestroyGroup(sched_group group);

int schedDestroyJob(sched_job job);

int schedSpawnPes(sched_device device, sched_comp comp,
    int num_pes, sched_pe * pe);

int schedKillPes(sched_pe pe);

int schedKillGroup();

int schedRespawnPes(sched_device device, sched_pe pe);

#define CHECK_Z(statement, e_format, ...)       \
    do {                                        \
        if (statement != 0) {                   \
            TRACE_ERR(e_format, ##__VA_ARGS__); \
            return -1;                          \
        }                                       \
    }  while (0)

#else //!defined(_SCHED_MAIN) && !defined(_SCHED)
#include "sched_lib.h"
#endif //!defined(_SCHED_MAIN) && !defined(_SCHED)

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SCHED_API_H

