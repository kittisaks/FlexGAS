#ifndef _AGENT_H
#define _AGENT_H

#include "channel.h"
#include "thread.h"
#include "sched_api_export.h"
#include "sched_agent_export.h"
#include "agent_internal.h"

class AgentLocalListener : public Thread {
public:
    AgentLocalListener();
    ~AgentLocalListener();
    void * Execute();

private:
    char         * mShmPath;
    ch_init_attr   mChAttr;
    ch             mChannel;
};

class AgentLocalHandler : public Thread {
public:
    AgentLocalHandler(ch channel);
    ~AgentLocalHandler();
    void * Execute();

private:
    inline void create_response(saa_req * req, req_status status,
        job_id jid, group_id gid, host_id hid, device_id did, pe_id peid);
    inline void create_response(saa_req * req, req_status status);
    inline int  respond_api(saa_req * req);
    inline int  handle_descendant_pe(saa_req * req, prosp_pe ppe, sched_pe pe);

    int HandleRegisterPe(saa_req * req);
    int HandleUnregisterPe(saa_req * req);
    int HandleRegisterComp(saa_req * req);
    int HandleConfigureComp(saa_req * req);
    int HandleLaunchComp(saa_req * req);
    int HandleMemAlloc(saa_req * req);
    int _HandleMemMap_QuerySched(saa_req * req, 
        bool * mr_valid, sched_mem_et * entry , sched_device * device);
    int _HandleMemMap_QueryAgent(saa_req * req, sched_device device,
        bool * mr_valid, ch_mr * mr);
    int HandleMemMap(saa_req * req);
    int HandleMemPut(saa_req * req);
    int HandleMemGet(saa_req * req);
    int HandleMemUpdate(saa_req * req);

    ch           mApiChannel;
    bool         mTerminate;

    job_id       mJid;
    prosp_pe     mPpe;
    sched_pe     mPe;
    sched_job    mpJob;
    sched_group  mpGroup;
    sched_device mpDevice;
};

class AgentRemoteListener : public Thread {
public:
    typedef enum {
        Rdma, Inet
    } ListenerChannel;

    AgentRemoteListener(ListenerChannel chan_type);
    ~AgentRemoteListener();
    void * Execute();

private:
    ListenerChannel   mChannelType;
    char            * mPort;
    ch_init_attr      mChAttr;
    ch                mChannel;
};

class AgentRemoteHandler : public Thread {
public:
    AgentRemoteHandler(ch channel);
    ~AgentRemoteHandler();
    void * Execute();

private:
    inline void         create_response(sag_req * req, req_status status);
    inline int          respond_sched(sag_req * req);
    inline sched_device finalize_creating_device(
        sag_req * req, sched_group group);
    inline int          delete_job();
    inline int          delete_group();

    int HandlerRegisterJob(sag_req * req);
    int HandlerRegisterGroup(sag_req * req);
    int HandlerRegisterDevice(sag_req * req);
    int HandlerRegister_Job_Device_Group(sag_req * req);
    int HandlerUnregisterDevice(sag_req * req);
    int HandlerUnregisterGroup(sag_req * req);
    int HandlerUnregisterJob(sag_req * req);
    int HandlerSpawnPes(sag_req * req);
    int HandlerKillPes(sag_req * req);
    int HandlerQueryMemory(sag_req * req);

    ch           mSchedChannel;
    bool         mTerminate;

    sched_job    mpJob;
    sched_group  mpGroup;
    sched_device mpDevice;
};

inline void AgentRemoteHandler::create_response(
    sag_req * req, req_status status) {

    req->status = status;
}

inline int AgentRemoteHandler::respond_sched(sag_req * req) {

    return chSend(mSchedChannel, req, sizeof(sag_req));
}

inline void create_sched_request(sag_req_type type, sag_req * req,
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

inline int submit_sched_request(sag_req * req, sched_device device) {

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

inline int send_sched_mr(ch_mr mr, sched_device device) {

    int ret;

    lock_device_internal(device);

    ret = chSendMr(device->intrn.request_ch, mr);
    if (ret != 0) {
        unlock_device_internal(device);
        return -1;
    }

    unlock_device_internal(device);

    return 0;

}

inline int receive_sched_mr(ch_mr * mr, sched_device device) {

    int ret;

    lock_device_internal(device);

    ret = chRecvMr(device->intrn.request_ch, mr);
    if (ret != 0) {
        unlock_device_internal(device);
        return -1;
    }

    unlock_device_internal(device);

    return 0;
}

#include "agent_trace.h"

inline int remote_write(sched_device device, ch_mr l_mr, ch_mr r_mr,
    void * l_addr, uint64_t length) {

    ch_event event;
    int      ret;

    uint64_t l_base_addr_u64 = (uint64_t) l_mr->addr;
    uint64_t l_end_addr_u64  = l_base_addr_u64 + l_mr->length;
    uint64_t l_addr_u64      = (uint64_t) l_addr;
    if ((l_addr_u64 < l_base_addr_u64) || (l_addr_u64 > l_end_addr_u64)) 
        return -1;

    uint64_t offset = l_addr_u64 - l_base_addr_u64;
    if (offset + length > r_mr->length) 
        return -1;

    uint64_t   r_base_addr_u64 = (uint64_t) r_mr->addr;
    uint64_t   r_addr_u64      = r_base_addr_u64 + offset;
    void     * r_addr          = (void *) r_addr_u64;

    ret = chRemoteWriteAsync(device->intrn.request_ch,
        l_mr, l_addr, r_mr, r_addr, length, &event);
    if (ret != 0) 
        return -1;

    ret = chWaitEvent(event);
    if (ret != 0) 
        return -1;
    if (event->type != ch_success) 
        return -1;

    return 0;
}

inline int remote_read(sched_device device, ch_mr l_mr, ch_mr r_mr,
    void * l_addr, uint64_t length) {

    ch_event event;
    int      ret;

    uint64_t l_base_addr_u64 = (uint64_t) l_mr->addr;
    uint64_t l_end_addr_u64  = l_base_addr_u64 + l_mr->length;
    uint64_t l_addr_u64      = (uint64_t) l_addr;
    if ((l_addr_u64 < l_base_addr_u64) || (l_addr_u64 > l_end_addr_u64))
        return -1;

    uint64_t offset = l_addr_u64 - l_base_addr_u64;
    if (offset + length > r_mr->length)
        return -1;

    uint64_t   r_base_addr_u64 = (uint64_t) r_mr->addr;
    uint64_t   r_addr_u64      = r_base_addr_u64 + offset;
    void     * r_addr          = (void *) r_addr_u64;

    ret = chRemoteReadAsync(device->intrn.request_ch,
        l_mr, l_addr, r_mr, r_addr, length, &event);
    if (ret != 0)
        return -1;

    ret = chWaitEvent(event);
    if (ret != 0)
        return -1;
    if (event->type != ch_success)
        return -1;

    return 0;
}
        

#if 0 //Unused code portion

inline int remote_read_all(ch_mr l_mr, ch_mr r_mr, sched_device device) {

    ch_event event;
    int      ret;

TRACE("AGENT %x - %x", l_mr->addr, r_mr->addr);
    ret = chRemoteReadAsync(device->intrn.request_ch,
        l_mr, l_mr->addr, r_mr, r_mr->addr, r_mr->length, &event);
    if (ret != 0)
        return -1;

    ret = chWaitEvent(event);
    if (ret != 0)
        return -1;
    if (event->type != ch_success)
        return -1;

    return 0;
}
#endif

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


#endif //_AGENT_H

