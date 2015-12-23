#ifndef _THREAD_H
#define _THREAD_H

#include <pthread.h>

class Thread {
public:
    Thread();
    ~Thread();
    int Start();
    int Wait();
    virtual void * Execute() = 0;

protected:
    static void * Entry(void * arg);
    pthread_t       mThread;
    pthread_attr_t  mAttr;
};

#endif //_THREAD_H

