#include "ivm.h"
#include "ivm_internal.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int ivmGetMyJobId() {

    return jid;
}

int ivmGetMyGroupId() {

    return gid;
}

int ivmGetMyDeviceId() {

    return did;
}

int ivmGetMyPeId() {

    return peid;
}

int ivmGetMyRecursionLevel() {

    return rec_level;
}

int ivmGetMaxGroupPeId() {

    return 0;
}

ivm_device_type ivmGetMyDeviceType() {

    return ivm_any;
}

#ifdef __cplusplus
}
#endif //__cplusplus

