#include "api_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STRLEN 256

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

bool apis_shm_path_init = false;
char apis_shm_path[STRLEN];  

bool apis_rdma_port_init = false;
char apis_rdma_port[STRLEN];

bool apis_inet_port_init = false;
char apis_inet_port[STRLEN];

inline char * apis_get_shm_path(bool sched) {

    if (apis_shm_path_init)
        return apis_shm_path;

    char hostname[64];
    gethostname(hostname, 64);

    char * buf;
    if (sched)
        buf = getenv(ENV_SCHED_SHM_PATH);
    else
        buf = getenv(ENV_AGENT_SHM_PATH);

    if (buf == NULL) {
        buf = new char[STRLEN];
        char * buf_tmp = getcwd(buf, STRLEN);
        if (sched)
            snprintf(apis_shm_path, STRLEN, "%s/vivm_sched_%s",
                buf_tmp, hostname);
        else
            snprintf(apis_shm_path, STRLEN, "%s/vivm_agent_%s",
                buf_tmp, hostname);
        delete [] buf;
    }
    else {
        if (sched)
            snprintf(apis_shm_path, STRLEN, "%s/vivm_sched_%s",
                buf, hostname);
        else
            snprintf(apis_shm_path, STRLEN, "%s/vivm_agent_%s",
                buf, hostname);
    }

    apis_shm_path_init = true;

    return (char *) apis_shm_path;
}

char * apis_get_shm_path_sched() {

    return apis_get_shm_path(true);
}

char * apis_get_shm_path_agent() {

    return apis_get_shm_path(false);
}

#if 0
char * apis_get_rdma_port() {

    if (apis_rdma_port_init)
        return apis_rdma_port;

    char * buf = getenv(ENV_API_RDMA_PORT);
    if (buf == NULL) 
        snprintf(apis_rdma_port, STRLEN, "%s", DEF_API_RDMA_PORT);
    else
        snprintf(apis_rdma_port, STRLEN, "%s", buf);

    apis_rdma_port_init = true;

    return apis_rdma_port;
}

char * apis_get_inet_port() {

    if (apis_inet_port_init)
        return apis_inet_port;

    char * buf = getenv(ENV_API_INET_PORT);
    if (buf == NULL) 
        snprintf(apis_inet_port, STRLEN, "%s", DEF_API_INET_PORT);
    else
        snprintf(apis_inet_port, STRLEN, "%s", buf);

    apis_inet_port_init = true;

    return apis_inet_port;
}
#endif

bool apis_get_trace_all() {

    char * buf = getenv(ENV_API_ENABLE_TRACE_ALL);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

bool apis_get_trace() {

    char * buf = getenv(ENV_API_ENABLE_TRACE);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

bool apis_get_trace_err() {

    char * buf = getenv(ENV_API_ENABLE_TRACE_ERR);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

bool apis_get_is_descendant() {

    char * buf = getenv(ENV_API_IS_DESCENDANT);
    if (buf != NULL)
        return true;

    return false;
}

#ifdef __cplusplus
}
#endif //__cplusplus

