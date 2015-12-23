#include "ivm.h"
#include "ivm_internal.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int ivmMalloc(void ** ptr, size_t size, const char * obj_id, int flags) {

    if ((ptr == NULL) || (obj_id == NULL))
        return -1;

    CHECK_Z(    Init_api());

    saa_req req;
    create_request(saa_mem_alloc, &req);
    snprintf(req.data.mem_id, MED_STR_LEN, "%s", obj_id);
    req.data.mem_flags = flags;
    req.data.mem_size  = size;

    //Request for memory allocation and receive a memory-region.
    CHECK_Z(    submit_request(&req));
    if (req.status != success)
        return -1;

    //Gain access to the allocated space with the received memory-region.
    void   * ptr_tmp;
    size_t   length;
    int ret = chMapMemory(channel, &req.data.mr, &ptr_tmp, &length);
    if (ret != 0)
        return -1;

    register_mem_et(ptr_tmp, obj_id, length, flags, req.data.mr);

    *ptr = ptr_tmp;

    return 0;
}

int ivmFree(const char * obj_id) {

    return 0;
}

int ivmMap(void ** ptr, const char * obj_id, int flags) {

    if ((ptr == NULL) || (obj_id == NULL))
        return -1;

    CHECK_Z(    Init_api());

    saa_req req;
    create_request(saa_mem_map, &req);
    snprintf(req.data.mem_id, MED_STR_LEN, "%s", obj_id);
    req.data.mem_flags = flags;

    //Request for memory mapping and receive a memory-region.
    CHECK_Z(    submit_request(&req));
    if (req.status != success)
        return -1;

    //Gain access to the allocated space with the received memory-region.
    void   * ptr_tmp;
    size_t   length;
    int ret = chMapMemory(channel, &req.data.mr, &ptr_tmp, &length);
    if (ret != 0)
        return -1;

    *ptr = ptr_tmp;

    register_mem_et(ptr_tmp, obj_id, length, flags, req.data.mr);

    return 0;
}

int ivmMapReg(void ** ptr, size_t offset, size_t size,
    const char * obj_id, const char * tag, int flags) {

    return 0;
}

int ivmUnMap(const char * obj_id) {

    return 0;
}

int ivmUnMapReg(const char * obj_id, const char * tag) {

    return 0;
}

int ivmSyncPut(void * ptr, size_t length) {

    CHECK_Z(    Init_api());

    api_mem_et entry = find_mem_et(ptr);
    if (entry == NULL)
        return -1;
    if (length > entry->size)
        return -1;

    ch_mr    r_mr = &entry->mr;
    uint64_t l_base_addr_u64 = (uint64_t) entry->addr;
    uint64_t l_end_addr_u64  = l_base_addr_u64 + entry->size;
    uint64_t l_addr_u64      = (uint64_t) ptr;
    if ((l_addr_u64 < l_base_addr_u64) || (l_addr_u64 > l_end_addr_u64))
        return -1;

    uint64_t offset = l_addr_u64 - l_base_addr_u64;
    if (offset + length > r_mr->length)
        return -1;

    saa_req req;
    create_request(saa_mem_put, &req);
    snprintf(req.data.mem_id, MED_STR_LEN, "%s", entry->mem_id);
    req.data.mem_addr    = NULL;    //We do not need addr for memput request.
    req.data.mem_offset  = offset;
    req.data.mem_size    = length;
    req.data.mem_flags   = entry->mem_flags;

    //Request for memory put.
    CHECK_Z(    submit_request(&req));
    if (req.status != success)
        return -1;

    return 0;
}

int ivmSyncGet(void *  ptr, size_t length) {

    CHECK_Z(    Init_api());

    api_mem_et entry = find_mem_et(ptr);
    if (entry == NULL)
        return -1;
    if (length > entry->size)
        return -1;

    ch_mr    r_mr = &entry->mr;
    uint64_t l_base_addr_u64 = (uint64_t) entry->addr;
    uint64_t l_end_addr_u64  = l_base_addr_u64 + entry->size;
    uint64_t l_addr_u64      = (uint64_t) ptr;
    if ((l_addr_u64 < l_base_addr_u64) || (l_addr_u64 > l_end_addr_u64))
        return -1;

    uint64_t offset = l_addr_u64 - l_base_addr_u64;
    if (offset + length > r_mr->length)
        return -1;

    saa_req req;
    create_request(saa_mem_get, &req);
    snprintf(req.data.mem_id, MED_STR_LEN, "%s", entry->mem_id);
    req.data.mem_addr   = NULL;   //We do not need addr for memget request.
    req.data.mem_offset = offset;
    req.data.mem_size   = length;
    req.data.mem_flags  = entry->mem_flags;

    //Request for memory get.
    CHECK_Z(    submit_request(&req));
    if (req.status != success)
        return -1;

    return 0;
}

int ivmUpdate(void * ptr, size_t length) {

    CHECK_Z(    Init_api());

    api_mem_et entry = find_mem_et(ptr);
    if (entry == NULL)
        return -1;
    if (length > entry->size)
        return -1;

    ch_mr    r_mr = &entry->mr;
    uint64_t l_base_addr_u64 = (uint64_t) entry->addr;
    uint64_t l_end_addr_u64  = l_base_addr_u64 + entry->size;
    uint64_t l_addr_u64      = (uint64_t) ptr;
    if ((l_addr_u64 < l_base_addr_u64) || (l_addr_u64 > l_end_addr_u64))
        return -1;

    uint64_t offset = l_addr_u64 - l_base_addr_u64;
    if (offset + length > r_mr->length)
        return -1;

    saa_req req;
    create_request(saa_mem_update, &req);
    snprintf(req.data.mem_id, MED_STR_LEN, "%s", entry->mem_id);
    req.data.mem_addr   = NULL;   //We do not need addr for memget request.
    req.data.mem_offset = offset;
    req.data.mem_size   = length;
    req.data.mem_flags  = entry->mem_flags;

    //Request for memory get.
    CHECK_Z(    submit_request(&req));
    if (req.status != success)
        return -1;

    return 0;
}

/* //TO-BE-REMOVED (TBR)

int ivmSyncPutGroup(void * ptr, size_t length) {

    return 0;
}

int ivmSyncGetGroup(void * ptr, size_t length) {

    return 0;
}

*/

#ifdef __cplusplus
}
#endif //__cplusplus

