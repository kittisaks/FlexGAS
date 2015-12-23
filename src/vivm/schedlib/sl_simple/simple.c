#define _SCHED_MAIN
#include "simple_internal.h"
#include <stdio.h>
#include <stdlib.h>

#define COMP_NAME "SYS_HANDLER"

int     node_list_available = 0;
char ** node_list  = NULL;
int     node_count = 0;

inline int init_sys_handler(simp_context * context) {

    CHECK_ZF(    schedCreateSchedHandle(&(context->event_handle)),
        "Cannot create handle for system events.");
    CHECK_ZF(    schedRegisterHandle(context->event_handle,
        SCHED_INST_SYS, 0, 0, SYS_EVENT_START_JOB |
        SYS_EVENT_PAUSE_JOB | SYS_EVENT_STOP_JOB),
        "Cannot register handle for system events.");

    return 0;
}

inline int wait_event(simp_context * context, sched_event * event) {

    CHECK_ZF(    schedWaitEvent(context->event_handle, event),
        "Failed to retrieve a system event");
    CHECK_ZF(    schedAckEvent(*event),
        "Failed to acknowledge sys event");

    return 0;
}

int sched_main() {

    TRACE("*** Simple Scheduler Loaded ***");

    simp_context * context = malloc(sizeof(simp_context));
    init_sys_handler(context);

    sched_event event;
    do {
        wait_event(context, &event);
        simp_job_handler(context, event);
    } while(1);

    return 0;
}


