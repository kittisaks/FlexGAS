#define _SCHED
#include "simple_internal.h"
//#include "sched_internal.h"
#include <stdio.h>
#include <stdlib.h>

#define COMP_NAME "GROUP_HANDLER"

inline int init_group_handler(
    simp_group_context * context, sched_event job_event) {

    CHECK_ZF(   schedCreateSchedHandle(&(context->event_handle)),
        "Cannot create handle for group events.");
    CHECK_ZF(   schedRegisterHandle(context->event_handle, SCHED_INST_GROUP,
        job_event.jid, job_event.gid, 
        GROUP_EVENT_PE_EXIT | GROUP_EVENT_PE_KILLED),
        "Cannot register handle for group events.");

    context->job_event = job_event;

    return 0;
}

int allocate_compute_resources(sched_group group,
    sched_device ** device, int * num_devices) {

#if 0
#define NUM_RES 2
    int idx, num_devices_tmp = NUM_RES;
    const char hostname[][32] = {"ping0", "ping4", "ping2",
        "ping1", "ping3", "ping5"};
#undef NUM_RES
#else
    char ** hostname;
    int     idx, num_devices_tmp = 0;
    CHECK_ZF(   simp_get_node_list(&hostname, &num_devices_tmp),
        "Cannot get device list");
#endif

    sched_device * device_tmp = malloc(num_devices_tmp * sizeof(sched_device));
    for (idx=0;idx<num_devices_tmp;idx++) {
        CHECK_ZF(   schedCreateDevice(hostname[idx], sched_link_inet,
            ivm_cpu, 0, group, &device_tmp[idx]),
            "Cannot create device.");
    }

    *device = device_tmp;
    *num_devices = num_devices_tmp;

    return 0;

}

int deallocate_compute_resources(
    sched_device * device, int f) {

    int idx;
    for (idx=0;idx<f;idx++)
        CHECK_ZF(   schedDestroyDevice(device[idx]),
            "Cannot deallocate compute devices.");

    free(device);

    return 0;

}

void * simp_group_routine(void * arg) {

    simp_group_context * context      = (simp_group_context *) arg;
    sched_device       * device       = NULL;
    int                  num_devices;

    allocate_compute_resources(context->job_event.group,
        &device, &num_devices);

    sched_event event;
    sched_pe    pe[100];

//#define _KILL_PES
#if defined(_KILL_PES)
    sched_pe    killed_pe[100];
    int iter = 0, killed;
#endif

    int tot_pe = 0;
    do {
        int ret, idx, success = 0;
//        int jIdx;
        for (idx=0;idx<num_devices;idx++) {

//            for (jIdx=0;jIdx<4;jIdx++) {
            ret = schedSpawnPes(
                device[idx], context->job_event.comp,  1, &pe[success]);
            if ((ret == 0) && (tot_pe < 
                context->job_event.comp->config.max_pes_num)) {
                tot_pe++;
                success++;
            }
            else if (ret == -1)
                break;
//            }
        }

#if  defined(_KILL_PES)
        if (iter == 0) {
            killed = success;
            for (idx=0;idx<success;idx++) {
                CHECK_Z(    schedKillPes(pe[idx]),
                    "Failed to kill PEs.");
                killed_pe[idx] = pe[idx];
            }
        }
        iter++;
#endif

        for (idx=0;idx<success;idx++) {
            CHECK_Z(    schedWaitEvent(context->event_handle, &event),
                "Failed to retrieve a group event.");
            TRACE("Pe exit (%d - %x)", event.peid, event.type);
        }
    } while(tot_pe < context->job_event.comp->config.max_pes_num);
//    } while(event.peid < context->job_event.comp->config.max_pes_num);

#if  defined(_KILL_PES)
        int idx;
        for (idx=0;idx<killed;idx++) {
            CHECK_Z(    schedRespawnPes(device[0], killed_pe[idx]),
                "Failed to respawn PEs.");
        }
        for (idx=0;idx<killed;idx++) {
            CHECK_Z(    schedWaitEvent(context->event_handle, &event),
                "Failed to retrieve a group event.");
            TRACE("Pe exit (%d - %x)", event.peid, event.type);
        }
#endif

    CHECK_Z(    schedAckEvent(context->job_event),
        "Failed to acknowledge job event.");

    deallocate_compute_resources(device, num_devices);

    return NULL;
}

int simp_group_handler(simp_job_context * job_context, sched_event job_event) {

    simp_group_context * context = malloc(sizeof(simp_group_context));
    init_group_handler(context, job_event);

    pthread_attr_t attr;
    pthread_t      thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread, &attr, simp_group_routine, (void *) context);

    job_context->group_handler = thread;

    return 0;
}

