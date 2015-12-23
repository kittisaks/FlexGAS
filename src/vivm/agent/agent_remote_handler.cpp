#include "agent_threads.h"
#include "agent_trace.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int AgentRemoteHandler::HandlerRegisterJob(sag_req * req) {

    sched_job job = find_job(req->jid);
    if (job == NULL) {
        job = create_new_job_id(reinterpret_cast<saa_req *>(req), req->jid);
        register_job(job);
    }

    if (job == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_sched(req));

    return 0;
}

int AgentRemoteHandler::HandlerRegisterGroup(sag_req * req) {

    sched_job job = find_job(req->jid);
    if (job == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    sched_group  group = find_group(job, req->gid);
    if (group == NULL) {
        sched_comp comp = new sched_comp_t();
        *comp = req->data.comp;

        group = create_new_group_id(
            job, comp, req->gid, req->data.rec_level);
        CHECK_Z(    register_group(job, group));
    }

    if (group == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_sched(req));

    return 0;
}

sched_device AgentRemoteHandler::finalize_creating_device(
    sag_req * req, sched_group group) {

    char       * remote_host = mSchedChannel->init_attr.peer_id;
    char       * service;
    sched_link   link;
    switch (req->data.link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        service = agts_get_sched_rdma_port();
        link    = sched_link_rdma;
        break;
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        service = agts_get_sched_inet_port();
        link    = sched_link_inet;
        break;
    default:
        return NULL;
    }

    sched_device device_tmp = create_new_device_id(remote_host, service, link,
        req->data.device_type, req->data.phy_dev_id, group, req->data.did);
    if (device_tmp == NULL)
        return NULL;
    register_device(group, device_tmp);

    sag_req sched_req;
    create_sched_request(
        sag_sched_register_agent, &sched_req, device_tmp->pid, device_tmp->uid, 
        device_tmp->jid, device_tmp->gid, SCHED_API_ROOT_HID, device_tmp->did,
        SCHED_API_ROOT_PEID);
    int ret = submit_sched_request(&sched_req, device_tmp);
    if (ret != 0) {
        unregister_device(group, device_tmp);
        return NULL;
    }
    if (sched_req.status != success) {
        unregister_device(group, device_tmp);
        return NULL;
    }

    mpJob    = group->job;
    mpGroup  = group;
    mpDevice = device_tmp;

    return device_tmp;
}

int AgentRemoteHandler::HandlerRegisterDevice(sag_req * req) {

    sched_job job = find_job(req->jid);
    if (job == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    sched_group group = find_group(job, req->gid);
    if (group == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    sched_device device = find_device(group, req->did);
    if (device != NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    device = finalize_creating_device(req, group);
    if (device == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_sched(req));

    return 0;
}

int AgentRemoteHandler::HandlerRegister_Job_Device_Group(sag_req * req) {

    sched_job job = find_job(req->jid);
    if (job == NULL) {
        job = create_new_job_id(reinterpret_cast<saa_req *>(req), req->jid);
        register_job(job);
    }

    if (job == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    sched_group  group = find_group(job, req->gid);
    if (group == NULL) {
        sched_comp comp = new sched_comp_t();
        *comp = req->data.comp;

        group = create_new_group_id(
            job, comp, req->gid, req->data.rec_level);
        CHECK_Z(    register_group(job, group));
    }

    if (group == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    sched_device device = find_device(group, req->did);
    if (device != NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    device = finalize_creating_device(req, group);
    if (device == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_sched(req));

    return 0;
}

/**
 * ISSUE:
 */

inline int AgentRemoteHandler::delete_job() {

    TRACE("DELETING JOB");

    unregister_job(mpJob);
    delete mpJob;

    return 0;
}

#if 0 //RACING-DELETETION
inline int AgentRemoteHandler::delete_group() {

    TRACE("DELETING GROUP");

    unregister_group(mpJob, mpGroup);
//    delete mpGroup;

    if (is_group_tab_empty(mpJob)) {
//        delete_job();
    }

    return 0;
}
#else
inline int AgentRemoteHandler::delete_group() {

    TRACE("DELETING GROUP");

    CHECK_Z(    unregister_device(mpGroup, mpDevice));
    lock_device_tab(mpGroup);
    if (mpGroup->device_tab.size() > 0) {
        unlock_device_tab(mpGroup);
        return 0;
    }

    unregister_group(mpJob, mpGroup);
    delete mpGroup;

    if (is_group_tab_empty(mpJob)) {
        delete_job();
    }

    unlock_device_tab(mpGroup);

    return 0;
}
#endif

int AgentRemoteHandler::HandlerUnregisterDevice(sag_req * req) {

    TRACE("Handler Unregister Device");

    if (mpJob->jid != req->jid) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    if (mpGroup->gid != req->gid) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    if (mpDevice->did != req->data.did) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    sched_device device = find_device(mpGroup, req->data.did);
    if (device == NULL) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_sched(req));
    sleep(1);

    sag_req sched_req;
    create_sched_request(
        sag_sched_unregister_agent, &sched_req, device->pid, 
        device->uid, device->jid, device->gid,
        SCHED_API_ROOT_HID, device->did, SCHED_API_ROOT_PEID);

    CHECK_Z(    submit_sched_request(&sched_req, device));
    if (sched_req.status != success)
        return -1;

#if 0 //RACING-DELETION
    CHECK_Z(    unregister_device(mpGroup, mpDevice));
#endif


#if 0 //RACING-DELETION
    if (is_device_tab_empty(mpGroup)) {
        delete_group();
    }
#else
    delete_group();
    destroy_device(mpDevice);
#endif

    mTerminate = true;

    return 0;
}

int AgentRemoteHandler::HandlerUnregisterGroup(sag_req * req) {

    return 0;
}

int AgentRemoteHandler::HandlerUnregisterJob(sag_req * req) {

    return 0;
}

inline char ** unmarshal_prog_arg(sched_job job) {

    char ** argv_tmp = new char * [job->argc + 1];
    size_t offset = 0, noffset;

    for(int idx=0;idx<job->argc;idx++) {
        argv_tmp[idx] = &job->argv[offset];
        noffset = offset + strlen(&job->argv[offset]) + 1;
        offset = noffset;
    }

    argv_tmp[job->argc] = NULL;

    return argv_tmp;
}

//#include <iostream>
//using namespace std;
inline int create_pes(sag_req * req, sched_device device) {

    char    binary[MAX_STR_LEN];
    volatile char  * notif;

retry:
//char hostname[64]; gethostname(hostname, 64);
//cout << "\tcreate_pes 0: " << hostname << endl;

    comp_iter it = device->group->comp_tab.find(
        std::string(req->data.comp.comp_id));
    if (it == device->group->comp_tab.end()) 
        return -1;

//cout << "\tcreate_pes 1: " << hostname << endl;
    sched_comp comp = it->second;

#define _FORK_FOR_IB            

#ifndef _FORK_FOR_IB
    char ** argv;

    argv = unmarshal_prog_arg(device->job);
#endif
//cout << "\tcreate_pes 2: " << hostname << endl;
    sprintf(binary, "%s%s", device->job->path, device->job->bin);
    notif = new volatile char [req->data.num_pes];
    memset((void *) notif, PE_PENDING, req->data.num_pes);

    for (int idx=0;idx<req->data.num_pes;idx++) {
        pid_t pid = fork();
        if (pid == 0) {
//cout << "child " << getpid() << " 1" << endl;
            setsid();
//cout << "child " << getpid() << " 2" << endl;
            //close(1);
            //close(2);
#ifdef _FORK_FOR_IB
            /**
             * ISSUE: Because that libibverbs may have some limitation with
             *        system call such as fork(), I suspect that the environ-
             *        ment variable (IBV_FORK_SAFE) that enable libibverbs 
             *        to work with fork() could do some tweaking with the 
             *        kernel internal data structures. This could nullify
             *        all the memory locations and buffers a child process
             *        inherits from its parent. That's why 'argv' is 
             *        considered invalid in the child process and may 
             *        cause the the process to hang if 'argv' (or others) 
             *        is dereferenced.
             *
             *        Because of this limitation, passing argument to child
             *        processes is not possible with Infiniband connection.
             */
            char * ib_argv[] = {NULL};
//cout << "child " << getpid() << " 3" << endl;
            execv(binary, ib_argv);
//cout << "child " << getpid() << " 4" << endl;
#else
            execv(binary, argv);
#endif
            TRACE("THIS MESSAGE SHOULD NOT BE REACHED (EXECV - %d)", errno);
            exit(-1);
        }
        else if (pid > 0) {
//cout << "\tcreate_pes 3: " << hostname << "-" << pid << endl;
//cout << "I AM NOT SURE WHAT IS WRONG WITH THIS" << endl;
            //Register the process in the prospective-PE table.
            //So that when the PE connect locally to the agent,
            //the agent can recognize the process and assign
            //correct IDs and computation kernel to it.
            prosp_pe ppe;
            ppe.job            = device->job;
            ppe.group          = device->group;
            ppe.device         = device;
            ppe.pid            = pid;
            ppe.uid            = device->job->uid;
            ppe.jid            = device->jid;
            ppe.gid            = device->gid;
            ppe.hid            = SCHED_API_ROOT_HID;
            ppe.did            = device->did;
            ppe.peid           = req->data.s_peid + idx;
            ppe.rec_level      = device->group->rec_level;
            ppe.comp           = comp;
            ppe.comp_desc_exec = comp->ptr;
            ppe.responder      = pthread_self();
            ppe.notif          = notif;
            ppe.notif_idx      = idx;
            prosp_pe_tab.insert(std::make_pair(pid, ppe));
        }
        else {
//cout << "\tSOMETHING WRONG HAPPENED" << endl;
        }
    }

//cout << "\tcreate_pes 4: " << hostname << endl;
    //TODO: Add timer to this
    /**
     * ISSUE:
     */
    volatile char spawned;
    uint64_t cnt = 0;
    do {
        spawned = true;
        for (int idx=0;idx<req->data.num_pes;idx++)
            spawned &= notif[idx];
        cnt++;
        if (cnt > 1000000000)
            goto retry;
    } while (!spawned);
//cout << "\tcreate_pes 5: " << hostname << endl;

    return 0;
}

int AgentRemoteHandler::HandlerSpawnPes(sag_req * req) {

    char hostname[64]; gethostname(hostname, 64);

    if (req->data.device_type_pes != mpDevice->phy.type) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }
    if (req->data.phy_dev_id_pes != mpDevice->phy.phy_dev_id) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    int ret = create_pes(req, mpDevice);
    if (ret != 0) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    create_response(req, success);
    CHECK_Z(    respond_sched(req));

    return 0;
}

int AgentRemoteHandler::HandlerKillPes(sag_req * req) {

    pe_iter it = mpGroup->pe_tab.find(req->peid);
    if (it == mpGroup->pe_tab.end()) {
        create_response(req, failed);
        CHECK_Z(    respond_sched(req));
        return -1;
    }

    sched_pe pe = it->second;
    kill(pe->pid, SIGKILL);

    int status;
    pe->is_killed = 1;
    waitpid(pe->pid, &status, WUNTRACED);

    create_response(req, success);
    CHECK_Z(    respond_sched(req));

    return 0;
}

int AgentRemoteHandler::HandlerQueryMemory(sag_req * req) {

    /* NOT-YET-IMPLEMENTED (NYI): AgentRemoteHandler::HandlerQueryMemory */

    TRACE("HANDLING QUERY MEMORY (AGENT)");

    create_response(req, success);
    CHECK_Z(    respond_sched(req));

    return 0;
}

