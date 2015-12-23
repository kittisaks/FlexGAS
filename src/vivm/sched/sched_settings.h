#ifndef _SCHED_SETTINGS_H
#define _SCHED_SETTINGS_H

#include "env_name.h"

char * scheds_get_shm_path();
char * scheds_get_rdma_port();
char * scheds_get_inet_port();
char * scheds_get_agent_rdma_port();
char * scheds_get_agent_inet_port();
bool   scheds_get_trace_all();
bool   scheds_get_trace();
bool   scheds_get_trace_err();

#endif //_SCHED_SETTINGS_H

