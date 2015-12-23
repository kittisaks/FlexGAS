#include "agent_threads.h"
#include "agent_trace.h"
#include <sys/types.h>
#include <sys/wait.h>

inline void AgentLocalHandler::create_response(
    saa_req * req, req_status status,
    job_id jid, group_id gid, host_id hid, device_id did, pe_id peid) {

    req->status = status;

    req->jid    = jid;
    req->gid    = gid;
    req->hid    = hid;
    req->did    = did;
    req->peid   = peid;
}

inline void AgentLocalHandler::create_response(
    saa_req * req, req_status status) {

    req->status = status;
}

inline int AgentLocalHandler::respond_api(saa_req * req) {

    return chSend(mApiChannel, req, sizeof(saa_req));
}

inline int AgentLocalHandler::handle_descendant_pe(
    saa_req * req, prosp_pe ppe, sched_pe pe) {

    int status;

    //Note: This allow the process (PE) to terminate without hanging around
    //      with the defunct state.
    waitpid(req->pid, &status, WUNTRACED);

    if (pe->is_killed != 0) {
        mTerminate = true;
        return 0;
    }

    sag_req sched_req;
    create_sched_request(
        sag_sched_notify_pe_done, &sched_req, req->pid,
        ppe.uid, ppe.jid, ppe.gid,
        SCHED_API_ROOT_HID, ppe.did, ppe.peid);

    int ret = submit_sched_request(&sched_req, ppe.device);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    mTerminate = true;

    return 0;
}

int AgentLocalHandler::HandleRegisterPe(saa_req * req) {

    prosp_pe ppe;

    if (!prosp_pe_tab.fetch(req->pid, &ppe)) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    create_response(req, success,
        ppe.jid, ppe.gid, ppe.hid, ppe.did, ppe.peid);
    req->data.comp_desc_exec = ppe.comp_desc_exec;
    ppe.notif[ppe.notif_idx] = PE_SPAWNED;
    if (ppe.peid != SCHED_API_ROOT_PEID) {
        req->data.rec_level   = ppe.rec_level;
        req->data.device_type = ppe.device->phy.type;
        req->data.phy_dev_id  = ppe.device->phy.phy_dev_id;
    }
    CHECK_Z(    respond_api(req));

    if (ppe.peid != SCHED_API_ROOT_PEID) {

        sched_pe pe = create_new_pe_id(ppe.pid, ppe.uid,
            ppe.jid, ppe.gid, ppe.hid, ppe.did, ppe.peid,
            ppe.comp, ppe.device, ppe.group, ppe.job);

        CHECK_Z(    register_pes(ppe.job, ppe.group, ppe.device, &pe, 1));

#if 0
        return handle_descendant_pe(req, ppe, pe);
#else
        mPpe     = ppe;
        mPe      = pe;
        mpJob    = ppe.job;
        mpGroup  = ppe.group;
        mpDevice = ppe.device;
        return 0;
#endif
    }

    return 0;
}

int AgentLocalHandler::HandleUnregisterPe(saa_req * req) {

    create_response(req, success);
    respond_api(req);

#if 0
    return 0;
#else
    return handle_descendant_pe(req, mPpe, mPe);
#endif
}

int AgentLocalHandler::HandleRegisterComp(saa_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): AgentLocalHandler::HandleRegisterComp */

    create_response(req, failed);
    respond_api(req);

    return 0;
}

int AgentLocalHandler::HandleConfigureComp(saa_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): AgentLocalHandler::HandleConfigureComp */

    create_response(req, failed);
    respond_api(req);

    return 0;
}

int AgentLocalHandler::HandleLaunchComp(saa_req * req) {

    sag_req sched_req;
    create_sched_request(
        sag_sched_launch_comp, &sched_req, req->pid,
        req->uid, req->jid, req->gid,
        SCHED_API_ROOT_HID, req->did, req->peid);
    snprintf(sched_req.data.comp_id, MED_STR_LEN, "%s", req->data.comp_id);
    sched_req.data.comp_config = req->data.comp_config;
    sched_req.data.rec_level   = mpGroup->rec_level;

    submit_sched_request(&sched_req, mpDevice);

    create_response(req, success);
    respond_api(req);

    return 0;
}

int AgentLocalHandler::HandleMemAlloc(saa_req * req) {

    /**
     * Note: The memory entry created with this routine can be added
     *       either to the memory tables of the job or of the group,
     *       depending on the specified flag: IVM_MEM_GLOBAL, IVM_MEM_LOCAL.
     */
    TRACE("Allocating Memory (Agent)");

    //Check whether the memory object already exists.
    sched_mem_et entry = find_memory_entry(mpGroup, req->data.mem_id);
    if (entry != NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    //Allocate the memory object and create the corresponding memory entry.
    int ret = allocate_master_object(mpJob->jid, mpGroup->gid, mpDevice->did,
        mpJob, mpGroup, mpDevice, mApiChannel, req->data.mem_size,
        req->data.mem_id, req->data.mem_flags, &entry);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    //Register the memory entry with the local memory table.
    ret = register_mem_entry(entry, req->data.mem_flags);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    //Inform the scheduler to update the corresponding memory table for 
    //providing the correct name service.
    sag_req sched_req;
    create_sched_request(
        sag_sched_add_memory_entry, &sched_req, req->pid,
        req->uid, req->jid, req->gid,
        SCHED_API_ROOT_HID, req->did, req->peid);
    sched_req.data.mem_flags = req->data.mem_flags;
    sched_req.data.mem_size = req->data.mem_size;
    snprintf(sched_req.data.mem_id, MED_STR_LEN, "%s", req->data.mem_id);

    ret = submit_sched_request(&sched_req, mpDevice);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    //Provide back to the API a memory-region.
    memcpy(&req->data.mr, entry->mr, sizeof(ch_mr_t));

    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

int AgentLocalHandler::_HandleMemMap_QuerySched(
        saa_req * req, bool * mr_valid, 
        sched_mem_et * entry, sched_device * device) {

    //Query the scheduler whether the requested memory object exists.
    //The scheduler will reply whether the object exists and where to
    //find it. In case of the scheduler owning the object, it replies
    //back with the memory-region. In another case in which the scheduler
    //does not own the object, it replies back with the information of
    //of the deviceowning the object.
    sag_req sched_req;
    create_sched_request(
        sag_sched_query_memory, &sched_req, req->pid,
        req->uid, req->jid, req->gid,
        SCHED_API_ROOT_HID, req->did, req->peid);
    sched_req.data.mem_flags = req->data.mem_flags;
    snprintf(sched_req.data.mem_id, MED_STR_LEN, "%s", req->data.mem_id);

    int ret = submit_sched_request(&sched_req, mpDevice);
    if (ret != 0) 
        return -1;
    if (sched_req.status != success)
        return -1;

    //Expecting a memory-region or device information
    if (sched_req.data.mr_valid) {

        sched_mem_et entry_tmp;
        ch_mr        mr_tmp;

        //Receive the remote memory-region
        if (receive_sched_mr(&mr_tmp, mpDevice) != 0)
            return -1;

        //Create the mirror-copy
        ret = allocate_mirror_object(
            mpJob->jid, mpGroup->gid, mpDevice->did,
            mpJob, mpGroup, mpDevice, mpDevice, mr_tmp, 
            mApiChannel, mr_tmp->length, req->data.mem_id,
            req->data.mem_flags, &entry_tmp);
        if (ret != 0)
            return -1;

        //Register the memory entry of the mirror-object
        register_mem_entry(entry_tmp, req->data.mem_flags);

        //Send back the local memory-region
        if (send_sched_mr(entry_tmp->mr, mpDevice) != 0)
            return -1;

        //Wait for remote write completion
        if (receive_sched_mr(&mr_tmp, mpDevice) != 0)
            return -1;

        *mr_valid    = sched_req.data.mr_valid;
        *entry       = entry_tmp;
        *device      = NULL;
    }
    else {
        char       * remote_host;
        char       * service;
        sched_link   link;

        remote_host = sched_req.data.host;
        TRACE("REMOTE HOST: %s", remote_host);

        switch (sched_req.data.link) {
#ifndef _DISABLE_LM_RDMA
        case ch_link_rdma:
            service = agts_get_agent_rdma_port();
            link    = sched_link_rdma;
            break;
#endif //_DISABLE_LM_RDMA
        case ch_link_inet:
            service = agts_get_agent_inet_port();
            link    = sched_link_inet;
            break;
        default:
            create_response(req, failed);
            CHECK_Z(    respond_api(req));
            return -1;
        }

        sched_device device_tmp = create_new_device_id(
            remote_host, service, link, sched_req.data.device_type,
            sched_req.data.phy_dev_id, mpGroup, sched_req.data.did);
        if (device_tmp == NULL) {
            create_response(req, failed);
            CHECK_Z(    respond_api(req));
            return -1;
        }

        *mr_valid = sched_req.data.mr_valid;
        *entry    = NULL;
        *device   = device_tmp;
    }

    return 0;
}

int AgentLocalHandler::_HandleMemMap_QueryAgent(saa_req * req,
    sched_device device, bool * mr_valid, ch_mr * mr) {

    sag_req agent_req;
    create_sched_request(
        sag_agent_query_memory, &agent_req, req->pid,
        req->uid, req->jid, req->gid,
        SCHED_API_ROOT_HID, req->did, req->peid);
    agent_req.data.mem_flags = req->data.mem_flags;
    snprintf(agent_req.data.mem_id, MED_STR_LEN, "%s", req->data.mem_id);

    int ret = submit_sched_request(&agent_req, device);
    if (ret != 0) 
        return -1;
    if (agent_req.status != success)
        return -1;
    
    return 0;
}

int AgentLocalHandler::HandleMemMap(saa_req * req) {

    TRACE("Mapping Memory (Agent) (%d)", req->peid);
    TRACE("\tData object: %s", req->data.mem_id);

    //Search for the memory entry locally. If the memory entry is not
    //found, then query the scheduler for the memory entry.
    bool         mr_valid;
    sched_device device;
    ch_mr        mr;

    sched_mem_et entry = find_memory_entry(
        mpGroup, req->data.mem_id, req->data.mem_flags);
    if (entry == NULL) {
        if (_HandleMemMap_QuerySched(
            req, &mr_valid, &entry, &device) != 0) {

            create_response(req, failed);
            CHECK_Z(    respond_api(req));
            return -1;
        }

        if (!mr_valid) {
            TRACE("CONTACTING DEVICE");
            if (_HandleMemMap_QueryAgent(
                req, device, &mr_valid, &mr) != 0) {

                create_response(req, failed);
                CHECK_Z(    respond_api(req));
                return -1;
            }
            //contact master node
            //allocate mirror
            //get master content
            //create memory entry
            //*send back mr
            entry = new sched_mem_et_t();
            entry->mr = new ch_mr_t();
        }
        else {
            TRACE("\tCreating mirror object for '%s'. (%d)", 
                req->data.mem_id, req->peid);
        }
    }
    //Provide back to the API a memory-region.
    memcpy(&req->data.mr, entry->mr, sizeof(ch_mr_t));
    
    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

#include <iostream>
using namespace std;

int AgentLocalHandler::HandleMemPut(saa_req * req) {

    TRACE("Mem put (Agent)");

    sched_mem_et entry = find_memory_entry(
        mpGroup, req->data.mem_id, req->data.mem_flags);
    if (entry == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    if (entry->copy_type == mem_master) {
        create_response(req, success);
        CHECK_Z(    respond_api(req));
        return 0;
    }

    //Mirror-copy
    ch_mr      l_mr     = entry->mr; 
    ch_mr      r_mr     = entry->remote_device.begin()->mr;
    uint64_t   base_u64 = (uint64_t) l_mr->addr;
    uint64_t   addr_u64 = base_u64 + req->data.mem_offset;
    void     * addr     = (void *) addr_u64;

    int ret = remote_write(
        mpDevice, l_mr, r_mr, addr, req->data.mem_size);
    if (ret != 0) {
        create_response(req,failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

int AgentLocalHandler::HandleMemGet(saa_req * req) {

    TRACE("Mem get (Agent)");

    sched_mem_et entry = find_memory_entry(
        mpGroup, req->data.mem_id, req->data.mem_flags);
    if (entry == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    if (entry->copy_type == mem_master) {
        create_response(req, success);
        CHECK_Z(    respond_api(req));
        return 0;
    }

    //Mirror-copy
    ch_mr      l_mr     = entry->mr;
    ch_mr      r_mr     = entry->remote_device.begin()->mr;
    uint64_t   base_u64 = (uint64_t) l_mr->addr;
    uint64_t   addr_u64 = base_u64 + req->data.mem_offset;
    void     * addr     = (void *) addr_u64;

    create_response(req, failed);
    CHECK_Z(    respond_api(req));

    int ret = remote_read(
        mpDevice, l_mr, r_mr, addr, req->data.mem_size);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_api(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_api(req));

    return 0;
}

///* //TO-BE-REMOVED (TBR)

#include <iostream>
using namespace std;

//*/

int AgentLocalHandler::HandleMemUpdate(saa_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): AgentLocalHandler::HandleMemUpdate */

    TRACE("Memory update (Agent)");
//    cout << "cout:: Memory update (Agent)" << endl;

    create_response(req, failed);
    CHECK_Z(    respond_api(req));

    return 0;
}

