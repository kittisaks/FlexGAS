#include "agent_threads.h"
#include "agent_settings.h"
#include "agent_trace.h"
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

char              local_hostname[MED_STR_LEN];
job_table         job_tab;
pthread_mutex_t   job_tab_mutex;
volatile job_id   job_id_cnt = 0;
prosp_pe_table    prosp_pe_tab;

/******************************************************************************
 * AgentLocalListener
 *****************************************************************************/

AgentLocalListener::AgentLocalListener() {
    char * agent_name;
    if (strlen(local_hostname) > 16) {
        agent_name = new char [24];
        for (int idx=0;idx<16;idx++)
            agent_name[idx] = local_hostname[idx];
        agent_name[17] = '\0';    
        agt.SetComponentName(agent_name);
    }
    else
        agt.SetComponentName(local_hostname);
}

AgentLocalListener::~AgentLocalListener() {
}

void * AgentLocalListener::Execute() {

    mChAttr.link    = ch_link_shm;
    mChAttr.self_id = agts_get_shm_path();
    unlink(mChAttr.self_id);

    ch_event event;
    chOpen(&mChannel, mChAttr, 1);

    TRACE("Local listener started (%s).", mChAttr.self_id);

    do {
        //Wait for new connection from local PEs.
        chWaitChannel(&event, mChannel);
        if (event->type != ch_new_connection) {
            TRACE_ERR("Invalid event (local)");
            continue;
        }

        //Accpet and prepare to send the connection to local handler.
        ch client_channel = event->content.channel;
        chWaitChannel(&event, client_channel);
        if (event->type != ch_established) {
            TRACE_ERR("Invalid incoming connection (local)");
            continue;
        }

        //Pass the connection to the local handler
        AgentLocalHandler * local_handler =
            new AgentLocalHandler(client_channel);
        local_handler->Start();

    } while(1);


    return NULL;
}

/******************************************************************************
 * AgentLocalHandler
 *****************************************************************************/

AgentLocalHandler::AgentLocalHandler(ch channel) {
    mApiChannel = channel;
    mTerminate  = false;
}

AgentLocalHandler::~AgentLocalHandler() {
}

void * AgentLocalHandler::Execute() {

    TRACE("Local handler created.");

    int ret = 0;
    do {
        saa_req req;
        ret = chRecv(mApiChannel, &req, sizeof(saa_req));
        if (ret != 0) {
            TRACE_ERR("Communication error (Agent/Loc)");
            break;
        }

        if (req.type != local) {
            TRACE_ERR("Communication error: invalid request (Agent/Loc)");
            break;
        }

        int ret;
        switch(req.data.type) {
        case saa_register_pe:
            HandleRegisterPe(&req);
            break;
        case saa_unregister_pe:
            ret = HandleUnregisterPe(&req);
            if (ret == 0)
                mTerminate = true;
            break;
        case saa_register_comp:
            HandleRegisterComp(&req);
            break;
        case saa_configure_comp:
            HandleConfigureComp(&req);
            break;
        case saa_launch_comp:
            HandleLaunchComp(&req);
            break;
        case saa_mem_alloc:
            HandleMemAlloc(&req);
            break;
        case saa_mem_map:
            HandleMemMap(&req);
            break;
        case saa_mem_put:
            HandleMemPut(&req);
            break;
        case saa_mem_get:
            HandleMemGet(&req);
            break;
        case saa_mem_update:
            HandleMemUpdate(&req);
            break;
        default:
            break;
        }

    } while(!mTerminate);

    ret = chClose(mApiChannel);
    if (ret != 0)
        TRACE_ERR("Error closing local handler channel");

    TRACE("Local handler exited.");

    return NULL;
}


/******************************************************************************
 * AgentRemoteListener
 *****************************************************************************/

AgentRemoteListener::AgentRemoteListener(ListenerChannel chan_type) {

    if (chan_type == Inet)
        mChannelType = Inet;
    else if (chan_type == Rdma)
        mChannelType = Rdma;
    else
        throw "Invalid type of listener channel";
}

AgentRemoteListener::~AgentRemoteListener() {
}

void * AgentRemoteListener::Execute() {

    if (mChannelType == Inet) {
        mChAttr.link      = ch_link_inet;
        mChAttr.self_port = agts_get_inet_port();
    }
#ifndef _DISABLE_LM_RDMA
    else if (mChannelType == Rdma) {
        mChAttr.link      = ch_link_rdma;
        mChAttr.self_port = agts_get_rdma_port();
    }
#endif //_DISABLE_LM_RDMA
    else {
        TRACE_ERR("Invalid type of listener channel");
        return NULL;
    }

    ch_event event;
    chOpen(&mChannel, mChAttr, 1);

    TRACE("Remote listener started (%s).", mChAttr.self_port);

    do {
        //Wait for new connection from local PEs.
        chWaitChannel(&event, mChannel);
        if (event->type != ch_new_connection) {
            TRACE_ERR("Invalid event (remote)");
            continue;
        }

        //Accpet and prepare to send the connection to local handler.
        ch client_channel = event->content.channel;
        chWaitChannel(&event, client_channel);
        if (event->type != ch_established) {
            TRACE_ERR("Invalid incoming connection (remote)");
            continue;
        }

        //Pass the connection to the local handler
        AgentRemoteHandler * remote_handler =
            new AgentRemoteHandler(client_channel);
        remote_handler->Start();

    } while (1);


    return NULL;
}

/******************************************************************************
 * AgentRemoteHandler
 *****************************************************************************/

AgentRemoteHandler::AgentRemoteHandler(ch channel) {

    mSchedChannel = channel;
    mTerminate    = false;
}

AgentRemoteHandler::~AgentRemoteHandler() {
}

void * AgentRemoteHandler::Execute() {

    TRACE("Remote handler created.");

    int ret = 0;
    do {
        sag_req req;
        ret = chRecv(mSchedChannel, &req, sizeof(sag_req));
        if (ret != 0) {
            TRACE_ERR("Communication error (Agent/Rem)");
            break;
        }

        if (req.type != remote) {
            TRACE_ERR("Communication error: invalid request (Agent/Rem)");
            break;
        }

        switch(req.data.type) {
        case sag_agent_register_job:
            HandlerRegisterJob(&req);
            break;
        case sag_agent_register_group:
            HandlerRegisterGroup(&req);
            break;
        case sag_agent_register_device:
            HandlerRegisterDevice(&req);
            break;
        case sag_agent_register_jgd:
            HandlerRegister_Job_Device_Group(&req);
            break;
        case sag_agent_unregister_device:
            HandlerUnregisterDevice(&req);
            break;
        case sag_agent_unregister_group:
            HandlerUnregisterGroup(&req);
            break;
        case sag_agent_unregister_job:
            HandlerUnregisterJob(&req);
            break;
        case sag_agent_spawn_pes:
            HandlerSpawnPes(&req);
            break;
        case sag_agent_kill_pes:
            HandlerKillPes(&req);
            break;
        case sag_agent_query_memory:
            HandlerQueryMemory(&req);
            break;
        default:
            TRACE_ERR("Agent is not supposed to handle sched-request");
            create_response(&req, failed);
            respond_sched(&req);
            break;
        }
    } while(!mTerminate);

    ret = chClose(mSchedChannel);
    if (ret != 0)
        TRACE_ERR("Error closing remote handler channel");

    TRACE("Remote handler exited.");

    return NULL;
}

