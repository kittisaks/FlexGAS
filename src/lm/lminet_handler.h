#ifndef _LMINET_HANDLER
#define _LMINET_HANDLER

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <map>
#include <vector>

#define MAX_CONNS 65536

class HandlerTimer {
public:
    HandlerTimer();
    ~HandlerTimer();
    int    Start();
    int    Accu();
    int    Stop();
    double GetDuration();

private:
    bool           mIsRunning;
    struct timeval mStartTime;
    struct timeval mEndTime;
    double         mDuration;
};

class InetHandler;

template <typename T> class Queue : public std::vector<T> {
friend class InetHandler;
public:
    Queue();
    ~Queue();

    int PushHead(T inst);
    int PushTail(T inst);
    int PopHead(T * inst);
    int PopTail(T * inst);

    void Lock();
    void Unlock();

private:
    pthread_mutex_t mMtx;

    int _PopHead(T * inst);
};

/**
 * We expect to have only one request per file descriptor. Concurrent
 * asynchronous transfers on a file descriptor is not supported.
 */
typedef enum {
    Free   = 0x10,
    Header,
    Body,
    Auto
} RequestState;

typedef struct {
    inet_request * req;
    inet_request   preq;
    int            fd;
    RequestState   state;
    size_t         rbytes;
} RequestRec;

/**
 * lreq includes: Send/Recv/RemRd/RemWr (requests from self)
 * rreq includes: Send/RemRd/Remwr (requests from peer)
 *
 * lreq is received through the RequestQueue.
 * rreq is received through epoll event (EPOLLIN).
 */
typedef struct {
    struct epoll_event  epEv;
    RequestRec          lreq;
    RequestRec          rreq;
} InetConnection;

/**
 * The request-queue needs to be thread-safe since it will
 * be conncurrently by threads and the request-handler.
 *
 * The connection-table does not need to be thread-safe since
 * we expect that the the structure will only be accessed and
 * modified only by the request-handler.
 */
typedef Queue<inet_request *>                  RequestQueue;
typedef Queue<int>                             FdQueue;
typedef std::map<int /*fd*/, InetConnection *> ConnectionTable;

class InetHandler {
public:
    InetHandler();
    ~InetHandler();

    int HandlerStart();
    int HandlerStop();
    int GetRequestQueue(RequestQueue ** reqQueue);
    int GetCompletionQueue(RequestQueue ** compQueue);
    int RegisterFileDescriptor(int fd);
    int SubmitRequest(inet_request * req);
    int PollCompletion(inet_request ** req);
    int WaitCompletion(inet_request ** req);

private:
    static void * HandlerEntry(void * arg);

    void _Notify_ReqArrived();
    void _Wait_ReqArrived();
    int  _RetrieveFdFromQueue();
    void _RetrieveReqFromQueue();

    int _HandleRequest_Send(InetConnection * conn);
    int _HandleRequest_Recv(InetConnection * conn);
    int _HandleRequest_RemRd(InetConnection * conn);
    int _HandleRequest_RemWr(InetConnection * conn);
    int _HandleRequests();

    int  _HandleConnection_In(
        struct epoll_event * event, InetConnection * conn);
    int  _HandleConnection_Out(
        struct epoll_event * event, InetConnection * conn);
    int  _HandleConnections();
    int HandlerRun();

    //Overall status
    bool                mIsRunning;

    //Handler thread
    pthread_attr_t      mThreadAttr;
    pthread_t           mHandlerThread;

    //Request arrival notification
#if 0
    typedef enum {
        Cleared = 0, Raised = 1
    } FlagStatus;
    FlagStatus mNotif;
    FlagStatus mAck;
#else
    uint32_t            mNotif;
    uint32_t            mAck;
#endif
    bool                mIdle;
    pthread_mutex_t     mNotifyMutex;
    pthread_mutex_t     mWaitMutex;
    pthread_cond_t      mNotifyCond;

    //epoll
    int                 mEpFd;
    struct epoll_event  mpEpEv[MAX_CONNS];
    
    //Data structures
    RequestQueue        mReqQueue;
    RequestQueue        mCompQueue;
    FdQueue             mFdQueue;
    ConnectionTable     mConnTab;
    HandlerTimer        mSleepTimer;

};

template <typename T> Queue<T>::Queue() {
    pthread_mutex_init(&mMtx, NULL);
    this->clear();
}

template <typename T> Queue<T>::~Queue() {
}

template <typename T> int Queue<T>::PushHead(T inst) {

    typename Queue<T>::iterator it;

    pthread_mutex_lock(&mMtx);
    it = this->begin();
    this->insert(it, inst);
    pthread_mutex_unlock(&mMtx);

    return 0;
}

template <typename T> int Queue<T>::PushTail(T inst) {
    pthread_mutex_lock(&mMtx);
    this->push_back(inst);
    pthread_mutex_unlock(&mMtx);

    return 0;
}

template <typename T> int Queue<T>::PopHead(T * inst) {

    typename Queue<T>::iterator it;

    pthread_mutex_lock(&mMtx);
    if (this->size() <= 0) {
        pthread_mutex_unlock(&mMtx);
        return -1;
    }

    *inst = this->at(0);
    it = this->begin();
    this->erase(it);
    pthread_mutex_unlock(&mMtx);

    return 0;
}

template <typename T> int Queue<T>::_PopHead(T * inst) {

    typename Queue<T>::iterator it;

    if (this->size() <= 0)
        return -1;

    *inst = this->at(0);
    it = this->begin();
    this->erase(it);

    return 0;
}

template <typename T> int Queue<T>::PopTail(T * inst) {

    pthread_mutex_lock(&mMtx);
    if (this->size() <= 0) {
        pthread_mutex_unlock(&mMtx);
        return -1;
    }

    *inst = this->pop_back();
    pthread_mutex_unlock(&mMtx);
}

template <typename T> void Queue<T>::Lock() {
    pthread_mutex_lock(&mMtx);
}

template <typename T> void Queue<T>::Unlock() {
    pthread_mutex_unlock(&mMtx);
}


#endif //_LMINET_HANDLER

