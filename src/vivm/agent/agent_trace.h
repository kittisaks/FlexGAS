#ifndef _AGENT_TRACE_H
#define _AGENT_TRACE_H

#include "trace.h"
#include "agent_settings.h"

class AgentTrace : public Trace {
public:
    AgentTrace();
    ~AgentTrace();

private:
};

extern AgentTrace agt;

#ifdef _ENABLE_TRACE
#define TRACE(format, ...)                                 \
    if ((agts_get_trace_all()) && (agts_get_trace()))      \
        agt.Print(Trace::notif_msg, format, ##__VA_ARGS__)
#define TRACE_ERR(format, ...)                             \
    if ((agts_get_trace_all()) && (agts_get_trace_err()))  \
    agt.Print(Trace::error_msg, format, ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#define TRACE_ERR(format, ...)
#endif


#endif //_AGENT_TRACE_H

