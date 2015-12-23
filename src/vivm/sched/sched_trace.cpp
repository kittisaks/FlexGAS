#include "sched_trace.h"

SchedTrace::SchedTrace() {
}

SchedTrace::~SchedTrace() {
}

void SchedTrace::SchedPrintf(trace_type type, const char * str) {

    Trace::PrintStr(type, str);
}

