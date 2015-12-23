#include "lminet.h"
#include "lminet_handler.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

//#define _DEBUG

#ifdef _DEBUG
#define DEBUG_MSG(str, ...) printf("\t"); printf(str, ##__VA_ARGS__)
#else
#define DEBUG_MSG(str, ...)
#endif

using namespace std;

HandlerTimer::HandlerTimer() {
    mIsRunning = false;
}

HandlerTimer::~HandlerTimer() {
}

int HandlerTimer::Start() {
    if (mIsRunning)
        return -1;

    gettimeofday(&mStartTime, NULL);
    mDuration = 0.0;
    mIsRunning = true;

    return 0;
}

int HandlerTimer::Accu() {

    if (!mIsRunning)
        return -1;

    gettimeofday(&mEndTime, NULL);
    double start_time = ((double) mStartTime.tv_sec * 1000000.0) +
        (double) mStartTime.tv_usec;
    double end_time   = ((double) mEndTime.tv_sec * 1000000.0) +
        (double) mEndTime.tv_usec;
    mDuration += end_time - start_time;

    gettimeofday(&mStartTime, NULL);

    return 0;
}

int HandlerTimer::Stop() {

    if (!mIsRunning)
        return -1;

    gettimeofday(&mEndTime, NULL);

    double start_time = ((double) mStartTime.tv_sec * 1000000.0) +
        (double) mStartTime.tv_usec;
    double end_time   = ((double) mEndTime.tv_sec * 1000000.0) +
        (double) mEndTime.tv_usec;
    mDuration = end_time - start_time;
    mIsRunning = false;

    return 0;
}

double HandlerTimer::GetDuration() {
    return mDuration;
}



/******************************************************************************
 * Public
 *****************************************************************************/

InetHandler::InetHandler() {

    pthread_mutex_init(&mNotifyMutex, NULL);
    pthread_mutex_init(&mWaitMutex, NULL);
    pthread_cond_init(&mNotifyCond, NULL);
    mNotif = 0;
    mAck   = 0;

    mEpFd = epoll_create1(EPOLL_CLOEXEC);
    if (mEpFd == -1)
        throw "Error: InetHandler cannot initialize epoll.";

    mReqQueue.clear();
    mCompQueue.clear();
    mFdQueue.clear();
    mConnTab.clear();

    mIsRunning = false;
    mIdle      = true;
}

InetHandler::~InetHandler() {
}

int InetHandler::HandlerStart() {

    pthread_attr_init(&mThreadAttr);
    pthread_attr_setdetachstate(&mThreadAttr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mHandlerThread, &mThreadAttr, HandlerEntry, this);

    while (!mIsRunning) usleep(100);

    return 0;
}

int InetHandler::HandlerStop() {

    return 0;
}

int InetHandler::GetRequestQueue(RequestQueue ** reqQueue) {

    *reqQueue = &mReqQueue;

    return 0;
}

int InetHandler::GetCompletionQueue(RequestQueue ** compQueue) {

    *compQueue = &mCompQueue;

    return 0;
}

int InetHandler::RegisterFileDescriptor(int fd) {

    if (fd <= 2)
        return -1;

    mFdQueue.PushTail(fd);
    _Notify_ReqArrived();

    return 0;
}

int InetHandler::SubmitRequest(inet_request * req) {

    if (req == NULL)
        return -1;

    mReqQueue.PushTail(req);
    _Notify_ReqArrived();

    return 0;
}

int InetHandler::PollCompletion(inet_request ** req) {

    return 0;
}

int InetHandler::WaitCompletion(inet_request ** req) {

    return 0;
}

/******************************************************************************
 * Private
 *****************************************************************************/

void * InetHandler::HandlerEntry(void * arg) {

    InetHandler * handler = reinterpret_cast<InetHandler *>(arg);
    handler->HandlerRun();

    return 0;
}

void InetHandler::_Notify_ReqArrived() {

    pthread_mutex_lock(&mNotifyMutex);

    //Requester wakes up the handler.
    mNotif = 1;
    int retryCnt = 0;
    while(mAck != 1) {
        pthread_cond_signal(&mNotifyCond);
        usleep(10);

        if (retryCnt++ > 20) {
            //Assuming that the handler is busy with handling requests
            //and is not able to response to this submission. The requester
            //then simply returns.
            pthread_mutex_unlock(&mNotifyMutex);
            return;
        }
    }
    mNotif = 0;
    while (mAck != 0) usleep(10);

    pthread_mutex_unlock(&mNotifyMutex);
}

void InetHandler::_Wait_ReqArrived() {

    //Handler replies the requester.
    pthread_mutex_lock(&mWaitMutex);
    pthread_cond_wait(&mNotifyCond, &mWaitMutex);

    mAck = 1;
    while(mNotif == 1) usleep(10);
    mAck = 0;

    pthread_mutex_unlock(&mWaitMutex);
}

int InetHandler::_RetrieveFdFromQueue() {

    if (mFdQueue.size() <= 0)
        return 0;

    mFdQueue.Lock();
    while (mFdQueue.size() > 0) {
        int fd = -1;
        mFdQueue._PopHead(&fd);

        //Register the file descriptor with connection table and epoll
        InetConnection * conn = new InetConnection();
        conn->epEv.events  = EPOLLIN | EPOLLRDHUP;
        conn->epEv.data.fd = fd;

        memset(&conn->lreq.preq, 0, sizeof(inet_request));
        conn->lreq.req    = NULL;
        conn->lreq.fd     = fd;
        conn->lreq.state  = Free;
        conn->lreq.rbytes = 0;

        memset(&conn->rreq.preq, 0, sizeof(inet_request));
        conn->rreq.req    = NULL;
        conn->rreq.fd     = fd;
        conn->rreq.state  = Free;
        conn->rreq.rbytes = 0;

        mConnTab.insert(make_pair(fd, conn));
        int rc = epoll_ctl(mEpFd, EPOLL_CTL_ADD, fd, &(conn->epEv));
        if (rc == -1) {
            mFdQueue.Unlock();
            return -1;
        }
        DEBUG_MSG("[Handler]: Added Fd: %d\n", fd);
    }
    mFdQueue.Unlock();

    return 0;
}

void InetHandler::_RetrieveReqFromQueue() {

    if (mReqQueue.size() <= 0)
        return;

    while (mReqQueue.size() > 0) {

        DEBUG_MSG("[Handler]: Fetch...");
        inet_request * req = NULL;
        mReqQueue.PopHead(&req);
        if (req == NULL)
            return;

        //Check whether the file descriptor exists
        ConnectionTable::iterator it;
        it = mConnTab.find(req->context->fd);
        if (it != mConnTab.end()) {
            //If there are concurrent requests
            if (it->second->lreq.state != Free) {
                req->status = Fail;
                mCompQueue.PushTail(it->second->lreq.req);
                DEBUG_MSG("Failed-0\n");
                return;
            }
        }
        else {
            //Retry by looking for the file descriptor in the queue
            _RetrieveFdFromQueue();
            it = mConnTab.find(req->context->fd);
            if (it == mConnTab.end()) {
                req->status = Fail;
                mCompQueue.PushTail(req);
                DEBUG_MSG("Failed-1 %p\n", req->context);
                return;
            }
        }

        //Insert the request into the connection table
        it->second->lreq.req    = req;
        it->second->lreq.state  = Header;
        it->second->lreq.rbytes = 0;
        DEBUG_MSG("Success\n");
    }
}

/******************************************************************************
 *                             _HandleRequests()
 *****************************************************************************/

int InetHandler::_HandleRequest_Send(InetConnection * conn) {

    size_t rbytes = 0;
    switch (conn->lreq.state) {
    case Free:
        //This should not occur.
        break;
    case Header:
        rbytes = send(conn->lreq.fd, 
            ((char *) conn->lreq.req) + conn->lreq.rbytes,
            sizeof(inet_request) - conn->lreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00)
            conn->lreq.rbytes += rbytes;

        if (conn->lreq.rbytes >= sizeof(inet_request)) {
            conn->lreq.state = Body;
            conn->lreq.rbytes = 0;
        }
        break;
    case Body:
        rbytes = send(conn->lreq.fd,
            ((char *) conn->lreq.req->addr) + conn->lreq.rbytes,
            conn->lreq.req->length - conn->lreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00) {
            conn->lreq.rbytes += rbytes;
            DEBUG_MSG("SEND BODY: %ld / %ld\n", conn->lreq.rbytes,
                 conn->lreq.req->length);
        }

        if (conn->lreq.rbytes >= conn->lreq.req->length) {
            //Completion Queue: push the send request back to the CQ.
            conn->lreq.req->status = Success;
            mCompQueue.PushTail(conn->lreq.req);
            conn->lreq.state  = Free;
            conn->lreq.req    = NULL;
            conn->lreq.rbytes = 0;
        }
        break;
    case Auto:
        //This should not occur.
        break;
    }

    return 0;
}

int InetHandler::_HandleRequest_Recv(InetConnection * conn) {

    if (conn->lreq.state == Header)
        conn->lreq.state = Body;
    return 0;
}

int InetHandler::_HandleRequest_RemRd(InetConnection * conn) {

    size_t rbytes = 0;
    switch(conn->lreq.state) {
    case Free:
        //This should not occur.
        break;
    case Header:
        rbytes = send(conn->lreq.fd,
            ((char *) conn->lreq.req) + conn->lreq.rbytes,
            sizeof(inet_request) - conn->lreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00)
            conn->lreq.rbytes += rbytes;

        if (conn->lreq.rbytes >= sizeof(inet_request)) {
            //Temporarily prevent epoll to handle
            //EPOLLIN event on the file descriptor.
            conn->epEv.events &= ~EPOLLIN;
            epoll_ctl(mEpFd, EPOLL_CTL_MOD,
                conn->lreq.fd, &(conn->epEv));
            conn->lreq.state = Auto;
            conn->lreq.rbytes = 0;
        }
        break;
    case Body:
        //This should not occur.
        break;
    case Auto:

        rbytes = recv(conn->lreq.fd,
            ((char *) conn->lreq.req->addr) + conn->lreq.rbytes,
            conn->lreq.req->length - conn->lreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00) {
            conn->lreq.rbytes += rbytes;
            DEBUG_MSG("REMRD: %ld / %ld\n", conn->lreq.rbytes, rbytes);
        }

        if (conn->lreq.rbytes >= conn->lreq.req->length) {
            //Re-enable epoll to handle EPOLLIN event
            //since the REMRD operation is complete.
            conn->epEv.events |= EPOLLIN;
            epoll_ctl(mEpFd, EPOLL_CTL_MOD,
                conn->lreq.fd, &(conn->epEv));
            //Completion Queue: Push the RemRd request to CQ.
            conn->lreq.req->status = Success;
            mCompQueue.PushTail(conn->lreq.req);
            conn->lreq.req    = NULL;
            conn->lreq.state  = Free;
            conn->lreq.rbytes = 0;
        }
        break;
    }

    return 0;
}

int InetHandler::_HandleRequest_RemWr(InetConnection * conn) {

    size_t rbytes = 0;
    switch(conn->lreq.state) {
    case Free:
        //This should not occur.
        break;
    case Header:

        rbytes = send(conn->lreq.fd,
            ((char *) conn->lreq.req) + conn->lreq.rbytes,
            sizeof(inet_request) - conn->lreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00)
            conn->lreq.rbytes += rbytes;

        if (conn->lreq.rbytes >= sizeof(inet_request)) {
            conn->lreq.state = Body;
            conn->lreq.rbytes = 0;
        }
        break;
    case Body:

        rbytes = send(conn->lreq.fd,
            ((char *) conn->lreq.req->addr) + conn->lreq.rbytes,
            conn->lreq.req->length - conn->lreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00)
            conn->lreq.rbytes += rbytes;

        if (conn->lreq.rbytes >= conn->lreq.req->length) {
            //Completion Queue: Push the RemWr request to CQ.
            conn->lreq.req->status = Success;
            mCompQueue.PushTail(conn->lreq.req);
            conn->lreq.req    = NULL;
            conn->lreq.state  = Free;
            conn->lreq.rbytes = 0;
        }
        break;
    case Auto:
        //This should not occur.
        break;
    }

    return 0;
}

int InetHandler::_HandleRequests() {

    ConnectionTable::iterator it;
    for (it=mConnTab.begin();it!=mConnTab.end();++it) {

        InetConnection * conn = it->second;

        if ((conn->lreq.state == Free) ||
            (conn->lreq.req == NULL))
            continue;

        switch(conn->lreq.req->type) {
        case Send:

            _HandleRequest_Send(conn);
            break;
        case Recv:

            _HandleRequest_Recv(conn);
            break;
        case RemRd:

            _HandleRequest_RemRd(conn);
            break;
        case RemWr:

            _HandleRequest_RemWr(conn);
            break;
        default:

            break;
        }
    }

    return 0;
}

/******************************************************************************
 *                            _HandleConnections()
 *****************************************************************************/

int InetHandler::_HandleConnection_In(
        struct epoll_event * event, InetConnection * conn) {

    size_t rbytes, length;
    switch (conn->rreq.state) {
    case Free:
        rbytes = recv(conn->rreq.fd, 
            ((char *) &conn->rreq.preq) + conn->rreq.rbytes,
            sizeof(inet_request) - conn->rreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00) 
            conn->rreq.rbytes += rbytes;

        if (conn->rreq.rbytes < sizeof(inet_request)) {
            conn->rreq.state = Header;
        }
        else {
            //TODO: We will have to handle RemRd here
            switch (conn->rreq.preq.type) {
            case Recv:
                conn->rreq.state  = Free;
                conn->rreq.rbytes = 0;
                break;
            case Send:
                conn->rreq.state  = Body;
                conn->rreq.rbytes = 0;
                break;
            case RemWr:
                conn->rreq.state  = Body;
                conn->rreq.rbytes = 0;
                break;
            case RemRd:
                conn->rreq.state  = Auto;
                conn->rreq.rbytes = 0;
                break;
            default:
                conn->rreq.state  = Free;
                conn->rreq.rbytes = 0;
                break;
            }
        }
        break;
    case Header:
        rbytes = recv(conn->rreq.fd, 
            ((char *) &conn->rreq.preq) + conn->rreq.rbytes,
            sizeof(inet_request) - conn->rreq.rbytes, 0);
        if (rbytes != (size_t) ~0x00)
            conn->rreq.rbytes += rbytes;

        if (conn->rreq.rbytes < sizeof(inet_request)) {
            conn->rreq.state = Header;
        }
        else {
            switch (conn->rreq.preq.type) {
            case Recv:
                conn->rreq.state  = Free;
                conn->rreq.rbytes = 0;
                break;
            case Send:
                conn->rreq.state  = Body;
                conn->rreq.rbytes = 0;
                break;
            case RemWr:
                conn->rreq.state  = Body;
                conn->rreq.rbytes = 0;
                break;
            case RemRd:
                conn->rreq.state  = Auto;
                conn->rreq.rbytes = 0;
                break;
            default:
                conn->rreq.state  = Free;
                conn->rreq.rbytes = 0;
                break;
            }
        }
        break;
    case Body: //For Send and RemWr
        length = conn->rreq.preq.length;

        switch (conn->rreq.preq.type) {
        case Send:
            //If self does not post read-request then postpone receiving
            //until the read-request is received.

            if (conn->lreq.req == NULL)
                break;

            if ((conn->lreq.req->type != Recv) ||
                (conn->lreq.state != Body))
                break;

            rbytes = recv(conn->rreq.fd, 
                ((char *) conn->lreq.req->addr) + conn->rreq.rbytes,
                length - conn->rreq.rbytes, 0);
            if (rbytes != (size_t) ~0x00) {
                conn->rreq.rbytes += rbytes;
                DEBUG_MSG("RECV: %ld / %ld / %ld\n", 
                    conn->rreq.rbytes, rbytes, conn->rreq.preq.length);
            }

            if (conn->rreq.rbytes >= length) {
                //Completion Queue: Push the RECV request to CQ.
                conn->lreq.req->status = Success;
                mCompQueue.PushTail(conn->lreq.req);
                conn->lreq.req    = NULL;
                conn->lreq.state  = Free;
                conn->lreq.rbytes = 0;
                conn->rreq.state  = Free;
                conn->rreq.rbytes = 0;
            }
            break;

        case RemWr:

            rbytes = recv(conn->rreq.fd,
                ((char *) conn->rreq.preq.rem_addr) + conn->rreq.rbytes,
                length - conn->rreq.rbytes, 0);
            if (rbytes != (size_t) ~0x00) {
                conn->rreq.rbytes += rbytes;
                DEBUG_MSG("REWR: %ld / %ld / %ld\n", 
                    conn->rreq.rbytes, rbytes, conn->rreq.preq.length);
            }

            if (conn->rreq.rbytes >= length) {
                conn->rreq.state = Free;
                conn->rreq.rbytes = 0;
            }
            break;

        default:
            conn->rreq.state = Free;
            conn->rreq.rbytes = 0;
        }
        break;
    case Auto: //For RemRd
        //This should not happen.
        break;
    }

    return 0;
}

int InetHandler::_HandleConnection_Out(
        struct epoll_event * event, InetConnection * conn) {

    size_t rbytes, length = conn->rreq.preq.length;

    rbytes = send(conn->rreq.fd,
        ((char *) conn->rreq.preq.rem_addr) + conn->rreq.rbytes,
        length - conn->rreq.rbytes, 0);

    if (rbytes != (size_t) ~0x00) {        
        conn->rreq.rbytes += rbytes;
        DEBUG_MSG("REMRD: %ld / %ld / %ld\n",
            conn->rreq.rbytes, rbytes, length);
    }

    if (conn->rreq.rbytes >= length) {
        conn->rreq.state = Free;
        conn->rreq.rbytes = 0;
    }

    return 0;
}

int InetHandler::_HandleConnections() {

    ConnectionTable::iterator it;
    for (it=mConnTab.begin();it!=mConnTab.end();++it) {
        InetConnection * conn = it->second;
        if (conn->rreq.state == Auto)
            _HandleConnection_Out(&conn->epEv, conn);
    }

    int rc = epoll_wait(mEpFd, mpEpEv, mConnTab.size(), 1);
    if (rc == -1)
        return -1;
    if (rc == 0)
        return 0;

    for (int idx=0;idx<rc;idx++) {
        if (mpEpEv[idx].events & EPOLLRDHUP) {

            //Unregister the file descriptor with connection table
            ConnectionTable::iterator conn = mConnTab.find(
                mpEpEv[idx].data.fd);
            if (conn == mConnTab.end())
                return -1;
            mConnTab.erase(conn);

            //Unregister the file descriptor with epoll
            int ret = epoll_ctl(
                mEpFd, EPOLL_CTL_DEL, mpEpEv[idx].data.fd, NULL);
            if (ret == -1) {
                return -1;
            }
            DEBUG_MSG("[Handler]: Removed fd: %d\n", mpEpEv[idx].data.fd);
        }
        else if (mpEpEv[idx].events & EPOLLOUT) {

            ConnectionTable::iterator conn = mConnTab.find(
                mpEpEv[idx].data.fd);
            if (conn == mConnTab.end())
                return -1;
            _HandleConnection_In(&mpEpEv[idx], conn->second);
        }
        else if (mpEpEv[idx].events & EPOLLIN) {

            ConnectionTable::iterator conn = mConnTab.find(
                mpEpEv[idx].data.fd);
            if (conn == mConnTab.end())
                return -1;            
            _HandleConnection_In(&mpEpEv[idx], conn->second);
        } 
    }

    return 0;
}

int InetHandler::HandlerRun() {

    mIsRunning = true;
    DEBUG_MSG("[Handler]: Handler Started\n");

    mSleepTimer.Start();

    while(1) {

//        _Wait_ReqArrived();

        _RetrieveFdFromQueue();
        _RetrieveReqFromQueue();

        _HandleRequests();
        _HandleConnections();
        mSleepTimer.Accu();
        usleep(1000);
//        cout.precision(6);
//        cout << fixed;
//        cout << "Duration: " << mSleepTimer.GetDuration() << endl;

    }

    return 0;
}

