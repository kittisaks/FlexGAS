#ifndef _SCHEDULER_TRACE_H
#define _SCHEDULER_TRACE_H

#include "trace.h"
#include "sched_settings.h"

class SchedTrace : public Trace {
public:
    SchedTrace();
    ~SchedTrace();
    void SchedPrintf(trace_type type, const char * str);

private:
};

extern SchedTrace scht;

#ifdef _ENABLE_TRACE
#define TRACE(format, ...)                                    \
    if ((scheds_get_trace_all()) && (scheds_get_trace()))     \
        scht.Print(Trace::notif_msg, format, ##__VA_ARGS__)
#define TRACE_ERR(format, ...) \
    if ((scheds_get_trace_all()) && (scheds_get_trace_err())) \
        scht.Print(Trace::error_msg, format, ##__VA_ARGS__)

#define TRACE_STR(str)                                        \
    if ((scheds_get_trace_all()) && (scheds_get_trace()))     \
        scht.SchedPrintf(Trace::notif_msg, str)
#define TRACE_STR_ERR(str)                                    \
    if ((scheds_get_trace_all()) && (scheds_get_trace_err())) \
        scht.SchedPrintf(Trace::error_msg, str)
#else
#define TRACE(format, ...)
#define TRACE_ERR(format, ...)
#define TRACE_STR(str)
#define TRACE_STR_ERR(str)
#endif


#endif //_AGENT_TRACE_H

