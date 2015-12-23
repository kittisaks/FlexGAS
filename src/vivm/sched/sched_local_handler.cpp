#include "sched_threads.h"
#include "sched_internal.h"
#include <stdio.h>

char              local_hostname[MED_STR_LEN];
sched_tnotif      sys_handle;
job_table         job_tab;
pthread_mutex_t   job_tab_mutex;
volatile job_id   job_id_cnt = 0;

inline void SchedLocalHandler::create_response(
    saa_req * req, req_status status,
    job_id jid, group_id gid, host_id hid, device_id did, pe_id peid) {

    req->status = status;

    req->jid    = jid;
    req->gid    = gid;
    req->hid    = hid;
    req->did    = did;
    req->peid   = peid;
}

inline void SchedLocalHandler::create_response(
    saa_req * req, req_status status) {

    req->status = status;
}

inline int SchedLocalHandler::respond_api(saa_req * req) {

    return chSend(mApiChannel, req, sizeof(saa_req));
}

#define CHECK_ZE(statement, errcode) \
    do {                             \
        int ret = statment;          \
        if (ret != 0)                \
            return errcode;          \
    } while(0)

#ifdef CHECK_Z
#undef CHECK_Z
#define CHECK_Z(statement)           \
        if (statement != 0)          \
            return -1
#endif //CHECK_Z

/******************************************************************************
 * Initialization
 *****************************************************************************/

int SchedLocalHandler::HandleRegisterPe(saa_req * req) {

    //Note: Only the root PE can register itself with the scheduler.
    //      No need to handle the registration for descendant PEs.
    sched_job job = create_new_job(req);
    register_job(job);

    create_response(req, success,
        job->jid, SCHED_API_ROOT_GID, SCHED_API_ROOT_HID,
        SCHED_API_ROOT_DID, SCHED_API_ROOT_PEID);
    mJid  = job->jid;
    mpJob = job;

    if (sys_handle != NULL) {
        sched_event event = create_new_sys_event(SYS_EVENT_START_JOB, job,
            sched_event_busy_wait, sys_handle);
        wait_event_ack(event);
        destroy_event(event);
    }

    CHECK_Z(    respond_api(req));

    return 0;
}

int SchedLocalHandler::HandleUnregisterPe(saa_req * req) {

    //Note: Only the root PE can unregister itself with the scheduler.
    //      No need to handle the registration for decendant PEs.

    if (req->jid != mJid) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    if (mpJob->group_tab.size() > 0) {
        TRACE_ERR("Job: %d: remaining computation group.", req->jid);
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

/******************************************************************************
 * Computation Management
 *****************************************************************************/
#if 0
inline int add_new_computation_to_table(
    sched_job job, sched_comp comp, saa_req * req) {

    sprintf(comp->comp_id, "%s", req->data.comp_id);
    comp->ptr                = req->data.comp_fn_ptr;
    comp->is_configured      = FALSE;
    comp->config.max_pes_num = 0;
    comp->config.type        = ivm_any;

    pthread_mutex_lock(&(job->intrn.cm_mtx));
    job->comp_tab.insert(std::make_pair(std::string(comp->comp_id), comp));
    pthread_mutex_unlock(&(job->intrn.cm_mtx));

    return 0;
}

inline int get_computation_from_table(
    sched_job job, saa_req * req, sched_comp * comp) {

    pthread_mutex_lock(&(job->intrn.cm_mtx));
    comp_table::iterator it = job->comp_tab.find(
        std::string(req->data.comp_id));
    if (it == job->comp_tab.end()) {
        pthread_mutex_unlock(&(job->intrn.cm_mtx));
        return -1;
    }
    pthread_mutex_unlock(&(job->intrn.cm_mtx));

    *comp = it->second;

    return 0;
}
#endif

int SchedLocalHandler::HandleRegisterComp(saa_req * req) {

    if (req->jid != mJid) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    sched_comp comp = new sched_comp_t();
    add_new_computation_to_table(
        mpJob, comp, req->data.comp_id, req->data.comp_fn_ptr);

    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

int SchedLocalHandler::HandleConfigureComp(saa_req * req) {

    if (req->jid != mJid) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    sched_comp comp;
    int ret = get_computation_from_table(mpJob, req->data.comp_id, &comp);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }
    comp->config        = req->data.comp_config;
    comp->is_configured = TRUE;

    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

int SchedLocalHandler::HandleLaunchComp(saa_req * req) {

    if (req->jid != mJid) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    sched_comp comp;
    int ret = get_computation_from_table(mpJob, req->data.comp_id, &comp);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    /**
     * TODO:
     * This will submit launch request to the scheduling thread,
     * wait for the completion, and send the respond back to the API.
     * The scheduling thread has a shared request and respond queue for
     * all the local-sched-handler threads.
     */

    sched_comp comp_group = new sched_comp_t();
    *comp_group = *comp;
    comp_group->config = req->data.comp_config;

    sched_group group = create_new_group(
        mpJob, comp_group, 0);
    CHECK_Z(    register_group(mpJob, group));

    if (mpJob->intrn.handle == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    sched_event event = create_new_job_event(JOB_EVENT_START_COMP,
        mpJob, group, comp_group, sched_event_busy_wait, mpJob->intrn.handle);
    wait_event_ack(event);
    destroy_event(event);

    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

int SchedLocalHandler::HandleMemAlloc(saa_req * req) {

    /**
     * Note: If one or more memory entries are created by this handler,
     *       the entires will always be added to the memory table of
     *       the job.
     */
    TRACE("Allocating Memory (Sched)");

    //Check whether the memory object already exists.
    sched_mem_et entry = find_memory_entry(mpJob, req->data.mem_id);
    if (entry != NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    //Allocate the memory object and create the corresponding memory entry.
    int ret = allocate_master_object_root(mJid, mpJob, mApiChannel, 
        req->data.mem_size, req->data.mem_id, &entry);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    //Register the memory entry.
    register_mem_entry_root(entry);

    //Provide back to the API a memory-region.
    memcpy(&req->data.mr, entry->mr, sizeof(ch_mr_t));

    create_response(req, success);
    respond_api(req);

    return 0;
}

int SchedLocalHandler::HandleMemMap(saa_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): SchedLocalHandler::HandleMemMap */

    TRACE("Mapping Memory (Sched)");

    create_response(req, failed);
    respond_api(req);

    return -1;
}

int SchedLocalHandler::HandleMemPut(saa_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): SchedLocalHandler::HandleMemPut */

    TRACE("Mem put (Sched)");

    create_response(req, failed);
    respond_api(req);

    return -1;
}

int SchedLocalHandler::HandleMemGet(saa_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): SchedLocalHandler::HandleMemGet */

    TRACE("Mem get (Sched)");

    create_response(req, failed);
    respond_api(req);

    return -1;
}

int SchedLocalHandler::HandleMemUpdate(saa_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): SchedLocalHandler::HandleMemUpdate */

    TRACE("Mem update (Sched)");

    create_response(req, failed);
    respond_api(req);

    return -1;
}

