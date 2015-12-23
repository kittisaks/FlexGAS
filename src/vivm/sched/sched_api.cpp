#include "sched_agent_export.h"
#include "sched_internal.h"
#include "sched_trace.h"
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

inline void create_agent_request(sag_req_type type, sag_req * req, 
    pid_t pid, uid_t uid, job_id jid, group_id gid,
    host_id hid, device_id did, pe_id peid) {

    req->type = remote;
    req->pid  = pid;
    req->uid  = uid;
    sprintf(req->host, "%s", local_hostname);
    req->jid  = jid;
    req->gid  = gid;
    req->hid  = hid;
    req->did  = did;
    req->peid = peid;

    req->data.type = type;
}

int schedGetDeviceList() {

    return 0;
}

int schedGetInternalReference(intrn_ref ref_type, void ** ref) {

    switch(ref_type) {
    case intrn_ref_job_table:
        *ref = reinterpret_cast<void *>(&job_tab);
        break;
    default:
        return -1;
    }

    return 0;
}

int schedCreateSchedHandle(void ** handle) {


    pthread_cond_t * cond = new pthread_cond_t();
    pthread_cond_init(cond, NULL);

    pthread_mutex_t * mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, NULL);

    sched_tnotif tn = new sched_tnotif_t();
    tn->mutex = mutex;
    tn->cond  = cond;

    tn->queue.clear();

    *handle = reinterpret_cast<void *>(tn);

    return 0;
}

inline int _schedRegisterHandle_sys(
    void * handle, sched_event_mask mask) {

    if ((mask & 0xff000000) != SCHED_INST_SYS)
        return -1;

    sys_handle = reinterpret_cast<sched_tnotif>(handle);
    sys_handle->mask = mask;

    return 0;
}

inline int _schedRegisterHandle_job(
    void * handle, job_id jid, sched_event_mask mask) {

    if ((mask & 0xff000000) != SCHED_INST_JOB)
        return -1;

    lock_job_tab();

        sched_job job = find_job(jid);
        if (job == NULL) {
            unlock_job_tab();
            return -1;
        }

        sched_tnotif tn = reinterpret_cast<sched_tnotif>(handle);
        tn->mask = mask;
        job->intrn.handle = tn;

    unlock_job_tab();

    return 0;
}

inline int _schedRegisterHandle_group(
    void * handle, job_id jid, group_id gid, sched_event_mask mask) {

    if ((mask & 0xff000000) != SCHED_INST_GROUP)
        return -1;

    lock_job_tab();

        sched_job job = find_job(jid);
        if (job == NULL) {
            unlock_job_tab();
            return -1;
        }

        lock_group_tab(job);

            sched_group group = find_group(job, gid);
            if (group == NULL) {
                unlock_group_tab(job);
                unlock_job_tab();
                return -1;
            }

        sched_tnotif tn = reinterpret_cast<sched_tnotif>(handle);
        tn->mask = mask;
        group->intrn.handle = tn;

    unlock_group_tab(job);
    unlock_job_tab();

    return 0;
}

int schedRegisterHandle(void * handle, sched_instance inst,
    job_id jid, group_id gid, sched_event_mask mask) {

    switch (inst) {
    case SCHED_INST_SYS:
        return _schedRegisterHandle_sys(handle, mask);
    case SCHED_INST_JOB:
        return _schedRegisterHandle_job(handle, jid, mask);
    case SCHED_INST_GROUP:
        return _schedRegisterHandle_group(handle, jid, gid, mask);
    default:
        return -1;
    }

    return 0;
}

int schedWaitEvent(void * handle, sched_event * event) {

    sched_tnotif tn = reinterpret_cast<sched_tnotif>(handle);

    pthread_mutex_lock(tn->mutex);
        if (tn->queue.size() > 0) {
            *event = *(tn->queue.begin());
            tn->queue.erase(tn->queue.begin());
            pthread_mutex_unlock(tn->mutex);
            return 0;
        }
        pthread_cond_wait(tn->cond, tn->mutex);
        *event = *(tn->queue.begin());
        tn->queue.erase(tn->queue.begin());
    pthread_mutex_unlock(tn->mutex);

    return 0;
}

int schedPollEvent(void * handle, sched_event * event) {

    sched_tnotif tn = reinterpret_cast<sched_tnotif>(handle);

    pthread_mutex_lock(tn->mutex);
        if (tn->queue.size() <= 0) {
            pthread_mutex_unlock(tn->mutex);
            return -1;
        }
        *event = *(tn->queue.begin());
        tn->queue.erase(tn->queue.begin());
    pthread_mutex_unlock(tn->mutex);

    return 0;
}

int schedAckEvent(sched_event event) {

    switch (event.ack_type) {
    case sched_event_wait_none:
        break;
    case sched_event_busy_wait:
        *(event.ack) = 1;
        break;
    case sched_event_sleep_wait:
        pthread_cond_signal(event.sack);
        break;
    }

    return 0;
}

int schedPrintf(const char * format, ...) {

    char    str[2048];
    va_list ap;

    va_start(ap, format);
    vsprintf(str, format, ap);
    va_end(ap);

    TRACE_STR(str);

    return 0;
}

int schedPrintfe(const char * format, ...) {

    char    str[2048];
    va_list ap;

    va_start(ap, format);
    vsprintf(str, format, ap);
    va_end(ap);

    TRACE_STR_ERR(str);

    return 0;
}

int schedStartComputation() {

    return 0;
}

inline int submit_agent_request(sag_req * req, sched_device device) {

    int ret;

    lock_device_internal(device);

    ret = chSend(device->intrn.request_ch, req, sizeof(sag_req));
    if (ret != 0) {
        unlock_device_internal(device);
        return -1;
    }

    ret = chRecv(device->intrn.request_ch, req, sizeof(sag_req));
    if (ret != 0) {
        unlock_device_internal(device);
        return -1;
    }

    unlock_device_internal(device);

    return 0;
}

inline int request_agent_create_job(sched_job job, sched_device device) {

    sag_req req;
    create_agent_request(sag_agent_register_job, &req, 
        job->pid, job->uid, job->jid,
        SCHED_API_ROOT_GID, SCHED_API_ROOT_HID, SCHED_API_ROOT_DID, 
        SCHED_API_ROOT_PEID);

    req.data.argc = job->argc;
    memcpy(req.data.argv, job->argv, MAX_STR_LEN);
    memcpy(req.data.path, job->path, MAX_STR_LEN);
    memcpy(req.data.bin, job->bin, MIN_STR_LEN);

    int ret = submit_agent_request(&req, device);
    if (ret != 0)
        return -1;
    if (req.status != success)
        return -1;

    return 0;
}

inline int request_agent_create_group(sched_group group, sched_device device) {

    sag_req req;
    create_agent_request(sag_agent_register_group, &req, 
        group->pid, group->uid, group->jid, group->gid,
        SCHED_API_ROOT_HID, SCHED_API_ROOT_DID, SCHED_API_ROOT_PEID);

    req.data.gid       = group->gid;
    req.data.comp      = *(group->comp_tab.begin()->second);
    req.data.rec_level = group->rec_level + 1;

    int ret = submit_agent_request(&req, device);
    if (ret != 0)
        return -1;
    if (req.status != success)
        return -1;

    return 0;
}

inline int request_agent_create_device(sched_device device) {

    sag_req req;
    create_agent_request(sag_agent_register_device, &req,
        device->job->pid, device->job->uid, device->job->jid,
        device->group->gid, SCHED_API_ROOT_HID, device->did,
        SCHED_API_ROOT_PEID);

    req.data.did         = device->did;
    req.data.link        = device->phy.link;
    req.data.device_type = device->phy.type;
    req.data.phy_dev_id  = device->phy.phy_dev_id;

    int ret = submit_agent_request(&req, device);
    if (ret != 0)
        return -1;
    if (req.status != success)
        return -1;

    return 0;
}

inline int request_agent_create_job_group_device(
    sched_job job, sched_group group, sched_device device) {

    sag_req req;
    create_agent_request(sag_agent_register_jgd, &req,
        device->job->pid, device->job->uid, device->job->jid,
        device->group->gid, SCHED_API_ROOT_HID, device->did,
        SCHED_API_ROOT_PEID);

    req.data.argc = job->argc;
    memcpy(req.data.argv, job->argv, MAX_STR_LEN);
    memcpy(req.data.path, job->path, MAX_STR_LEN);
    memcpy(req.data.bin, job->bin, MIN_STR_LEN);

    req.data.gid         = group->gid;
    req.data.comp        = *(group->comp_tab.begin()->second);
    req.data.rec_level   = group->rec_level + 1;

    req.data.did         = device->did;
    req.data.link        = device->phy.link;
    req.data.device_type = device->phy.type;
    req.data.phy_dev_id  = device->phy.phy_dev_id;

    int ret = submit_agent_request(&req, device);
    if (ret != 0)
        return -1;
    if (req.status != success)
        return -1;

    return 0;
}

inline int create_new_device(const char * host, sched_link link,
    ivm_device_type dev_type, int phy_dev_id,
    sched_group group, sched_device * device) {

    device_id did = get_new_device_id(group);

    char * service;
    switch(link) {
    case sched_link_rdma:
        service = scheds_get_agent_rdma_port();
        break;
    case sched_link_inet:
        service = scheds_get_agent_inet_port();
        break;
    default:
        return -1;
    }

    sched_device device_tmp = create_new_device_id(
        host, service, link, dev_type, phy_dev_id, group, did);
    if (device_tmp == NULL) {
        TRACE_ERR("Failed to create new device");
        return -1;
    }

#if 0
    /**
     *Note: Separately request job/group/device creations incur high overhead.
     *      To avoid too much communication overhead, we request all of the
     *      instances to be created with a single request (see the code in the
     *      'else' case).
     */
    CHECK_Z(    request_agent_create_job(device_tmp->job, device_tmp),
        "Cannot create agent-job");
    CHECK_Z(    request_agent_create_group(device_tmp->group, device_tmp),
        "Cannot create agent-group");
    CHECK_Z(    request_agent_create_device(device_tmp),
        "Cannot create agent-device");
#else
    CHECK_Z(    request_agent_create_job_group_device(
        device_tmp->job, device_tmp->group, device_tmp),
        "Cannot create agent-job,group,device on host: %s", host);
#endif

    CHECK_Z(    register_device(group, device_tmp),
        "Cannot add device to tables");

    *device = device_tmp;

    return 0;
}

int schedCreateDevice(const char * host, sched_link link,
    ivm_device_type dev_type, int phy_dev_id,
    sched_group group, sched_device * device) {

    return create_new_device(host, link, dev_type, phy_dev_id, group, device);
}

int schedDestroyJob(sched_job job) {

    CHECK_Z(    unregister_job(job),
        "Cannot destroy job.");
    delete job;

    //Note: The job at a remote node is implicitly deleted when
    //      schedDestroyDevice is called.

    return 0;
}

int schedDestroyGroup(sched_group group) {

    CHECK_Z(    unregister_group(group->job, group),
        "Cannot destroy group.");
    delete group;

    //Note: The group at a remote node is implicitly deleted when
    //      schedDestroyDevice is called.

    return 0;
}

int schedDestroyDevice(sched_device device) {

    sag_req req;
    create_agent_request(sag_agent_unregister_device, &req,
        device->job->pid, device->job->uid, device->job->jid,
        device->group->gid, SCHED_API_ROOT_HID, device->did,
        SCHED_API_ROOT_PEID);

    req.data.did = device->did;

    CHECK_Z(    submit_agent_request(&req, device),
        "Cannot destroy device on remote host.");
    if (req.status != success)
        return -1;

    //Note: The device is already destroyed in the SchedRemoteHandler.

    return 0;
}

int schedSpawnPes(sched_device device, sched_comp comp,
    int num_pes, sched_pe * pe) {

    /**
     * ISSUE:
     */
//    usleep(10000);
///*
    comp_iter cit = device->group->comp_tab.find(std::string(comp->comp_id));
    if (cit == device->group->comp_tab.end()) {
        TRACE("1.1");
        return -1;
    }
    if (get_current_max_peid(device->group) >= 
        cit->second->config.max_pes_num) {
        TRACE("1.2 %d - %d %s", get_current_max_peid(device->group),
            cit->second->config.max_pes_num, comp->comp_id);
        return 0;
    }
//*/

    sag_req req;
    create_agent_request(sag_agent_spawn_pes, &req,
        device->pid, device->uid, device->jid,
        device->gid, SCHED_API_ROOT_HID, device->did,
        SCHED_API_ROOT_PEID);

    req.data.comp            = *comp;
    req.data.num_pes         = num_pes;
    req.data.device_type_pes = device->phy.type;
    req.data.phy_dev_id_pes  = device->phy.phy_dev_id;
    req.data.hid             = SCHED_API_ROOT_HID;
    get_new_peids(device->group, num_pes,
        &(req.data.s_peid), &(req.data.e_peid));

    CHECK_Z(    submit_agent_request(&req, device),
        "Cannot submit request to agent.");
    if (req.status != success)
        return -1;

    sched_pe * pe_vector = new sched_pe [num_pes];
    for (int idx=0;idx<num_pes;idx++) {

        pe_vector[idx] = create_new_pe_id(SCHED_API_ROOT_PEID, device->uid,
            device->jid, device->gid, SCHED_API_ROOT_HID, device->did,
            req.data.s_peid + idx, comp, device, device->group, device->job);
    }

    CHECK_Z(    register_pes(device->job, device->group,
        device, pe_vector, num_pes),
        "Cannot register device.");

    if (pe != NULL) {
        for (int idx=0;idx<num_pes;idx++) {
            pe[idx] = pe_vector[idx];
        }
    }

    delete [] pe_vector;

    return 0;
}

int schedKillPes(sched_pe pe) {

    //Note: The function has no effect in case that the PE 
    //      has not been spawned or has already exited.
    if (pe->group->pe_tab.find(pe->peid) ==
        pe->group->pe_tab.end())
        return 0;

    sag_req req;
    create_agent_request(sag_agent_kill_pes, &req,
        pe->device->pid, pe->uid, pe->jid,
        pe->gid, SCHED_API_ROOT_HID, pe->did,
        pe->peid);

    CHECK_Z(    submit_agent_request(&req, pe->device),
        "Cannot submit request to agent.");
    if (req.status != success)
        return -1;

    pe->is_killed = 1;
    sched_pe pes[1] = {pe};
    CHECK_Z(    unregister_pes(pe->job, pe->group, pe->device, pes, 1),
        "Cannot unregister PEs.");
    CHECK_Z(    register_killed_pes(pe->job, pe->group, pe->device, pes, 1),
        "Cannot register killed PEs.");

    if (pe->group->intrn.handle != NULL) {
        create_new_group_event(GROUP_EVENT_PE_KILLED,
            pe->job, pe->group, pe->device, pe->peid, pe->group->intrn.handle); 
    }

    return 0;
}

int schedKillGroup() {

    return 0;
}

int schedRespawnPes(sched_device device, sched_pe pe) {

/*
    if (get_current_max_peid(device->group) >= 
        device->group->comp_tab.begin()->second->config.max_pes_num)
        return -1;
*/

    sched_pe pe_vector[1] = {pe};
    CHECK_Z(    unregister_killed_pes(device->job, device->group,
        device, pe_vector, 1), 
        "Specified PE was not previously killed.");

    if (device->jid != pe->jid)
        return -1;
    if (device->gid != pe->gid)
        return -1;

    sag_req req;
    create_agent_request(sag_agent_spawn_pes, &req,
        device->pid, device->uid, device->jid,
        device->gid, SCHED_API_ROOT_HID, device->did,
        SCHED_API_ROOT_PEID);

    req.data.comp            = *(pe->comp);
    req.data.num_pes         = 1;
    req.data.device_type_pes = device->phy.type;
    req.data.phy_dev_id_pes  = device->phy.phy_dev_id;
    req.data.hid             = SCHED_API_ROOT_HID;
    req.data.s_peid          = pe->peid;
    req.data.e_peid          = pe->peid;

    CHECK_Z(    submit_agent_request(&req, device),
        "Cannot submit request to agent.");
    if (req.status != success)
        return -1;

    pe->device            = device;
    CHECK_Z(    register_pes(device->job, device->group,
        device, pe_vector, 1),
        "Cannot register device.");

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

