#ifndef _IVM_H
#define _IVM_H

#include <stdint.h>
#include <unistd.h>

#define API_ERR_UNSPECIFIED   -1
#define API_ERR_COMMUNICATION -2

#define IVM_MEM_GLOBAL        0x00000010
#define IVM_MEM_LOCAL         0x00000000

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//Defines the data type for the group and PE identification number
typedef uint32_t job_id, group_id, host_id, device_id, pe_id;

//Defines the types of supported compute devices
typedef enum {
    ivm_cpu = 0x01, ivm_gpu = 0x02, ivm_any = 0x03
} ivm_device_type;

/*****************************************************************************
 * Initialization
 ****************************************************************************/
int ivmEnter(int argc, char ** argv);

int ivmExit();

/*****************************************************************************
 * Process Element Management
 ****************************************************************************/
int ivmGetMyJobId();

int ivmGetMyGroupId();

int ivmGetMyDeviceId();

int ivmGetMyPeId();

int ivmGetMyRecursionLevel();

int ivmGetMaxGroupPeId();

ivm_device_type ivmGetMyDeviceType();

/*****************************************************************************
 * Computation Management
 ****************************************************************************/
typedef void * (* ivm_comp) (void * arg);

typedef struct {
    uint32_t        max_pes_num;
    ivm_device_type type;
    uint32_t        min_res;
    uint32_t        req_res;
    uint32_t        max_res;
} ivm_comp_config;

typedef struct {
} ivm_comp_handle_t, * ivm_comp_handle;

/**
 * Note: We assume that all computations are registered by the root PE at
 *       program startup. If we allow registration in side a computation,
 *       there is no way the scheduler will recognized the computation 
 *       registered inside a computation since the registered computation
 *       goes to the internal data structure (comp_tab) belonging to the
 *       registering group. This issue inherit from the centralized scheduling.
 */
int ivmRegisterComp(ivm_comp comp, const char * comp_id);

/**
 * TODO: This will be deprecated. We can supply the configuration at
 *       computation launch.
 */

int ivmConfigureComp(const char * compe_id, ivm_comp_config config);

//Conceptaully, computations can be launched hierarchically similarly to
//Dynamic Parallelism in CUDA. However, we do not allow PE with cid >= 0
//to launch computations.
int ivmLaunchComp(const char * compe_id, ivm_comp_config config);

//Low-priority
int ivmLaunchCompAsync(const char * compe_id, ivm_comp_handle comp_handle);

//Low-priority
int ivmWaitComp(ivm_comp_handle comp_handle);

//Maybe we want to include some thing similar to launching a set of PEs which
//work on a computation and all of the PEs exist/alive for the entire
//computation + They communicate through send/recv primitives.

/*****************************************************************************
 * Memory Management
 ****************************************************************************/
int ivmMalloc(void ** ptr, size_t size, const char * obj_id, int flags);

int ivmFree(const char * obj_id);

int ivmMap(void ** ptr, const char * obj_id, int flags);

int ivmMapReg(void ** ptr, size_t offset, size_t size,
    const char * obj_id, const char * tag, int flags);

int ivmUnMap(const char * obj_id);

int ivmUnMapReg(const char * obj_id, const char * tag);

int ivmSyncPut(void * ptr, size_t length);

int ivmSyncGet(void * ptr, size_t length);

int ivmUpdate(void * ptr, size_t length);

/* //TO-BE-REMOVED (TBR)

int ivmSyncPutGroup(void * ptr, size_t length);

int ivmSyncGetGroup(void * ptr, size_t length);

*/

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_IVM_H

