#ifndef _SCHED_LIB_H
#define _SCHED_LIB_H

#ifdef _SCHED_MAIN
#define SCHEDLIB_API
#else //_SCHED_MAIN
#define SCHEDLIB_API extern
#endif //SCHED_MAIN

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

SCHEDLIB_API void * schedGetDeviceList_ptr;
SCHEDLIB_API void * schedGetInternalReference_ptr;
SCHEDLIB_API void * schedCreateSchedHandle_ptr;
SCHEDLIB_API void * schedRegisterHandle_ptr;
SCHEDLIB_API void * schedWaitEvent_ptr;
SCHEDLIB_API void * schedPollEvent_ptr;
SCHEDLIB_API void * schedAckEvent_ptr;
SCHEDLIB_API void * schedPrintf_ptr;
SCHEDLIB_API void * schedPrintfe_ptr;
SCHEDLIB_API void * schedStartComputation_ptr;
SCHEDLIB_API void * schedCreateDevice_ptr;
SCHEDLIB_API void * schedDestroyDevice_ptr;
SCHEDLIB_API void * schedDestroyGroup_ptr;
SCHEDLIB_API void * schedDestroyJob_ptr;
SCHEDLIB_API void * schedSpawnPes_ptr;
SCHEDLIB_API void * schedKillPes_ptr;
SCHEDLIB_API void * schedKillGroup_ptr;
SCHEDLIB_API void * schedRespawnPes_ptr;

static inline int schedGetDeviceList() {
    return ((fnt_schedGetDeviceList)
        schedGetDeviceList_ptr)();
}

static inline int schedGetInternalReference(intrn_ref ref_type, void ** ref) {
    return ((fnt_schedGetInternalReference)
        schedGetInternalReference_ptr) (ref_type, ref);
}

static inline int schedCreateSchedHandle(void ** handle) {
    return ((fnt_schedCreateSchedHandle)
        schedCreateSchedHandle_ptr) (handle);
}

static inline int schedRegisterHandle(void * handle, sched_instance inst,
    job_id jid, group_id gid, sched_event_mask mask) {
    return  ((fnt_schedRegisterHandle)
        schedRegisterHandle_ptr) (handle, inst, jid, gid, mask);
}

static inline int schedWaitEvent(void * handle, sched_event * event) {
    return ((fnt_schedWaitEvent)
        schedWaitEvent_ptr) (handle, event);
}

static inline int schedPollEvent(void * handle, sched_event * event) {
    return ((fnt_schedPollEvent)
        schedPollEvent_ptr) (handle, event);
}

static inline int schedAckEvent(sched_event event) {
    return ((fnt_schedAckEvent)
        schedAckEvent_ptr) (event);
}

#define schedPrintf(format, ...)             \
    ((fnt_schedPrintf)                       \
    schedPrintf_ptr) (format, ##__VA_ARGS__)

#define schedPrintfe(format, ...)             \
    ((fnt_schedPrintfe)                       \
    schedPrintfe_ptr) (format, ##__VA_ARGS__)

static inline int schedStartComputation() {
    return ((fnt_schedStartComputation)
        schedStartComputation_ptr) ();
}

static inline int schedCreateDevice(const char * host, sched_link link,
    ivm_device_type dev_type, int phy_dev_id,
    sched_group group, sched_device * device) {
    return ((fnt_schedCreateDevice)
        schedCreateDevice_ptr) (host, link, dev_type, phy_dev_id,
        group, device);
}

static inline int schedDestroyDevice(sched_device device) {
    return ((fnt_schedDestroyDevice)
        schedDestroyDevice_ptr) (device);
}

static inline int schedDestroyGroup(sched_group group) {
    return ((fnt_schedDestroyGroup)
        schedDestroyGroup_ptr) (group);
}

static inline int schedDestroyJob(sched_job job) {
    return ((fnt_schedDestroyJob)
        schedDestroyJob_ptr) (job);
}

static inline int schedSpawnPes(
    sched_device device, sched_comp comp, int num_pes, sched_pe * pe) {
    return ((fnt_schedSpawnPes)
        schedSpawnPes_ptr) (device, comp, num_pes, pe);
}

static inline int schedKillPes(sched_pe pe) {
    return ((fnt_schedKillPes)
        schedKillPes_ptr) (pe);
}

static inline int schedKillGroup() {
    return ((fnt_schedKillGroup)
        schedKillGroup_ptr) ();
}

static inline int schedRespawnPes(sched_device device, sched_pe pe) {
    return ((fnt_schedRespawnPes)
        schedRespawnPes_ptr) (device, pe);
}

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SCHED_LIB_H

