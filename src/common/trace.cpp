#include "trace.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

Trace::Trace() {
    pthread_mutex_init(&mMutex, NULL);
}

Trace::~Trace() {
}

void Trace::SetComponentName(const char * name) {

    snprintf(mComponentName, 32, "%s", name);
}

void Trace::PrintStr(trace_type type, const char * str) {

    time_t t = time(NULL);
    struct tm * loc_tm = localtime(&t);

    switch(type) {
    case notif_msg:
        pthread_mutex_lock(&mMutex);
        fprintf(stdout, "[%s, %d, ", mComponentName, getpid());
        fprintf(stdout, "%02d:%02d:%02d]: N - ",
            loc_tm->tm_hour, loc_tm->tm_min, loc_tm->tm_sec);
        fprintf(stdout, "%s", str);
        fprintf(stdout, "\n");
        pthread_mutex_unlock(&mMutex);
        break;
    case error_msg:
        pthread_mutex_lock(&mMutex);
        fprintf(stderr, "[%s, %d, ", mComponentName, getpid());
        fprintf(stderr, "%02d:%02d:%02d]: E - ",
            loc_tm->tm_hour, loc_tm->tm_min, loc_tm->tm_sec);
        fprintf(stderr, "%s", str);
        fprintf(stderr, "\n");
        pthread_mutex_unlock(&mMutex);
        break;
    default:
        return;
    }
}

void Trace::Print(trace_type type, const char * format, ...) {

    char    str[2048];
    va_list ap;
/*
    if (!get_ivm_trace_all())
        return;

    switch(type) {
    case notif_msg:
        if (!get_ivm_trace())
            return;
        break;
    case error_msg:
        if (!get_ivm_trace_err())
            return;
        break;
    }
*/
    va_start(ap, format);
    vsprintf(str, format, ap);
    va_end(ap);

    time_t t = time(NULL);
    struct tm * loc_tm = localtime(&t);

    switch(type) {
    case notif_msg:
        pthread_mutex_lock(&mMutex);
        fprintf(stdout, "[%s, %d, ", mComponentName, getpid());
        fprintf(stdout, "%02d:%02d:%02d]: N - ",
            loc_tm->tm_hour, loc_tm->tm_min, loc_tm->tm_sec);
        fprintf(stdout, "%s", str);
        fprintf(stdout, "\n");
        pthread_mutex_unlock(&mMutex);
        break;
    case error_msg:
        pthread_mutex_lock(&mMutex);
        fprintf(stderr, "[%s, %d, ", mComponentName, getpid());
        fprintf(stderr, "%02d:%02d:%02d]: E - ",
            loc_tm->tm_hour, loc_tm->tm_min, loc_tm->tm_sec);
        fprintf(stderr, "%s", str);
        fprintf(stderr, "\n");
        pthread_mutex_unlock(&mMutex);
        break;
    default:
        return;
    }
}

