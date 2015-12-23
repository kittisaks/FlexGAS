#include "thread.h"

Thread::Thread() {
    pthread_attr_init(&mAttr);
    pthread_attr_setdetachstate(&mAttr, PTHREAD_CREATE_JOINABLE);
}

Thread::~Thread() {
}

int Thread::Start() {
    pthread_create(&mThread, &mAttr, Thread::Entry, (void *) this);

    return 0;
}

int Thread::Wait() {
    void * ret;
    pthread_join(mThread, &ret);

    return 0;
}

void * Thread::Entry(void * arg) {
    Thread * thread = reinterpret_cast<Thread *>(arg);
    return thread->Execute();
}

