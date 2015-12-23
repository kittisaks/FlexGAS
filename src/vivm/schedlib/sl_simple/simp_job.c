#define _SCHED
#include "simple_internal.h"
#include <stdio.h>
#include <stdlib.h>

#define COMP_NAME "JOB_HANDLER"

inline int init_job_handler(
    simp_job_context * context, sched_event sys_event) {

    CHECK_ZF(   schedCreateSchedHandle(&(context->event_handle)),
        "Cannot create handle for group events.");
    CHECK_ZF(   schedRegisterHandle(context->event_handle, SCHED_INST_JOB,
        sys_event.jid, 0, JOB_EVENT_START_COMP |
        JOB_EVENT_PAUSE_COMP | JOB_EVENT_STOP_COMP),
        "Cannot register handle for group events.");

    context->sys_event = sys_event;

    return 0;
}

void * simp_job_routine(void * arg) {

    simp_job_context * context = (simp_job_context *) arg;

    sched_event event;
    do {
        CHECK_Z(    schedWaitEvent(context->event_handle, &event),
            "Failed to retrieve a group event.");
        switch (event.type) {
        case JOB_EVENT_START_COMP:
            simp_group_handler(context, event);
            break;
        case JOB_EVENT_PAUSE_COMP:
            break;
        case JOB_EVENT_STOP_COMP:
            break;
        case JOB_EVENT_EXEC_COMPLETE:
            break;
        }
    } while(1);

    return NULL;
}

int simp_job_handler(simp_context * sys_context, sched_event sys_event) {

    simp_job_context * context = malloc(sizeof(simp_job_context));
    init_job_handler(context, sys_event);

    pthread_attr_t attr;
    pthread_t      thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread, &attr, simp_job_routine, (void *) context);

    sys_context->job_handler = thread;

    return 0;
}

