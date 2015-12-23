#include "agent_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STRLEN 256

bool agts_shm_path_init = false;
char agts_shm_path[STRLEN];  

bool agts_rdma_port_init = false;
char agts_rdma_port[STRLEN];

bool agts_inet_port_init = false;
char agts_inet_port[STRLEN];

bool agts_sched_rdma_port_init = false;
char agts_sched_rdma_port[STRLEN];

bool agts_sched_inet_port_init = false;
char agts_sched_inet_port[STRLEN];

bool agts_agent_rdma_port_init = false;
char agts_agent_rdma_port[STRLEN];

bool agts_agent_inet_port_init = false;
char agts_agent_inet_port[STRLEN];

char * agts_get_shm_path() {

    if (agts_shm_path_init)
        return agts_shm_path;

    char hostname[64];
    gethostname(hostname, 64);

    char * buf = getenv(ENV_AGENT_SHM_PATH);
    if (buf == NULL) {
        buf = new char[STRLEN];
        char * buf_tmp = getcwd(buf, STRLEN);
        snprintf(agts_shm_path, STRLEN, "%s/vivm_agent_%s",
            buf_tmp, hostname);
        delete [] buf;
    }
    else {
        snprintf(agts_shm_path, STRLEN, "%s/vivm_agent_%s",
            buf, hostname);
    }

    agts_shm_path_init = true;

    return (char *) agts_shm_path;
}

char * agts_get_rdma_port() {

    if (agts_rdma_port_init)
        return agts_rdma_port;

    char * buf = getenv(ENV_AGENT_RDMA_PORT);
    if (buf == NULL) 
        snprintf(agts_rdma_port, STRLEN, "%s", DEF_AGENT_RDMA_PORT);
    else
        snprintf(agts_rdma_port, STRLEN, "%s", buf);

    agts_rdma_port_init = true;

    return agts_rdma_port;
}

char * agts_get_inet_port() {

    if (agts_inet_port_init)
        return agts_inet_port;

    char * buf = getenv(ENV_AGENT_INET_PORT);
    if (buf == NULL) 
        snprintf(agts_inet_port, STRLEN, "%s", DEF_AGENT_INET_PORT);
    else
        snprintf(agts_inet_port, STRLEN, "%s", buf);

    agts_inet_port_init = true;

    return agts_inet_port;
}

char * agts_get_sched_rdma_port() {

    if (agts_sched_rdma_port_init)
        return agts_sched_rdma_port;

    char * buf = getenv(ENV_SCHED_RDMA_PORT);
    if (buf == NULL) 
        snprintf(agts_sched_rdma_port, STRLEN, "%s", DEF_SCHED_RDMA_PORT);
    else
        snprintf(agts_sched_rdma_port, STRLEN, "%s", buf);

    agts_sched_rdma_port_init = true;

    return agts_sched_rdma_port;
}

char * agts_get_sched_inet_port() {

    if (agts_sched_inet_port_init)
        return agts_sched_inet_port;

    char * buf = getenv(ENV_SCHED_INET_PORT);
    if (buf == NULL) 
        snprintf(agts_sched_inet_port, STRLEN, "%s", DEF_SCHED_INET_PORT);
    else
        snprintf(agts_sched_inet_port, STRLEN, "%s", buf);

    agts_sched_inet_port_init = true;

    return agts_sched_inet_port;
}

char * agts_get_agent_rdma_port() {

    if (agts_agent_rdma_port_init)
        return agts_agent_rdma_port;

    char * buf = getenv(ENV_AGENT_RDMA_PORT);
    if (buf == NULL) 
        snprintf(agts_agent_rdma_port, STRLEN, "%s", DEF_AGENT_RDMA_PORT);
    else
        snprintf(agts_agent_rdma_port, STRLEN, "%s", buf);

    agts_agent_rdma_port_init = true;

    return agts_agent_rdma_port;
}

char * agts_get_agent_inet_port() {

    if (agts_agent_inet_port_init)
        return agts_agent_inet_port;

    char * buf = getenv(ENV_AGENT_INET_PORT);
    if (buf == NULL) 
        snprintf(agts_agent_inet_port, STRLEN, "%s", DEF_AGENT_INET_PORT);
    else
        snprintf(agts_agent_inet_port, STRLEN, "%s", buf);

    agts_agent_inet_port_init = true;

    return agts_agent_inet_port;
}

bool agts_get_trace_all() {

    char * buf = getenv(ENV_AGENT_ENABLE_TRACE_ALL);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

bool agts_get_trace() {

    char * buf = getenv(ENV_AGENT_ENABLE_TRACE);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

bool agts_get_trace_err() {

    char * buf = getenv(ENV_AGENT_ENABLE_TRACE_ERR);
    if (buf == NULL)
        return true;

    int ena = atoi(buf);
    if (ena == 0)
        return false;

    return true;
}

