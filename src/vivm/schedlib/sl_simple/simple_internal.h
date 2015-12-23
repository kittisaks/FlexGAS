#ifndef _SIMPLE_INTERNAL_H
#define _SIMPLE_INTERNAL_H

#include "sched_api.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COMP_NAME
#define CHECK_Z(statement, err_msg_format, ...)                            \
    if (statement != 0)                                                    \
        schedPrintfe("[%s]: %s", COMP_NAME, err_msg_format, ##__VA_ARGS__)

#define CHECK_ZF(statement, err_msg_format, ...)                            \
    do                                                                      \
        if (statement != 0) {                                               \
            schedPrintfe("[%s]: %s", COMP_NAME,                             \
                err_msg_format, ##__VA_ARGS__);                             \
            return -1;                                                      \
    } while(0)

#define TRACE(str, ...)                                    \
    schedPrintf(str, ##__VA_ARGS__)

typedef struct {
    void        * event_handle;
    pthread_t     job_handler;
} simp_context;

typedef struct {
    sched_event   sys_event;
    void        * event_handle;
    pthread_t     group_handler;
} simp_job_context;

typedef struct {
    sched_event   job_event;
    void        * event_handle;
} simp_group_context;

int simp_job_handler(
    simp_context * sys_context, sched_event sys_event);

int simp_group_handler(
    simp_job_context * job_context, sched_event job_event);

extern int     node_list_available;
extern char ** node_list;
extern int     node_count;

static inline int simp_get_node_list(char *** node_lst, int * num_nodes) {

    FILE    * nl_file;
    char   ** node_lst_tmp;
    char      cwd[128];
    char      nl_fname[256];
    size_t    len;
    int       idx, ret;

    if (node_list_available) {
        *node_lst  = node_list;
        *num_nodes = node_count;
        return 0;
    }

#define MAX_NODES   128

    char * buf =  getcwd(cwd, 128);
    if (buf != cwd)
        return -1;
    snprintf(nl_fname, 128, "%s/nodes.lst", cwd);

    node_lst_tmp = (char ** ) malloc(MAX_NODES * sizeof(char *));

    nl_file = fopen(nl_fname, "r");
    if (nl_file == NULL) 
        return -1;

    idx = 0;
    do {
        ret = getline(&(node_lst_tmp[idx]), &len, nl_file);
        size_t len = strlen(node_lst_tmp[idx]);
        if (node_lst_tmp[idx][len - 1] == '\n')
            node_lst_tmp[idx][len - 1] = '\0';

        TRACE("NN: %s", node_lst_tmp[idx]);
        if (ret != -1)
            idx++;
    } while ((ret != -1) && (idx < MAX_NODES));

    *num_nodes = idx;
    *node_lst  = node_lst_tmp;
    node_count = idx;
    node_list  = node_lst_tmp;
    node_list_available = 1;

    fclose(nl_file);

    return 0;
}

#undef COMP_NAME
#endif //_SIMPLE_INTERNAL_H

