#include "ivm.h"
#include "ivm_internal.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int ivmRegisterComp(ivm_comp comp, const char * comp_id) {

    CHECK_Z(    Init_api());

    saa_req req;
    create_request(saa_register_comp, &req);
    sprintf(req.data.comp_id, "%s", comp_id);
    req.data.comp_fn_ptr = comp;

    CHECK_Z(    submit_request(&req));

    if (req.status != success)
        return -1;

    return 0;
}

int ivmConfigureComp(const char * comp_id, ivm_comp_config config) {

    CHECK_Z(    Init_api());

    saa_req req;
    create_request(saa_configure_comp, &req);
    sprintf(req.data.comp_id, "%s", comp_id);
    req.data.comp_config = config;

    CHECK_Z(    submit_request(&req));

    if (req.status != success)
        return -1;

    return 0;
}

int ivmLaunchComp(const char * comp_id, ivm_comp_config config) {

    CHECK_Z(    Init_api());

    saa_req req;
    create_request(saa_launch_comp, &req);
    sprintf(req.data.comp_id, "%s", comp_id);
    req.data.comp_config = config;

    CHECK_Z(    submit_request(&req));

    if (req.status != success)
        return -1;

    return 0;
}

//Low-priority
int ivmLaunchCompAsync(const char * comp_id, ivm_comp_handle comp_handle) {

    return 0;
}

//Low-priority
int ivmWaitComp(ivm_comp_handle comp_handle) {

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

