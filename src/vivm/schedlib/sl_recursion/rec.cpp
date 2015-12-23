#define _SCHED_MAIN
#include "rec_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>

#define COMP_NAME "SYS_HANDLER"

//Data structure necessary for management at the system level
char             ** node_list               = NULL;
int                 node_count              = 0;
bool                node_list_available     = false;
resource         *  resource_list           = NULL;
int                 resource_count          = 0;
bool                resource_list_available = false;
pthread_mutex_t     resource_list_mtx;

sys_context    sys_ctx;

AtomicVector<sched_event>               q_sys_event;
AtomicMap<sched_event *, sched_event *> r_sys_event;
thread_t                                th_sys_sched;

void * sys_sched_routine(void *);

void init_sched() {

    get_node_list();

    //Initialize the resource list
    resource_list = new resource 
        [(CPUS_PER_NODE + GPUS_PER_NODE) * node_count];

    int offset = 0;
    for (int iIdx=0;iIdx<node_count;iIdx++) {

        for (int jIdx=0;jIdx<CPUS_PER_NODE;jIdx++) {
            snprintf(resource_list[offset].hostname, 64, "%s", node_list[iIdx]);
            resource_list[offset].type = ivm_cpu;
            resource_list[offset].occupant_pes.clear();
            resource_list[offset].num_occupants = 0;
            pthread_mutex_init(&(resource_list[offset].lock), NULL);
            offset++;
        }

        for (int jIdx=0;jIdx<GPUS_PER_NODE;jIdx++) {
            snprintf(resource_list[offset].hostname, 64, "%s", node_list[iIdx]);
            resource_list[offset].type = ivm_gpu;
            resource_list[offset].occupant_pes.clear();
            resource_list[offset].num_occupants = 0;
            pthread_mutex_init(&(resource_list[offset].lock), NULL);
            offset++;
        }
    }
    resource_count = offset;
    resource_list_available = true;
    pthread_mutex_init(&resource_list_mtx, NULL);

    thread_create(&th_sys_sched, sys_sched_routine, (void *) &th_sys_sched);
}

int init_sys_handler() {

    CHECK_ZF(   schedCreateSchedHandle(&(sys_ctx.event_handle)),
        "Error at line: %d", __LINE__);
    CHECK_ZF(   schedRegisterHandle(sys_ctx.event_handle,
        SCHED_INST_SYS, 0, 0, SYS_EVENT_START_JOB),
        "Error at line: %d", __LINE__);

    return 0;
}

int wait_event(sched_event * event) {

    CHECK_ZF(   schedWaitEvent(sys_ctx.event_handle, event),
        "Error at line: %d", __LINE__);
    CHECK_ZF(   schedAckEvent(*event),
        "Error at line: %d", __LINE__);

    return 0;
}

void * sys_sched_routine(void * arg) {

    thread self = (thread) arg;

    thread_sleep(self);

    do {
        q_sys_event.lock(); {
            //Wait for incoming job launch
            if (q_sys_event.size_unsafe() == 0) {
                q_sys_event.unlock();
                thread_sleep(self);
                q_sys_event.lock();
            }

            sched_event * event = new sched_event();
            q_sys_event.fetch_unsafe(event, 0);

//            r_sys_event.insert_unsafe(std::make_pair(event, event));
            thread_t * th_job_sched = new thread_t();
            th_event * th_ev        = new th_event();
            th_ev->th               = th_job_sched;
            th_ev->event            = event;
            thread_create(th_job_sched, job_sched_routine, (void *) th_ev);
        }
        q_sys_event.unlock();
        usleep(100000);
    } while(1);

    pthread_exit(NULL);
}

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int sched_main() {

    TRACE("*** Recursion Scheduler Loaded ***");
    init_sched();

    if (init_sys_handler() != 0) {
        TRACE("Error!");
        exit(-1);
    }

    do {
        sched_event event;
        wait_event(&event);
        q_sys_event.push_back(event);
        thread_wake(&th_sys_sched);
    } while(1);

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

