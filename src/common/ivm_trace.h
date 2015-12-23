#ifndef _IVM_TRACE_H
#define _IVM_TRACE_H

#ifdef _IVM_ENABLE_TRACE
#define TRACE(format, ...)     \
    ivm_utils::ivm_trace(ivm_utils::notif_msg, format, ##__VA_ARGS__)
#define TRACE_ERR(format, ...) \
    ivm_utils::ivm_trace(ivm_utils::error_msg, format, ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#define TRACE_ERR(format, ...)
#endif

#define IVM_TRACE_ALLOC_FILE "ivm_alloced.log"

#include "ivmd.h"

namespace ivm_utils {

typedef enum {
    notif_msg = 0x00,
    error_msg
} ivm_trace_type;

void ivm_trace_component_name(const char * name);

void ivm_trace(ivm_trace_type type, const char * format, ...);

void ivm_trace_alloc(ivmd::memory_id obj_id, size_t size,
    group_id gid, pe_id peid);

} //ivm_utils

#endif //_IVM_TRACE_H

