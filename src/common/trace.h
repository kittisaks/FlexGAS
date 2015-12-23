#ifndef _TRACE_H
#define _TRACE_H
/*
#ifdef _ENABLE_TRACE
#define TRACE(format, ...)     \
    Print(Trace::notif_msg, format, ##__VA_ARGS__)
#define TRACE_ERR(format, ...) \
    Print(Trace::error_msg, format, ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#define TRACE_ERR(format, ...)
#endif
*/

#include <pthread.h>

class Trace {
public:
    typedef enum {
        notif_msg = 0x00,
        error_msg
    } trace_type;

    Trace();
    ~Trace();
    void SetComponentName(const char * name);
    void PrintStr(trace_type type, const char * str);
    void Print(trace_type type, const char * format, ...);

protected:
    char            mComponentName[32];
    pthread_mutex_t mMutex;

};

#endif //_TRACE_H

