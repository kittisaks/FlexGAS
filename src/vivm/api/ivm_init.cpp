#include "ivm.h"
#include "ivm_internal.h"
#include "api_settings.h"
#include <stdlib.h>

#include <iostream>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

bool             is_api_init   = false;
bool             is_descendant = false;
ch_init_attr     ch_attr;
ch               channel;

pid_t            pid;
uid_t            uid;
char             host[64];
pe_id            peid;
device_id        did;
host_id          hid;
group_id         gid;
job_id           jid;
int              rec_level;
ivm_device_type  device_type;
uint32_t         phy_dev_id;

api_memory_table a_mem_tab;
pthread_mutex_t  a_mem_tab_lock;

int Init_api() {
    if (is_api_init)
        return 0;

    pid = getpid();
    uid = getuid();

    pthread_mutex_init(&a_mem_tab_lock, NULL);

    ch_attr.link = ch_link_shm;

    //Check whether the process is created by a user or an agent
    is_descendant = apis_get_is_descendant();
    if (is_descendant)
        ch_attr.peer_id = apis_get_shm_path_agent();
    else
        ch_attr.peer_id = apis_get_shm_path_sched();

    //Connect to the local Agent/Scheduler.
    int ret = chOpen(&channel, ch_attr, 0);
    if (ret != 0)
        return -1;

    //Check if the connection goes properly.
    ch_event event;
    ret = chWaitChannel(&event, channel);
    if (ret != 0)
        return -1;
    if (event->type != ch_established)
        return -1;

    is_api_init = true;

    return 0;
}

int Finalize_api() {

    return chClose(channel);
}

int ivmEnter(int argc, char ** argv) {

    CHECK_Z(    Init_api());

    saa_req req;
    create_request(saa_register_pe, &req);

    if (!is_descendant) {
        marshal_prog_arg(argc, argv, &req);
        marshal_bin_path(argv, &req);
    }

    CHECK_Z(    submit_request(&req));

    if (req.status != success) 
        return -1;

    jid  = req.jid;
    gid  = req.gid;
    hid  = req.hid;
    did  = req.did;
    peid = req.peid;

    if (is_descendant) {
        rec_level   = req.data.rec_level;
        device_type = req.data.device_type;
        phy_dev_id  = req.data.phy_dev_id;
        req.data.comp_desc_exec(NULL);
        ivmExit();
        exit(0);
    }

    return 0;
}

int ivmExit() {

    CHECK_Z(    Init_api());

    saa_req req;
    create_request(saa_unregister_pe, &req);

    CHECK_Z(    submit_request(&req));

    if (req.status != success)
        return -1;

    int ret = Finalize_api();
    if (ret != 0)
        return -1;

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

