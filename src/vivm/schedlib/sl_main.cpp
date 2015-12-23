#define _SCHED_MAIN
#include "sched_api.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int sched_enter() {

    return 0;
}

int sched_main() {

    schedPrintf("SCHEDLIB LOADED...");

    void * sys_handle;
    schedCreateSchedHandle(&sys_handle);
    schedRegisterHandle(sys_handle, SCHED_INST_SYS, 0, 0,
        SYS_EVENT_START_JOB | SYS_EVENT_PAUSE_JOB | SYS_EVENT_STOP_JOB);

    int ret, idx=0;

        sched_event event;
        schedWaitEvent(sys_handle, &event);
        schedPrintf("JOB RECEIVED: %x (%d)", event.type, idx++);
        schedAckEvent(event);

        void * job_handle;
        schedCreateSchedHandle(&job_handle);
        schedRegisterHandle(job_handle, SCHED_INST_JOB, event.jid, 0,
            JOB_EVENT_START_COMP);

    do {
        schedWaitEvent(job_handle, &event);
        schedPrintf("JOB EVENT: %x - %d", event.type, event.jid);

#define NUM_RES 3
        sched_device   device[NUM_RES];
        char           hostname[10][32] = 
            {"ping0", "ping1", "ping2"};

        for (int idx=0;idx<NUM_RES;idx++) {
            ret = schedCreateDevice(hostname[idx], sched_link_inet, ivm_cpu, 0,
                event.group, &device[idx]);
            if (ret != 0)
                schedPrintfe("Error creating device");
            else
                schedPrintf("Successfully created device");
        }

#if 1
        void * group_handle;
        schedCreateSchedHandle(&group_handle);
        schedRegisterHandle(group_handle, SCHED_INST_GROUP,
            event.jid, event.gid, GROUP_EVENT_PE_EXIT);


        sched_event g_event; int idx = 0;
        do {
            for (int jIdx=0;jIdx<NUM_RES;jIdx++)
                schedSpawnPes(device[jIdx], event.comp, 1, NULL);

            for (int jIdx=0;jIdx<NUM_RES;jIdx++) {
                schedWaitEvent(group_handle, &g_event);
                schedPrintf("PEID: %d", g_event.peid);
            }
            idx++;
        } while(idx<33);


        for (idx=0;idx<NUM_RES;idx++)
            schedDestroyDevice(device[idx]);

        schedAckEvent(event);
#endif
    } while (1);

    return 0;
}

int sched_exit() {

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

