#ifndef _AGENT_SETTINGS_H
#define _AGENT_SETTINGS_H

#include "env_name.h"

char * agts_get_shm_path();
char * agts_get_rdma_port();
char * agts_get_inet_port();
char * agts_get_sched_rdma_port();
char * agts_get_sched_inet_port();
char * agts_get_agent_rdma_port();
char * agts_get_agent_inet_port();
bool   agts_get_trace_all();
bool   agts_get_trace();
bool   agts_get_trace_err();

#endif //_AGENT_SETTINGS_H

