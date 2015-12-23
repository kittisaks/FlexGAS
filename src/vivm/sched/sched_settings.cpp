#include "sched_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STRLEN 256

bool scheds_shm_path_init  = false;
char scheds_shm_path[STRLEN];  

bool scheds_rdma_port_init = false;
char scheds_rdma_port[STRLEN];

bool scheds_inet_port_init = false;
char scheds_inet_port[STRLEN];

bool scheds_agt_rdma_port_init  = false;
char scheds_agt_rdma_port[STRLEN];

bool scheds_agt_inet_port_init  = false;
char scheds_agt_inet_port[STRLEN];

char * scheds_get_shm_path() {

    if (scheds_shm_path_init)
        return scheds_shm_path;

    char hostname[64];
    gethostname(hostname, 64);

    char * buf = getenv(ENV_SCHED_SHM_PATH);
    if (buf == NULL) {
        buf = new char[STRLEN];
        char * buf_tmp = getcwd(buf, STRLEN);
        snprintf(scheds_shm_path, STRLEN, "%s/vivm_sched_%s",
            buf_tmp, hostname);
        delete [] buf;
    }
    else {
        snprintf(scheds_shm_path, STRLEN, "%s/vivm_sched_%s",
            buf, hostname);
    }

    scheds_shm_path_init = true;

    return (char *) scheds_shm_path;
}

char * scheds_get_rdma_port() {

    if (scheds_rdma_port_init)
        return scheds_rdma_port;

    char * buf = getenv(ENV_SCHED_RDMA_PORT);
    if (buf == NULL) 
        snprintf(scheds_rdma_port, STRLEN, "%s", DEF_SCHED_RDMA_PORT);
    else
        snprintf(scheds_rdma_port, STRLEN, "%s", buf);

    scheds_rdma_port_init = true;

    return scheds_rdma_port;
}

char * scheds_get_inet_port() {

    if (scheds_inet_port_init)
        return scheds_inet_port;

    char * buf = getenv(ENV_SCHED_INET_PORT);
    if (buf == NULL) 
        snprintf(scheds_inet_port, STRLEN, "%s", DEF_SCHED_INET_PORT);
    else
        snprintf(scheds_inet_port, STRLEN, "%s", buf);

    scheds_inet_port_init = true;

    return scheds_inet_port;
}

char * scheds_get_agent_rdma_port() {

    if (scheds_agt_rdma_port_init)
        return scheds_agt_rdma_port;

    char * buf = getenv(ENV_AGENT_RDMA_PORT);
    if (buf == NULL)
        snprintf(scheds_agt_rdma_port, STRLEN, "%s", DEF_AGENT_RDMA_PORT);
    else
        snprintf(scheds_agt_rdma_port, STRLEN, "%s", buf);

    scheds_agt_rdma_port_init = true;

    return scheds_agt_rdma_port;
}

char * scheds_get_agent_inet_port() {

    if (scheds_agt_inet_port_init)
        return scheds_agt_inet_port;

    char * buf = getenv(ENV_AGENT_INET_PORT);
    if (buf == NULL)
        snprintf(scheds_agt_inet_port, STRLEN, "%s", DEF_AGENT_INET_PORT);
    else
        snprintf(scheds_agt_inet_port, STRLEN, "%s", buf);

    scheds_agt_inet_port_init = true;

    return scheds_agt_inet_port;
}

bool scheds_get_trace_all() {

    char * buf = getenv(ENV_SCHED_ENABLE_TRACE_ALL);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

bool scheds_get_trace() {

    char * buf = getenv(ENV_SCHED_ENABLE_TRACE);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

bool scheds_get_trace_err() {

    char * buf = getenv(ENV_SCHED_ENABLE_TRACE_ERR);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

