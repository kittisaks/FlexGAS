#ifndef _API_SETTINGS_H
#define _API_SETTINGS_H

#include "env_name.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

char * apis_get_shm_path_sched();
char * apis_get_shm_path_agent();
//char * apis_get_rdma_port();
//char * apis_get_inet_port();
bool   apis_get_trace_all();
bool   apis_get_trace();
bool   apis_get_trace_err();
bool   apis_get_is_descendant();

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_API_SETTINGS_H

