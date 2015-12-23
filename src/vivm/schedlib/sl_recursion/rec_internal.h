#ifndef _REC_INTERNAL_H
#define _REC_INTERNAL_H

#include "sched_api.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>

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


#define CPUS_PER_NODE  1
#define GPUS_PER_NODE  0
#define DEF_RES_CNT    2
#define MAX_RES_CNT   16

/**
 * Thread structure
 */

typedef struct {
    pthread_t       pthread_ref;
    pthread_attr_t  attr;
    pthread_cond_t  cond;
    pthread_mutex_t mtx;

    enum status_e {
        sleep, wake
    };
    status_e         status;
} thread_t, * thread;

/**
 * Resource-related structures
 */

typedef struct {
    char                  hostname[64];
    ivm_device_type       type;
    std::vector<sched_pe> occupant_pes;
    int                   num_occupants;
    pthread_mutex_t       lock;
    std::vector<int>      rec_levels;
} resource;

typedef struct {
    resource     * res;
    sched_device   device;
} exec_resource;

/**
 * Handler-related structures
 */

typedef struct {
    void        * event_handle;
    pthread_t     job_handler;
} sys_context;

typedef struct {
    sched_event * sys_event;
    void        * event_handle;
} job_context;

typedef struct {
    sched_event   * job_event;
    void          * event_handle;
    thread_t      * th_group;
    exec_resource * e_res;
    int             num_e_res;
} group_context;

int job_handler(
    sys_context * sys_ctx, sched_event sys_event);

int group_handler(
    job_context * job_ctx, sched_event job_event);

extern char             ** node_list;
extern int                 node_count;
extern bool                node_list_available;
extern resource         *  resource_list;
extern int                 resource_count;
extern bool                resource_list_available;
extern pthread_mutex_t     resource_list_mtx;


static inline int get_node_list() {

    FILE    * nl_file;
    char   ** node_lst_tmp;
    char      cwd[128];
    char      nl_fname[256];
    size_t    len;
    int       idx, ret;

    if (node_list_available)
        return 0;

#define MAX_NODES   128

    char * buf = getcwd(cwd, 128);
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
        if (ret == -1)
            break;
        size_t len = strlen(node_lst_tmp[idx]);
        if (node_lst_tmp[idx][len - 1] == '\n')
            node_lst_tmp[idx][len - 1] = '\0';

        TRACE("NODE: %s", node_lst_tmp[idx]);
        if (ret != -1)
            idx++;
    } while ((ret != -1) && (idx < MAX_NODES));

    node_count = idx;
    node_list  = node_lst_tmp;
    node_list_available = true;

    fclose(nl_file);

    return 0;
}

typedef struct {
    thread_t    * th;
    sched_event * event;
} th_event;

inline int thread_create(
    thread th, void * (* routine) (void *), void * arg) {

    pthread_attr_init(&(th->attr));
    pthread_attr_setdetachstate(&(th->attr), PTHREAD_CREATE_JOINABLE);
    pthread_cond_init(&(th->cond), NULL);
    pthread_mutex_init(&(th->mtx), NULL);

    pthread_create(&(th->pthread_ref), &(th->attr), routine, arg);

    return 0;
}

inline void thread_sleep(thread th) {

    pthread_mutex_lock(&(th->mtx)); {
        th->status = thread_t::sleep;
        pthread_cond_wait(&(th->cond), &(th->mtx));
        th->status = thread_t::wake;
    }
    pthread_mutex_unlock(&(th->mtx));
}

inline void thread_wake(thread th) {

    pthread_mutex_lock(&(th->mtx)); {
        if (th->status == thread_t::wake) {
            pthread_mutex_unlock(&(th->mtx));
            return;
        }
    }
    pthread_mutex_unlock(&(th->mtx));

    thread_t::status_e status;
    do {
        pthread_cond_signal(&(th->cond));
        pthread_mutex_lock(&(th->mtx));
            status = th->status;
        pthread_mutex_unlock(&(th->mtx));
    } while (status != thread_t::wake); 
}

///////////////////////////////// AtomicVector ////////////////////////////////

template <typename T> class AtomicVector : private std::vector<T> {
public:
    AtomicVector();
    ~AtomicVector();
    int  push_back(T item);
    int  push_back_unsafe(T item);
    int  push_head(T item);
    int  push_head_unsafe(T item);
    int  fetch(T * item, int index);
    int  fetch_unsafe(T * item, int index);
    int  size();
    int  size_unsafe();
    void lock();
    void unlock();
private:
    pthread_mutex_t mMtx;
};

template <typename T> AtomicVector<T>::AtomicVector() {

    pthread_mutex_init(&mMtx, NULL);
    std::vector<T>::clear();
}

template <typename T> AtomicVector<T>::~AtomicVector() {
}

template <typename T> int AtomicVector<T>::push_back(T item) {
    pthread_mutex_lock(&mMtx); {
        push_back_unsafe(item);
    }
    pthread_mutex_unlock(&mMtx);

    return 0;
}

template <typename T> int AtomicVector<T>::push_back_unsafe(T item) {

    std::vector<T>::push_back(item);

    return 0;
}

template <typename T> int AtomicVector<T>::push_head(T item) {
    pthread_mutex_lock(&mMtx); {
        push_head_unsafe(item);
    }
    pthread_mutex_unlock(&mMtx);

    return 0;
}

template <typename T> int AtomicVector<T>::push_head_unsafe(T item) {

    std::vector<T>::insert(std::vector<T>::begin(), item);

    return 0;
}

template <typename T> int AtomicVector<T>::fetch(T * item, int index) {
    pthread_mutex_lock(&mMtx); {
        fetch_unsafe(item, index);
    }
    pthread_mutex_unlock(&mMtx);

    return 0;
}

template <typename T> int AtomicVector<T>::fetch_unsafe(T * item, int index) {

    typename std::vector<T>::iterator it = std::vector<T>::begin();
    it += index;
    *item = *it;
    std::vector<T>::erase(it);

    return 0;
}

template <typename T> int AtomicVector<T>::size() {

    int size;
    pthread_mutex_lock(&mMtx); {
        size = size_unsafe();
    }
    pthread_mutex_unlock(&mMtx);

    return size;
}

template <typename T> int AtomicVector<T>::size_unsafe() {

    return std::vector<T>::size();
}

template <typename T> void AtomicVector<T>::lock() {
    pthread_mutex_lock(&mMtx);
}

template <typename T> void AtomicVector<T>::unlock() {
    pthread_mutex_unlock(&mMtx);
}

////////////////////////////////// AtomicMap //////////////////////////////////

template <typename T_key, typename T_val> 
    class AtomicMap : private std::map<T_key, T_val> {
public:
    AtomicMap();
    ~AtomicMap();
    void insert(std::pair<T_key, T_val> pair);
    void insert_unsafe(std::pair<T_key, T_val> pair);
    int  fetch(T_val * val, T_key key);
    int  fetch_unsafe(T_val * val, T_key key);
    void lock();
    void unlock();

private:
    pthread_mutex_t mMtx;
};

template <typename T_key, typename T_val>
    AtomicMap<T_key, T_val>::AtomicMap() {

    pthread_mutex_init(&mMtx, NULL);
    std::map<T_key, T_val>::clear();
}

template <typename T_key, typename T_val>
    AtomicMap<T_key, T_val>::~AtomicMap() {
}

template <typename T_key, typename T_val>
    void AtomicMap<T_key, T_val>::insert(
    std::pair<T_key, T_val> pair) {

    pthread_mutex_lock(&mMtx); {
        insert_unsafe(pair);
    }
    pthread_mutex_unlock(&mMtx);
}

template <typename T_key, typename T_val>
    void AtomicMap<T_key, T_val>::insert_unsafe(
    std::pair<T_key, T_val> pair) {

    std::map<T_key, T_val>::insert(pair);
}

template <typename T_key, typename T_val>
    int AtomicMap<T_key, T_val>::fetch(T_val * val, T_key key) {

    int ret;
    pthread_mutex_lock(&mMtx); {
        ret = fetch_unsafe(val, key);
    }
    pthread_mutex_unlock(&mMtx);

    return ret;
}

template <typename T_key, typename T_val>
    int AtomicMap<T_key, T_val>::fetch_unsafe(T_val * val, T_key key) {

    typename std::map<T_key, T_val>::iterator it;
    it = std::map<T_key, T_val>::find(key);
    if (it == std::map<T_key, T_val>::end())
        return -1;

    *val = it->second;

    return 0;
}

template <typename T_key, typename T_val>
    void AtomicMap<T_key, T_val>::lock() {

    pthread_mutex_lock(&mMtx);
}

template <typename T_key, typename T_val>
    void AtomicMap<T_key, T_val>::unlock() {

    pthread_mutex_unlock(&mMtx);
}

void * job_sched_routine(void * arg);
void * group_sched_routine(void * arg);

#undef COMP_NAME
#endif //_REC_INTERNAL_H

