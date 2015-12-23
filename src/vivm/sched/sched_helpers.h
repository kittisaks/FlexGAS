#ifndef _SCHED_HELPERS_H
#define _SCHED_HELPERS_H

#include <stdio.h>

/******************************************************************************
 * Search and lock operations
 *****************************************************************************/

inline sched_job find_job(job_id jid) {

    job_iter jit = job_tab.find(jid);
    if (jit == job_tab.end())
        return NULL;

    return jit->second;
}

inline int get_computation_from_table(
    sched_job job, const char * comp_id, sched_comp * comp) {

    pthread_mutex_lock(&(job->intrn.cm_mtx));
    comp_table::iterator it = job->comp_tab.find(
        std::string(comp_id));
    if (it == job->comp_tab.end()) {
        pthread_mutex_unlock(&(job->intrn.cm_mtx));
        return -1;
    }
    pthread_mutex_unlock(&(job->intrn.cm_mtx));

    *comp = it->second;

    return 0;
}

inline sched_group find_group(sched_job job, group_id gid) {

    group_table & group_tab = job->group_tab;
    group_iter git = group_tab.find(gid);
    if (git == group_tab.end())
        return NULL;

    return git->second;
}

inline sched_group find_group(job_id jid, group_id gid) {

    sched_job job = find_job(jid);
    if (job == NULL)
        return NULL;

    sched_group group = find_group(job, gid);
    if (group == NULL)
        return NULL;

    return group;
}

inline sched_device find_device(sched_group group, device_id did) {

    device_table & device_tab = group->device_tab;
    device_iter dit = device_tab.find(did);
    if (dit == device_tab.end())
        return NULL;

    return dit->second;
}

inline sched_mem_et find_memory_entry(sched_job job, const char * mem_id) {

    memory_table & memory_tab = job->mem_tab;
    meme_iter meit = memory_tab.find(std::string(mem_id));
    if (meit == memory_tab.end())
        return NULL;

    return meit->second;

   return NULL; 
}

inline sched_mem_et find_memory_entry(sched_group group, const char * mem_id) {

    memory_table & memory_tab = group->mem_tab;
    meme_iter meit = memory_tab.find(std::string(mem_id));
    if (meit == memory_tab.end())
        return NULL;

    return meit->second;
}

inline sched_mem_et find_memory_entry(
    sched_group group, const char * mem_id, int flags) {

    if ((flags && IVM_MEM_GLOBAL)) 
        return find_memory_entry(group->job, mem_id);
    else 
        return find_memory_entry(group, mem_id);

    return NULL;
}

inline void lock_job_tab() {
    pthread_mutex_lock(&job_tab_mutex);
}

inline void unlock_job_tab() {
    pthread_mutex_unlock(&job_tab_mutex);
}

inline void lock_group_tab(sched_job job) {
    pthread_mutex_lock(&(job->intrn.pem_mtx));
}

inline void unlock_group_tab(sched_job job) {
    pthread_mutex_unlock(&(job->intrn.pem_mtx));
}

inline void lock_group_tab(job_id jid) {

    sched_job job = find_job(jid);
    if (job == NULL)
        return;
    lock_group_tab(job);
}

inline void unlock_group_tab(job_id jid) {
    sched_job job = find_job(jid);
    if (job == NULL)
        return;
    unlock_group_tab(job);
}

inline void lock_device_tab(sched_group group) {
    pthread_mutex_lock(&(group->intrn.pem_mtx));
}

inline void unlock_device_tab(sched_group group) {
    pthread_mutex_unlock(&(group->intrn.pem_mtx));
}

inline void lock_device_internal(sched_device device) {
    pthread_mutex_lock(&(device->intrn.pem_mtx));
}

inline void unlock_device_internal(sched_device device) {
    pthread_mutex_unlock(&(device->intrn.pem_mtx));
}

inline void lock_mem_tab(sched_job job) {
    pthread_mutex_lock(&(job->intrn.mm_mtx));
}

inline void unlock_mem_tab(sched_job job) {
    pthread_mutex_unlock(&(job->intrn.mm_mtx));
}

inline void lock_mem_tab(sched_group group) {
    pthread_mutex_lock(&(group->intrn.mm_mtx));
}

inline void unlock_mem_tab(sched_group group) {
    pthread_mutex_unlock(&(group->intrn.mm_mtx));
}

inline void lock_mem_entry(sched_mem_et entry) {
    pthread_mutex_lock(&(entry->entry_mtx));
}

inline void unlock_mem_entry(sched_mem_et entry) {
    pthread_mutex_unlock(&(entry->entry_mtx));
}

/******************************************************************************
 * Create-and-add operations
 *****************************************************************************/

//////////////////////////////////// JOB //////////////////////////////////////


inline job_id get_new_job_id() {

    return __sync_fetch_and_add(&job_id_cnt, 1);
}

inline sched_job create_new_job_id(saa_req * req, job_id jid) {

    sched_job job = new sched_job_t();

    saa_req  * local_req;
    sag_req  * remote_req;
    int        argc;
    char     * argv;
    char     * path;
    char     * bin;

    switch(req->type) {
    case local:
        local_req = req;
        argc = local_req->data.argc;
        argv = local_req->data.argv;
        path = local_req->data.path;
        bin  = local_req->data.bin;
        break;
    case remote:
        remote_req = reinterpret_cast<sag_req *>(req);
        argc = remote_req->data.argc;
        argv = remote_req->data.argv;
        path = remote_req->data.path;
        bin  = remote_req->data.bin;
        break;
    default:
        return NULL;
    }

    job->jid      = jid;
    job->uid      = req->uid;
    job->pid      = req->pid;
    job->gid_cnt  = 0;
    job->argc     = argc;
    memcpy(job->argv, argv, MAX_STR_LEN);
    memcpy(job->path, path, MAX_STR_LEN);
    memcpy(job->bin, bin, MIN_STR_LEN);

    pthread_mutex_init(&(job->intrn.pem_mtx), NULL);
    pthread_mutex_init(&(job->intrn.mm_mtx), NULL);
    pthread_mutex_init(&(job->intrn.cm_mtx), NULL);
    job->intrn.handle = NULL;

    job->comp_tab.clear();

    job->group_tab.clear();
//    job->pe_tab.clear();

    return job;
}

inline sched_job create_new_job(saa_req * req) {

    job_id jid = get_new_job_id();

    return create_new_job_id(req, jid);
}

inline int register_job(sched_job job) {

    lock_job_tab();
    job_tab.insert(std::make_pair(job->jid, job));
    unlock_job_tab();

    return 0;
}

inline int unregister_job(sched_job job) {

    lock_job_tab();
    job_iter it = job_tab.find(job->jid);
    if (it == job_tab.end()) {
        unlock_job_tab();
        return -1;
    }
    job_tab.erase(it);
    unlock_job_tab();

    return 0;
}

//////////////////////////////// COMPUTATION //////////////////////////////////

inline int add_new_computation_to_table(
    sched_job job, sched_comp comp, 
    const char * comp_id, ivm_comp comp_fn_ptr) {

    sprintf(comp->comp_id, "%s", comp_id);
    comp->ptr                = comp_fn_ptr;
    comp->is_configured      = FALSE;
    comp->config.max_pes_num = 0;
    comp->config.type        = ivm_any;

    pthread_mutex_lock(&(job->intrn.cm_mtx));
    job->comp_tab.insert(std::make_pair(std::string(comp->comp_id), comp));
    pthread_mutex_unlock(&(job->intrn.cm_mtx));

    return 0;
}

//////////////////////////////////// GROUP ////////////////////////////////////

inline group_id get_new_group_id(sched_job job) {

    return __sync_fetch_and_add(&(job->gid_cnt), 1);
}

inline sched_group create_new_group_id(
    sched_job job, sched_comp comp, group_id gid, int rec_level) {

    sched_group group = new sched_group_t();

    group->jid        = job->jid;
    group->gid        = gid;
    group->uid        = job->uid;
    group->pid        = job->pid;
    group->did_cnt    = 0;
    group->peid_cnt   = 0;
    group->rec_level  = rec_level;

    pthread_mutex_init(&(group->intrn.pem_mtx), NULL);
    pthread_mutex_init(&(group->intrn.mm_mtx), NULL);
    pthread_mutex_init(&(group->intrn.cm_mtx), NULL);
    group->intrn.handle = NULL;

    group->comp_tab.insert(make_pair(std::string(comp->comp_id), comp));

    group->device_tab.clear();
    group->pe_tab.clear();
    group->job = job;

    return group;
}

inline sched_group create_new_group(sched_job job, 
    sched_comp comp, int rec_level) {

    group_id gid = get_new_group_id(job);

    return create_new_group_id(job, comp, gid, rec_level);
}

inline int register_group(sched_job job, sched_group group) {

    if (group->job != job)
        return -1;

    lock_group_tab(job);
    job->group_tab.insert(std::make_pair(group->gid, group));
    unlock_group_tab(job);

    return 0;
}

inline int unregister_group(sched_job job, sched_group group) {

    if (group->job != job)
        return -1;

    lock_group_tab(job);
    group_iter it = job->group_tab.find(group->gid);
    if (it == job->group_tab.end()) {
        unlock_group_tab(job);
        return -1;
    }
    job->group_tab.erase(it);
    unlock_group_tab(job);

    return 0;
}

inline bool is_group_tab_empty(sched_job job) {

    bool ret;
    lock_group_tab(job);
    ret = (job->group_tab.size() == 0);
    unlock_group_tab(job);

    return ret;
}

///////////////////////////////// DEVICE /////////////////////////////////////

inline device_id get_new_device_id(sched_group group) {

    return __sync_fetch_and_add(&(group->did_cnt), 1);
}


inline sched_device create_new_device_id(
    const char * host, const char * service, sched_link link,
    ivm_device_type dev_type, int phy_dev_id,
    sched_group group, device_id did) {

    //Sanity check
    sched_job s_job = find_job(group->jid);
    if (s_job == NULL)
        return NULL;

    sched_group s_group = find_group(s_job, group->gid);
    if (s_group != group)
        return NULL;

    sched_device device_tmp = new sched_device_t();
    device_tmp->jid = group->jid;
    device_tmp->gid = group->gid;
    device_tmp->did = did;
    device_tmp->pid = group->pid;
    device_tmp->uid = group->uid;

    device_tmp->phy.type = dev_type;
    sprintf(device_tmp->phy.host, "%s", host);
    sprintf(device_tmp->phy.service, "%s", service);
    switch(link) {
#ifndef _DISABLE_LM_RDMA
    case sched_link_rdma:
        device_tmp->phy.link = ch_link_rdma;
        break;
#endif //_DISABLE_LM_RDMA
    case sched_link_inet:
        device_tmp->phy.link = ch_link_inet;
        break;
    default:
        return NULL;
    }
    device_tmp->phy.phy_dev_id = phy_dev_id;

    //We skip 'device_tmp->intrn.service_ch' for now.
    pthread_mutex_init(&(device_tmp->intrn.pem_mtx), NULL);
    device_tmp->intrn.request_attr.link      = device_tmp->phy.link;
    device_tmp->intrn.request_attr.peer_id   = device_tmp->phy.host;
    device_tmp->intrn.request_attr.peer_port = device_tmp->phy.service;
    int ret = chOpen(&(device_tmp->intrn.request_ch),
        device_tmp->intrn.request_attr, 0);
    if (ret != 0)
        return NULL;

    device_tmp->pe_tab.clear();
    device_tmp->group = group;
    device_tmp->job   = group->job;

    return device_tmp;
}

inline int register_device(sched_group group, sched_device device) {

    if (device->group != group)
        return -1;

    lock_device_tab(group);
    group->device_tab.insert(std::make_pair(device->did, device));
    unlock_device_tab(group);

    return 0;
}

inline int unregister_device(sched_group group, sched_device device) {

    lock_device_tab(group);
    device_table & device_tab = group->device_tab;
    device_iter it = device_tab.find(device->did);
    if (it == device_tab.end()) {
        unlock_device_tab(group);
        return -1;
    }
    device_tab.erase(it);
    unlock_device_tab(group);

    return 0;
}

#if 0 //RACING-DELETION
inline bool is_device_tab_empty(sched_group group) {

    bool ret;
    lock_device_tab(group);
    ret = (group->device_tab.size() == 0);
    unlock_device_tab(group);

    return ret;
}
#endif

inline int destroy_device(sched_device device) {

    int ret = chClose(device->intrn.request_ch);
    if (ret != 0)
        return -1;

    delete device;

    return 0;
}

///////////////////////////////////// PE /////////////////////////////////////

inline pe_id get_current_max_peid(sched_group group) {

    return (group->peid_cnt > 0) ? group->peid_cnt - 1 : 0;
}

inline sched_pe create_new_pe_id(pid_t pid, uid_t uid, 
    job_id jid, group_id gid, host_id hid, device_id did, pe_id peid, 
    sched_comp comp, sched_device device, sched_group group, sched_job job) {

    sched_pe pe_tmp = new sched_pe_t();

    pe_tmp->pid       = pid;
    pe_tmp->uid       = uid;
    pe_tmp->jid       = jid;
    pe_tmp->gid       = gid;
    pe_tmp->hid       = hid;
    pe_tmp->did       = did;
    pe_tmp->peid      = peid;
    pe_tmp->comp      = comp;
    pe_tmp->device    = device;
    pe_tmp->group     = group;
    pe_tmp->job       = job;
    pe_tmp->is_killed = 0;

    return pe_tmp;
}

inline void get_new_peids(
    sched_group group, int num_pes, pe_id * s_peid, pe_id * e_peid) {

    /**
     * TODO: We need to find a fail-proved PEID allocation method.
     *       For now, we only assume that the spawning will success.
     */

    lock_device_tab(group);

    *s_peid = group->peid_cnt;
    group->peid_cnt += num_pes;
    *e_peid = group->peid_cnt - 1;

    unlock_device_tab(group);
}

inline int register_pes(sched_job job, sched_group group,
    sched_device device, const sched_pe * pes, int num_pes) {

    if (group->job != job) {
        return -1;
    }
    if (device->group != group) {
        return -1;
    }
    if (device->job != job) {
        return -1;
    }

    lock_device_tab(group);
    for (int idx=0;idx<num_pes;idx++)
        group->pe_tab.insert(std::make_pair(pes[idx]->peid, pes[idx]));
    unlock_device_tab(group);

    return 0;
}

inline int unregister_pes(sched_job job, sched_group group,
    sched_device device, const sched_pe * pes, int num_pes) {

    if (pes[0]->job != job)
        return -1;
    if (pes[0]->group != group)
        return -1;
    if (pes[0]->device != device)
        return -1;

    lock_device_tab(group);
    for (int idx=0;idx<num_pes;idx++) {

        pe_iter it = group->pe_tab.find(pes[idx]->peid);
        if (it == group->pe_tab.end()) {
            unlock_device_tab(group);
            return -1;
        }

        group->pe_tab.erase(it);
    }
    unlock_device_tab(group);

    //TODO: delete pes?
    return 0;
}

inline int unregister_pes(sched_job job, sched_group group,
    sched_device device, const pe_id * peids, int num_pes) {

    lock_device_tab(group);
    for (int idx=0;idx<num_pes;idx++) {

        pe_iter it = group->pe_tab.find(peids[idx]);
        if (it == group->pe_tab.end()) {

            //Note: This will allow the scheduler to wait in case that it 
            //      receives 'sag_unregister_pes' before it really finishes 
            //      registering PEs (the PEs execute very fast).
            unlock_device_tab(group);
            int retry_cnt = 0;
            do {
                lock_device_tab(group);
                it = group->pe_tab.find(peids[idx]);
                unlock_device_tab(group);
                retry_cnt++;
            } while ((it == group->pe_tab.end()) && (retry_cnt < 2000000000));
            lock_device_tab(group);

            it = group->pe_tab.find(peids[idx]);
            if (it == group->pe_tab.end()) {
                unlock_device_tab(group);
                return -1;
            }
        }

        sched_pe pe = it->second;
        if (pe->job->jid != job->jid) {
            unlock_device_tab(group);
            return -1;
        }
        if (pe->group != group) {
            unlock_device_tab(group);
            return -1;
        }
        if (pe->device != device) {
            unlock_device_tab(group);
            return -1;
        }

        group->pe_tab.erase(it);
    }
    unlock_device_tab(group);

    return 0;
}

inline int register_killed_pes(sched_job job, sched_group group,
    sched_device device, const sched_pe * pes, int num_pes) {

    if (group->job != job) {
        return -1;
    }
    if (device->group != group) {
        return -1;
    }
    if (device->job != job) {
        return -1;
    }

    lock_device_tab(group);
    for (int idx=0;idx<num_pes;idx++)
        group->killed_pe_tab.insert(
            std::make_pair(pes[idx]->peid, pes[idx]));
    unlock_device_tab(group);

    return 0;
}

inline int unregister_killed_pes(sched_job job, sched_group group,
    sched_device device, sched_pe * pes, int num_pes) {

    if (group->job != job)
        return -1;
    if (device->group != group)
        return -1;
    if (device->job != job)
        return -1;

    lock_device_tab(group);

    for (int idx=0;idx<num_pes;idx++) {
        pe_iter it = group->killed_pe_tab.find(pes[idx]->peid);
        if (it == group->killed_pe_tab.end()) {
            unlock_device_tab(group);
            return -1;
        }

        sched_pe pe = it->second;
        if (pes[idx]->job != pe->job) {
            unlock_device_tab(group);
            return -1;
        }
        if (pes[idx]->group != pe->group) {
            unlock_device_tab(group);
            return -1;
        }
        group->killed_pe_tab.erase(it);
    }

    unlock_device_tab(group);

    return 0;
}

/////////////////////////////////// EVENT ////////////////////////////////////

inline void queue_event(void * handle,
    sched_event_type type, sched_event event) {

    sched_tnotif tn = reinterpret_cast<sched_tnotif>(handle);
    pthread_mutex_lock(tn->mutex);

    if ((tn->mask & type) == type) {
        tn->queue.push_back(event);
        pthread_cond_signal(tn->cond);
    }

    pthread_mutex_unlock(tn->mutex);

}

inline sched_event create_new_sys_event(
    sched_event_type type, sched_job job, 
    sched_event_ack_type ack_type, void * handle) {

    sched_event event;
    event.type   = type;
    event.uid    = job->uid;
    event.pid    = job->pid;
    event.jid    = job->jid;
    event.gid    = SCHED_API_ROOT_GID;
    event.hid    = SCHED_API_ROOT_HID;
    event.did    = SCHED_API_ROOT_DID;
    event.peid   = SCHED_API_ROOT_PEID;
    event.job    = job;
    event.group  = NULL;
    event.device = NULL;

    switch(ack_type) {
    case sched_event_wait_none:
        break;
    case sched_event_busy_wait:
        event.ack    = new int();
        *event.ack   = 0;
        break;
    case sched_event_sleep_wait:
        event.sack = new pthread_cond_t();
        event.sack_mtx = new pthread_mutex_t();
        pthread_cond_init(event.sack, NULL);
        pthread_mutex_init(event.sack_mtx, NULL);
        break;
    }
    event.ack_type = ack_type;

    queue_event(handle, type, event);

    return event;
}

inline sched_event create_new_job_event(
    sched_event_type type, sched_job job, sched_group group, 
    sched_comp comp, sched_event_ack_type ack_type, void * handle) {

    sched_event event;
    event.type   = type;
    event.uid    = job->uid;
    event.pid    = job->pid;
    event.jid    = job->jid;
    event.gid    = group->gid;
    event.hid    = SCHED_API_ROOT_HID;
    event.did    = SCHED_API_ROOT_DID;
    event.peid   = SCHED_API_ROOT_PEID;
    event.job    = job;
    event.group  = group;
    event.device = NULL;

    if (type == JOB_EVENT_START_COMP) 
        event.comp = comp;
    else
        event.comp = NULL;

    switch(ack_type) {
    case sched_event_wait_none:
        break;
    case sched_event_busy_wait:
        event.ack    = new int();
        *event.ack   = 0;
        break;
    case sched_event_sleep_wait:
        event.sack = new pthread_cond_t();
        event.sack_mtx = new pthread_mutex_t();
        pthread_cond_init(event.sack, NULL);
        pthread_mutex_init(event.sack_mtx, NULL);
        break;
    }
    event.ack_type = ack_type;

    queue_event(handle, type, event);

    return event;
}

inline sched_event create_new_group_event(
    sched_event_type type, sched_job job,
     sched_group group, sched_device device, pe_id peid, void * handle) {

    sched_event event;
    event.type     = type;
    event.uid      = job->uid;
    event.pid      = job->pid;
    event.jid      = job->jid;
    event.gid      = group->gid;
    event.hid      = SCHED_API_ROOT_HID;
    event.did      = device->did;
    event.peid     = peid;
    event.job      = job;
    event.group    = group;
    event.device   = device;
    event.ack_type = sched_event_wait_none;
//TODO: Fill the comp    event.comp     = 

    queue_event(handle, type, event);

    return event;
}

inline int wait_event_ack(sched_event event) {

    switch(event.ack_type) {
    case sched_event_wait_none:
        break;
    case sched_event_busy_wait:
        do {
            usleep(10);
        } while(!*(event.ack));

        break;
    case sched_event_sleep_wait:
        pthread_mutex_lock(event.sack_mtx);
        pthread_cond_wait(event.sack, event.sack_mtx);
        pthread_mutex_unlock(event.sack_mtx);
        break;
    }

    return 0;
}

inline int destroy_event(sched_event event) {

    switch(event.ack_type) {
    case sched_event_wait_none:
        break;
    case sched_event_busy_wait:
        delete event.ack;
        break;
    case sched_event_sleep_wait:
        delete event.sack_mtx;
        delete event.sack;
        break;
    }

    return 0;
}

////////////////////////////// MEMORY ENTRY/TABLE /////////////////////////////

inline sched_mem_et create_memory_entry(
    sched_mem_copy_type   copy_type, 
    job_id                jid, 
    group_id              gid, 
    device_id             master_did, 
    sched_job             job, 
    sched_group           group, 
    sched_device          master_device, 
    const char          * iden, 
    void                * ptr, 
    ch_mr                 mr) {

    sched_mem_et entry = new sched_mem_et_t();
    snprintf(entry->iden, MAX_STR_LEN, "%s", iden);
    entry->copy_type     = copy_type;
    entry->local_ptr     = ptr;
    entry->mr            = mr;
    entry->jid           = jid;
    entry->gid           = gid;
    entry->master_did    = master_did;
    entry->job           = job;
    entry->group         = group;
    entry->master_device = master_device;
    pthread_mutex_init(&(entry->entry_mtx), NULL);

    return entry;
}

inline int register_mem_entry_root(sched_mem_et entry) {

    lock_mem_tab(entry->job);
    entry->job->mem_tab.insert(
        std::make_pair(std::string(entry->iden), entry));
    unlock_mem_tab(entry->job);

    return 0;
}

inline int register_mem_entry(sched_mem_et entry, int flags) {

    if ((flags && IVM_MEM_GLOBAL)) {
        lock_mem_tab(entry->job);
        entry->job->mem_tab.insert(
            std::make_pair(std::string(entry->iden), entry));
        unlock_mem_tab(entry->job);
    }
    else {
        lock_mem_tab(entry->group);
        entry->group->mem_tab.insert(
            std::make_pair(std::string(entry->iden), entry));
        unlock_mem_tab(entry->group);
    }

    return 0;
}

inline int register_remote_dmr(sched_mem_et entry, 
    sched_device r_device, ch_mr r_mr) {

    sched_device_mr dmr = {r_device, r_mr};
    lock_mem_entry(entry);
    entry->remote_device.push_back(dmr);
    unlock_mem_entry(entry);    

    return 0;
}

/////////////////////////////// DATA OBJECTS /////////////////////////////////

inline int decorate_mem_id(
    const char * mem_id, sched_job job, sched_group group, int flags,
    char * mem_id_dec) {

    if ((flags && IVM_MEM_GLOBAL)) {
        snprintf(mem_id_dec, MAX_STR_LEN, "%s-%d-%d", 
            mem_id, job->uid, job->jid);
    }
    else {
        snprintf(mem_id_dec, MAX_STR_LEN, "%s-%d-%d-%d", 
            mem_id, job->uid, job->jid, group->gid);
    }

    return 0;
}

inline int allocate_master_object_root(
    job_id         jid, 
    sched_job      job, 
    ch             channel,
    uint64_t       size, 
    const char   * mem_id, 
    sched_mem_et * mem_entry) {

    ch_mr mr;

    char mem_id_dec[MAX_STR_LEN];
    decorate_mem_id(mem_id, job, NULL, IVM_MEM_GLOBAL, mem_id_dec);

    int ret = chRegisterMemory(channel, (void *) mem_id_dec, 
        size, &mr, CH_SHM_FCLEAN);
    if (ret != 0)
        return -1;

    sched_mem_et entry = create_memory_entry(mem_master, jid,
        SCHED_API_ROOT_GID, SCHED_API_ROOT_DID, job, NULL, NULL,
        mem_id, NULL, mr);

    *mem_entry = entry;

    return 0;
}

inline int allocate_master_object(
    job_id         jid, 
    group_id       gid, 
    device_id      did,
    sched_job      job, 
    sched_group    group, 
    sched_device   device, 
    ch             channel,
    uint64_t       size, 
    const char   * mem_id, 
    int            flags,
    sched_mem_et * mem_entry) {

    ch_mr mr;

    char mem_id_dec[MAX_STR_LEN];
    decorate_mem_id(mem_id, job, group, flags, mem_id_dec);

    int ret = chRegisterMemory(channel, (void *) mem_id_dec,
        size, &mr, CH_SHM_FCLEAN);
    if (ret != 0)
        return -1;

    ret = chRegisterMemory(device->intrn.request_ch, (void *) mr->addr,
        size, &mr, CH_ADD_LINK_ENA);
    if (ret != 0)
        return -1;

    //TODO: Fix the NULL
    sched_mem_et entry = create_memory_entry(mem_master, jid, gid, did,
        job, group, device, mem_id, NULL, mr);

    *mem_entry = entry;

    return 0;
}

inline int allocate_mirror_object(
    job_id         jid,
    group_id       gid,
    device_id      did,
    sched_job      job,
    sched_group    group,
    sched_device   device,
    sched_device   master_device,
    ch_mr          master_mr,
    ch             channel,
    uint64_t       size,
    const char   * mem_id,
    int            flags,
    sched_mem_et * mem_entry) {

    ch_mr mr;

    char mem_id_dec[MAX_STR_LEN];
    decorate_mem_id(mem_id, job, group, flags, mem_id_dec);

    int ret = chRegisterMemory(channel, (void *) mem_id_dec,
        size, &mr, CH_SHM_FCLEAN);
    if (ret != 0)
        return -1;

    ret = chRegisterMemory(device->intrn.request_ch, (void *) mr->addr,
        size, &mr, CH_ADD_LINK_ENA);
    if (ret != 0)
        return -1;

    sched_mem_et entry = create_memory_entry(mem_mirror, jid, gid, did,
        job, group, device, mem_id, mr->addr, mr);

    register_remote_dmr(entry, master_device, master_mr);

    *mem_entry = entry;

    return 0;
}

#endif //_SChED_HELPERS_H

