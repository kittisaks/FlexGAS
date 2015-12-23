#ifndef _IVM_INTERNAL_H
#define _IVM_INTERNAL_H

#include "sched_api_export.h"
#include "channel.h"
#include <stdio.h>
#include <string.h>
#include <map>

#include <iostream>
using namespace std;

#define CHECK_ZE(statement, err_code) \
    if (statement != 0)               \
        return err_code

#define CHECK_Z(statement)   \
    do {                     \
        int ret = statement; \
        if (ret != 0)        \
            return ret;      \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
extern bool             is_api_init;
extern ch_init_attr     ch_attr;
extern ch               channel;

//Information used for scheduling
extern pid_t            pid;
extern uid_t            uid;
extern char             host[64];
extern pe_id            peid;
extern device_id        did;
extern host_id          hid;
extern group_id         gid;
extern job_id           jid;
extern int              rec_level;

//Information used for manipulating memory objects
typedef struct {
    void     * addr;
    char       mem_id[MED_STR_LEN];
    uint64_t   size;
    int        mem_flags;
    ch_mr_t    mr;
} api_mem_et_t, * api_mem_et;

typedef std::map<void *, api_mem_et> api_memory_table;
typedef api_memory_table::iterator   api_meme_iter;

extern api_memory_table a_mem_tab;
extern pthread_mutex_t  a_mem_tab_lock;

int    Init_api();

inline void marshal_bin_path(char ** argv, saa_req * req) {

    char   buf[MAX_STR_LEN];
    char * ret;
    ret = getcwd(buf, MAX_STR_LEN);
    if (ret == NULL)
        return;

    strcpy(req->data.path, buf);

    size_t offset = 0;
    for (size_t idx=0;idx<strlen(argv[0]);idx++) {
        if (argv[0][idx] == '/') {
            offset = idx++;
            break;
        }
    }
    strcpy(req->data.bin, &argv[0][offset]);
}

inline void marshal_prog_arg(int argc, char ** argv, saa_req * req) {

    req->data.argc = argc;
    memset(req->data.argv, 0, MAX_STR_LEN);
    size_t offset = 0;
    for (int idx=0;idx<argc;idx++) {
        strcpy(&(req->data.argv[offset]), argv[idx]);
        size_t noffset = offset + strlen(argv[idx]) + 1;
        offset = noffset;
    }
}

inline void create_request(saa_req_type type, saa_req * req) {

    req->type    = local;
    req->status  = initial;

    req->pid     = pid;
    req->uid     = uid;
    sprintf(req->host, "%s", host);
    req->peid    = peid;
    req->did     = did;
    req->hid     = hid;
    req->gid     = gid;
    req->jid     = jid;

    req->data.type = type;
}

inline int submit_request(saa_req * req) {

    CHECK_ZE(    chSend(channel, req, sizeof(saa_req)), -2);
    CHECK_ZE(    chRecv(channel, req, sizeof(saa_req)), -2);

    return 0;

}

inline void lock_api_memory_table() {
    pthread_mutex_lock(&a_mem_tab_lock);
}

inline void unlock_api_memory_table() {
    pthread_mutex_unlock(&a_mem_tab_lock);
}

inline void register_mem_et(
    void * addr, const char * mem_id, uint64_t size,
    int mem_flags, ch_mr_t mr) {

    api_mem_et entry = new api_mem_et_t();

    snprintf(entry->mem_id, MED_STR_LEN, "%s", mem_id);
    entry->addr      = addr;
    entry->size      = size;
    entry->mem_flags = mem_flags;
    entry->mr        = mr;

    lock_api_memory_table();
    a_mem_tab.insert(std::make_pair(addr, entry));
    unlock_api_memory_table();
}

inline api_mem_et find_mem_et(void * addr) {

    lock_api_memory_table();

    api_meme_iter it = a_mem_tab.find(addr);
    if (it!= a_mem_tab.end()) {
        unlock_api_memory_table();
        return it->second;
    }

    api_mem_et entry = NULL;
    bool       found = false;
    for (it=a_mem_tab.begin();it!=a_mem_tab.end();++it) {

        entry              = it->second;
        uint64_t addr_u64  = (uint64_t) addr;
        uint64_t lower_u64 = (uint64_t) entry->addr;
        uint64_t upper_u64 = lower_u64 + entry->size - 1;

        if ((addr_u64 >= lower_u64) && (addr_u64 <= upper_u64)) {
            found = true;
            break;
        }
    }
    if (!found)
        entry = NULL;

    unlock_api_memory_table();

    return entry;
}

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_IVM_INTERNAL_H

