#ifndef _SCHED_THREADS_H
#define _SCHED_THREADS_H

#include "thread.h"
#include "channel.h"
#include "sched_agent_export.h"
#include "sched_api_export.h"
#include "sched_internal.h"
#include "sched_settings.h"
#include "sched_trace.h"

class SchedLocalListener : public Thread {
public:
    SchedLocalListener();
    ~SchedLocalListener();
    void * Execute();

private:
    char         * mShmPath;
    ch_init_attr   mChAttr;
    ch             mChannel;
};

class SchedLocalHandler : public Thread {
public:
    SchedLocalHandler(ch channel);
    ~SchedLocalHandler();
    void * Execute();

private:
    inline void create_response(saa_req * req, req_status status,
        job_id jid, group_id gid, host_id hid, device_id did, pe_id peid);
    inline void create_response(saa_req * req, req_status status);
    inline int  respond_api(saa_req * req);

    int HandleRegisterPe(saa_req * req);
    int HandleUnregisterPe(saa_req * req);
    int HandleRegisterComp(saa_req * req);
    int HandleConfigureComp(saa_req * req);
    int HandleLaunchComp(saa_req * req);
    int HandleMemAlloc(saa_req * req);
    int HandleMemMap(saa_req * req);
    int HandleMemPut(saa_req * req);
    int HandleMemGet(saa_req * req);
    int HandleMemUpdate(saa_req * req);

    ch_init_attr   mChAttr;
    ch             mApiChannel;
    bool           mTerminate;

    job_id         mJid;
    sched_job      mpJob;
};

class SchedRemoteListener : public Thread {
public:
    typedef enum {
        Rdma, Inet
    } ListenerChannel;

    SchedRemoteListener(ListenerChannel chan_type);
    ~SchedRemoteListener();
    void * Execute();

private:
    ListenerChannel   mChannelType;
    char            * mPort;
    ch_init_attr      mChAttr;
    ch                mChannel;
};

class SchedRemoteHandler : public Thread {
public:
    SchedRemoteHandler(ch channel);
    ~SchedRemoteHandler();
    void * Execute();

private:
    inline void create_response(sag_req * req, req_status status);
    inline int  respond_agent(sag_req * req);
    inline int  send_agent_mr(ch_mr mr);
    inline int  receive_agent_mr(ch_mr * mr);
    inline int  remote_write(ch_mr l_mr, ch_mr r_mr);
    int HandleRegisterAgent(sag_req * req);
    int HandleUnregisterAgent(sag_req * req);
    int HandleNotifyPeDone(sag_req * req);
    int HandleAddMemoryEntry(sag_req * req);
    int HandleRemoveMemoryEntry(sag_req * req);
    int HandleQueryMemory(sag_req * req);
    int HandleLaunchComp(sag_req * req);

    ch_init_attr   mAttr;
    ch             mAgentChannel;
    bool           mTerminate;

    job_id         mJid;
    sched_job      mpJob;
    sched_group    mpGroup;
    group_id       mGid;
    sched_device   mpDevice;
    device_id      mDid;
};

inline void SchedRemoteHandler::create_response(
    sag_req * req, req_status status) {

    req->status = status;
}

inline int SchedRemoteHandler::respond_agent(sag_req * req) {

    return chSend(mAgentChannel, req, sizeof(sag_req));
}

inline int SchedRemoteHandler::send_agent_mr(ch_mr mr) {

    return chSendMr(mAgentChannel, mr);
}

inline int SchedRemoteHandler::receive_agent_mr(ch_mr * mr) {

    return chRecvMr(mAgentChannel, mr);
}

inline int SchedRemoteHandler::remote_write(ch_mr l_mr, ch_mr r_mr) {

    ch_event event;
    int      ret;

    ret = chRemoteWriteAsync(mAgentChannel, l_mr, l_mr->addr,
        r_mr, r_mr->addr, r_mr->length, &event);
    if (ret != 0)
        return -1;

    ret = chWaitEvent(event);
    if (ret != 0)
        return -1;
    if (event->type != ch_success)
        return -1;

    return 0;      

}

class SchedScheduler : public Thread {
public:
    SchedScheduler();
    SchedScheduler(const char * lib);
    ~SchedScheduler();
    void * Execute();

private:
    char mLibName[256];
};

#endif //_SCHED_THREADS_H

