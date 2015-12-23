#include "sched_threads.h"
#include "sched_api.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

/******************************************************************************
 * SchedLocalListener
 *****************************************************************************/

SchedLocalListener::SchedLocalListener() {
    scht.SetComponentName("Sched");
}

SchedLocalListener::~SchedLocalListener() {
}

void * SchedLocalListener::Execute() {

    mChAttr.link    = ch_link_shm;
    mChAttr.self_id = scheds_get_shm_path();
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

        //Pass the connection to the handler
        SchedLocalHandler * local_handler =
            new SchedLocalHandler(client_channel);
        local_handler->Start();

    } while(1);

    return NULL;
}

/*****************************************************************************
 * SchedLocalHandler
 ****************************************************************************/

SchedLocalHandler::SchedLocalHandler(ch channel) {
    mApiChannel = channel;
    mTerminate  = false;
}

SchedLocalHandler::~SchedLocalHandler() {
}

void * SchedLocalHandler::Execute() {

        TRACE("Local handler created.");

        int ret;
        do {
            saa_req req;
            ret = chRecv(mApiChannel, &req, sizeof(saa_req));
            if (ret != 0) {
                TRACE_ERR("Communication error (Sched/Loc)");
                break;
            }

            if (req.type != local) {
                TRACE_ERR("Communication error: invalid request (Sched/Loc)");
                break;
            }

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
        TRACE_ERR("Local handler disconnect error");
    TRACE("Local handler terminated.");

    return NULL;
}

/******************************************************************************
 * SchedRemoteListener
 *****************************************************************************/

SchedRemoteListener::SchedRemoteListener(ListenerChannel chan_type) {

    if (chan_type == Inet)
        mChannelType = Inet;
    else if (chan_type == Rdma)
        mChannelType = Rdma;
    else
        throw "Invalid type of listener channel";
}

SchedRemoteListener::~SchedRemoteListener() {
}

void * SchedRemoteListener::Execute() {

    if (mChannelType == Inet) {
        mChAttr.link      = ch_link_inet;
        mChAttr.self_port = scheds_get_inet_port();
    }
#ifndef _DISABLE_LM_RDMA
    else if (mChannelType == Rdma) {
        mChAttr.link      = ch_link_rdma;
        mChAttr.self_port = scheds_get_rdma_port();
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

        //Pass the connection to the handler
        SchedRemoteHandler * remote_handler =
            new SchedRemoteHandler(client_channel);
        remote_handler->Start();

    } while (1);


    return NULL;
}

/*****************************************************************************
 * SchedRemoteHandler
 ****************************************************************************/

SchedRemoteHandler::SchedRemoteHandler(ch channel) {
    mAgentChannel = channel;
    mpJob         = NULL;
    mpGroup       = NULL;
    mpDevice      = NULL;
    mTerminate    = false;
}

SchedRemoteHandler::~SchedRemoteHandler() {
}

void * SchedRemoteHandler::Execute() {

    TRACE("Remote handler created.");

    int ret;
    do {
        sag_req req;
        ret = chRecv(mAgentChannel, &req, sizeof(sag_req));
        if (ret != 0) {
            TRACE_ERR("Communication error (Sched/Rem)");
            break;
        }

        if (req.type != remote) {
            TRACE_ERR("Communication error: invalid request (Sched/Rem)");
            break;
        }

        switch(req.data.type) {
        case sag_sched_register_agent:
            HandleRegisterAgent(&req);
            break;
        case sag_sched_unregister_agent:
            HandleUnregisterAgent(&req);
            break;
        case sag_sched_notify_pe_done:
            HandleNotifyPeDone(&req);
            break;
        case sag_sched_add_memory_entry:
            HandleAddMemoryEntry(&req);
            break;
        case sag_sched_remove_memory_entry:
            HandleRemoveMemoryEntry(&req);
            break;
        case sag_sched_query_memory:
            HandleQueryMemory(&req);
            break;
        case sag_sched_launch_comp:
            HandleLaunchComp(&req);
            break;
        default:
            TRACE_ERR("Sched is not supposed to handle agent-request");
            create_response(&req, failed);
            respond_agent(&req);
            break;
        }
    } while (!mTerminate);

    TRACE("Remote handler exited.");

    return NULL;
}

/*****************************************************************************
 * SchedScheduler
 ****************************************************************************/

SchedScheduler::SchedScheduler() {

    sprintf(mLibName, "schedlib.so");
}

SchedScheduler::SchedScheduler(const char * lib) {

    sprintf(mLibName, "%s", lib);
}

SchedScheduler::~SchedScheduler() {
}

inline int load_scheduling_library(
    const char * lib_name, void ** lib_handle, fnt_sched_main * sched_main) {

    void * lib_handle_tmp = dlopen(lib_name , RTLD_NOW);
    if (lib_handle_tmp == NULL) {
        TRACE("Cannot find '%s' library: %s", lib_name, dlerror());
        return -1;
    }

    fnt_sched_main sched_main_tmp = (fnt_sched_main) 
        dlsym(lib_handle_tmp, SCHED_MAIN_NAME);
    if (sched_main_tmp == NULL) {
        TRACE("Cannot find reference to 'sched_main()'");
        return -1;
    }

    *lib_handle = lib_handle_tmp;
    *sched_main = sched_main_tmp;

    return 0;
}

inline int patch_schedlib_dynamic_symbols(void * lib_handle) {
#define PATCH(fn_name, fn_name_ptr)             \
    do {                                        \
        fnt_##fn_name * ptr = (fnt_##fn_name *) \
            dlsym(lib_handle, ""#fn_name_ptr);  \
        if (ptr == NULL)                        \
            return -1;                          \
        *ptr = fn_name;                         \
    } while (0)

    PATCH(schedGetDeviceList, schedGetDeviceList_ptr);
    PATCH(schedGetInternalReference, schedGetInternalReference_ptr);
    PATCH(schedCreateSchedHandle, schedCreateSchedHandle_ptr);
    PATCH(schedRegisterHandle, schedRegisterHandle_ptr);
    PATCH(schedWaitEvent, schedWaitEvent_ptr);
    PATCH(schedPollEvent, schedPollEvent_ptr);
    PATCH(schedAckEvent, schedAckEvent_ptr);
    PATCH(schedPrintf, schedPrintf_ptr);
    PATCH(schedPrintfe, schedPrintfe_ptr);
    PATCH(schedStartComputation, schedStartComputation_ptr);
    PATCH(schedCreateDevice, schedCreateDevice_ptr);
    PATCH(schedDestroyDevice, schedDestroyDevice_ptr);
    PATCH(schedDestroyGroup, schedDestroyGroup_ptr);
    PATCH(schedDestroyJob, schedDestroyJob_ptr);
    PATCH(schedSpawnPes, schedSpawnPes_ptr);
    PATCH(schedKillPes, schedKillPes_ptr);
    PATCH(schedKillGroup, schedKillGroup_ptr);
    PATCH(schedRespawnPes, schedRespawnPes_ptr);

    return 0;

#undef PATCH
}

void * SchedScheduler::Execute() {


    fnt_sched_main   sched_main;
    void           * lib_handle;
    int              ret;

    ret = load_scheduling_library(mLibName, &lib_handle, &sched_main);
    if (ret != 0) {
        TRACE_ERR("Error loading scheduling library %d", errno);
        exit(-1);
        return NULL;
    }
    ret = patch_schedlib_dynamic_symbols(lib_handle);
    if (ret != 0) {
        TRACE_ERR("Error patching symbols for scheduling library");
        exit(-1);
        return NULL;
    }
    sched_main();

    return NULL;
}

