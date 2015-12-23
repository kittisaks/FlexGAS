#include "sched_internal.h"
#include "sched_threads.h"

#ifdef CHECK_Z
#undef CHECK_Z
#define CHECK_Z(statement)           \
        if (statement != 0)          \
            return -1
#endif //CHECK_Z

inline int patch_mpDevice(
    sched_group mpGroup, sched_device & mpDevice, sag_req * req) {

    if (mpDevice != NULL)
        return 0;

    sched_device device_tmp = find_device(mpGroup, req->did);
    if (device_tmp == NULL)
        return -1;

    mpDevice = device_tmp;

    return 0;
}

int SchedRemoteHandler::HandleRegisterAgent(sag_req * req) {

    sched_job job = find_job(req->jid);
    if (job == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    sched_group group = find_group(job, req->gid);
    if (group == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    mpJob    = job;
    mJid     = job->jid;
    mpGroup  = group;
    mGid     = group->gid;
    mDid     = req->did;

    create_response(req, success);
    CHECK_Z(    respond_agent(req));

    return 0;
}

int SchedRemoteHandler::HandleUnregisterAgent(sag_req * req) {

    int ret = patch_mpDevice(mpGroup, mpDevice, req);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
    }

    create_response(req, success);
    CHECK_Z(    respond_agent(req));

    sleep(1);

    CHECK_Z(    unregister_device(mpGroup, mpDevice));
    CHECK_Z(    destroy_device(mpDevice));
    mTerminate = true;

    return 0;
}

int SchedRemoteHandler::HandleNotifyPeDone(sag_req * req) {

    int ret = patch_mpDevice(mpGroup, mpDevice, req);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    ret = unregister_pes(mpJob, mpGroup, mpDevice, &(req->peid), 1);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    if (mpGroup->intrn.handle != NULL) {
        create_new_group_event(GROUP_EVENT_PE_EXIT,
            mpJob, mpGroup, mpDevice, req->peid, mpGroup->intrn.handle);
    }

    create_response(req, success);
    CHECK_Z(    respond_agent(req));

    return 0;
}

int SchedRemoteHandler::HandleAddMemoryEntry(sag_req * req) {

    int ret = patch_mpDevice(mpGroup, mpDevice, req);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
    }

    sched_mem_et entry = create_memory_entry(mem_record, mpJob->jid, 
        mpGroup->gid, mpDevice->did, mpJob, mpGroup, mpDevice,
        req->data.mem_id, NULL, NULL);

    ret = register_mem_entry(entry, req->data.mem_flags);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_agent(req));

    return 0;
}

int SchedRemoteHandler::HandleRemoveMemoryEntry(sag_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): SchedRemoteHandler::HandleRemoveMemoryEntry */

    TRACE("REMOVE MEMORY ENTRY");

    create_response(req, success);
    CHECK_Z(    respond_agent(req));

    return 0;
}

int SchedRemoteHandler::HandleQueryMemory(sag_req * req) {

    TRACE("Received query-memory request (peid: %d):", req->peid);

    int ret = patch_mpDevice(mpGroup, mpDevice, req);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
    }

    sched_mem_et entry = find_memory_entry(
        mpGroup, req->data.mem_id, req->data.mem_flags);
    if (entry == NULL) {
        TRACE("\tData object '%s' is not found.", req->data.mem_id);
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    if (entry->copy_type == mem_master) {
        TRACE("\tData object '%s' is found (master).", req->data.mem_id);
        req->data.mr_valid = true;

        create_response(req, success);
        CHECK_Z(    respond_agent(req));

        // Syncronize content with the mirror-copy
        /**
         * Note: This remotely writes data from the master-copy on sched to
         *       a mirror-copy on an agent. The steps here is to exchange
         *       the memory-regions of both the master- and mirror-copy.
         *       The sched then writes to the remote mirror-copy and sends
         *       a notification as a memory-region back to the agent when
         *       the remote write is done.
         */
        ch_mr mirror_mr;
        CHECK_Z(    send_agent_mr(entry->mr));
        CHECK_Z(    receive_agent_mr(&mirror_mr));
        CHECK_Z(    remote_write(entry->mr, mirror_mr));
        CHECK_Z(    send_agent_mr(entry->mr));

        /**
         * Note: Add the remote memory-region to the list; this will be 
         *       useful for broadcast, gather, and scatter operations 
         *       since the sched keeps the list of nodes which map to 
         *       the master-copy.
         */
        register_remote_dmr(entry, mpDevice, mirror_mr);

    }
    else if ((entry->copy_type == mem_mirror) || 
        (entry->copy_type == mem_record)) {

        TRACE("\tData object '%s' is found (mirror/record)", req->data.mem_id);
        req->data.mr_valid    = false;
        req->data.did         = 
            entry->master_device->did;
        snprintf(req->data.host, MED_STR_LEN, "%s",
            entry->master_device->phy.host);
        req->data.link        = 
            entry->master_device->phy.link;
        req->data.device_type = 
            entry->master_device->phy.type;
        req->data.phy_dev_id  = 
            entry->master_device->phy.phy_dev_id;

        create_response(req, success);
        CHECK_Z(    respond_agent(req));
    }
    else {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    return 0;
}

int SchedRemoteHandler::HandleLaunchComp(sag_req * req) {

    TRACE("Launching computation from agent (%d-%d-%d, %s@%d)",
        req->jid, req->gid, req->peid, 
        req->data.comp_id, req->data.comp_config.max_pes_num);

    if (req->jid != mJid) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    sched_comp comp;
    int ret = get_computation_from_table(mpJob, req->data.comp_id, &comp);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    sched_comp comp_group = new sched_comp_t();
    *comp_group = *comp;
    comp_group->config = req->data.comp_config;
    
    sched_group group = create_new_group(
        mpJob, comp_group, req->data.rec_level);
    CHECK_Z(    register_group(mpJob, group));

    if (mpJob->intrn.handle == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_agent(req));
        return -1;
    }

    sched_event event = create_new_job_event(JOB_EVENT_START_COMP,
        mpJob, group, comp_group, sched_event_busy_wait, mpJob->intrn.handle);
    wait_event_ack(event);
    destroy_event(event);

    create_response(req, success);
    CHECK_Z(    respond_agent(req));

    return 0;
}

