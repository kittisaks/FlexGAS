#include "lmshm_internal.h"
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>

using namespace std;

typedef struct {
    uint64_t           reqId;
    shm_request_type   type;
    shm_request_status status;
    size_t             length;
} Header;

typedef vector<shm_request *> RequestQueue;

RequestQueue    * shm_reqQueue;
RequestQueue    * shm_compQueue;
pthread_mutex_t   shm_reqQueueMtx;
pthread_mutex_t   shm_compQueueMtx;

inline void RemoveReqQueueEntry(shm_request * req) {
    pthread_mutex_lock(&shm_reqQueueMtx);
    RequestQueue::iterator iitt;
    for (iitt=shm_reqQueue->begin();iitt!=shm_reqQueue->end();++iitt) {
        if (req->reqId == (*iitt)->reqId)
            break;
    }
    shm_reqQueue->erase(iitt);
    pthread_mutex_unlock(&shm_reqQueueMtx);
}

int ShmSubmitSend(shm_request * req) {

    Header * hdr = reinterpret_cast<Header *>(req->hdrbuf);
    hdr->reqId  = req->reqId;
    hdr->type   = req->type;
    hdr->status = req->status; 
    hdr->length = req->length;

    //Send header describing body of the message
    req->accBufLength = send(req->context->fd,
        hdr, sizeof(Header), MSG_WAITALL);
    if (req->accBufLength != sizeof(Header))
        return -1;

    //Send body of the message
    req->accLength = send(req->context->fd,
        req->addr, req->length, MSG_WAITALL);
    if (req->accLength != req->length)
        return -1;

    return 0;
}

int ShmSubmitRecv(shm_request * req) {

    int len = 0;
    Header * hdr = reinterpret_cast<Header *>(req->hdrbuf);

    TEST_Z(    ioctl(req->context->fd, FIONREAD, &len));
    if (len > 0) {
        switch (req->status) {
        case ShmRecvWaitHdr:
            //Receive header of the message
            len = recv(req->context->fd,
                &((char *) req->hdrbuf)[req->accBufLength],
                sizeof(Header) - req->accBufLength, MSG_DONTWAIT);

            if (len == -1)
                return -1;
            req->accBufLength += len;
            if (req->accBufLength == sizeof(Header))
                req->status = ShmRecvWaitBdy;
            break;
        case ShmRecvWaitBdy:
            //Receive body of the message
            len = recv(req->context->fd,
                &(((char *) req->addr)[req->accLength]),
                hdr->length - req->accLength, MSG_DONTWAIT);

            if (len == -1)
                return -1;
            req->accLength += len;
            break;
        default:
            return -1;
        }
    }
    else {
        usleep(100);
    }

    return 0;
}

int ShmSubmitRequest(shm_request * req) {

    switch(req->type) {
    case ShmSend:
        req->status = ShmPending;
        req->hdrbuf = reinterpret_cast<void *>(new Header());
        memset(req->hdrbuf, 0, sizeof(Header));
        req->accBufLength = 0;
        TEST_Z(    ShmSubmitSend(req));

        pthread_mutex_lock(&shm_reqQueueMtx);
        shm_reqQueue->push_back(req);
        pthread_mutex_unlock(&shm_reqQueueMtx);

        return 0;
    case ShmRecv:
        req->status = ShmRecvWaitHdr;
        req->hdrbuf = reinterpret_cast<void *>(new Header());
        memset(req->hdrbuf, 0, sizeof(Header));
        req->accBufLength = 0;
        TEST_Z(    ShmSubmitRecv(req));

        pthread_mutex_lock(&shm_reqQueueMtx);
        shm_reqQueue->push_back(req);
        pthread_mutex_unlock(&shm_reqQueueMtx);
      
        return 0;
    default:

        return -1;
    }

    return 0;
}

int lmShmSend(shm_context * context, uint64_t reqId,
    void * addr, size_t length) {

    shm_request * req = new shm_request();
    shm_request * comp;

    req->reqId     = reqId;
    req->context   = context;
    req->type      = ShmSend;
    req->addr      = addr;
    req->length    = length;
    req->accLength = 0;
    TEST_Z(    ShmSubmitRequest(req));

    TEST_Z(    lmShmWaitComp(reqId, &comp));

    return 0;
}

int lmShmRecv(shm_context * context, uint64_t reqId,
    void * addr, size_t length) {

    shm_request * req = new shm_request();
    shm_request * comp;

    req->reqId     = reqId;
    req->context   = context;
    req->type      = ShmRecv;
    req->addr      = addr;
    req->length    = length;
    req->accLength = 0;
    TEST_Z(    ShmSubmitRequest(req));

    TEST_Z(    lmShmWaitComp(reqId, &comp));

    return 0;
}

int lmShmSendAsync(shm_context * context, uint64_t reqId,
    void * addr, size_t length) {

    shm_request * req = new shm_request();

    req->reqId     = reqId;
    req->context   = context;
    req->type      = ShmSend;
    req->addr      = addr;
    req->length    = length;
    req->accLength = 0;
    TEST_Z(    ShmSubmitRequest(req));

    return 0;
}

int lmShmRecvAsync(shm_context * context, uint64_t reqId,
    void * addr, size_t length) {

    shm_request * req = new shm_request();

    req->reqId     = reqId;
    req->context   = context;
    req->type      = ShmRecv;
    req->addr      = addr;
    req->length    = length;
    req->accLength = 0;
    TEST_Z(    ShmSubmitRequest(req));

    return 0;
}

inline int ShmGetComp(shm_request * req,
    RequestQueue::iterator it) {

    int      ret;
    Header * hdr;

    switch(req->type) {
    case ShmSend:
        if (req->accLength == req->length) 
            req->status = ShmSuccess;
        else
            req->status = ShmFail;
        RemoveReqQueueEntry(req);
        return 0;
        break;
    case ShmRecv:
        hdr = reinterpret_cast<Header *> (req->hdrbuf);

        if ((req->accLength == hdr->length) && 
            ((req->accLength > 0) && (hdr->length))) {
            if (hdr->length == req->length)
                req->status = ShmSuccess;
            if (hdr->length != req->length) 
                req->status = ShmLengthError;
            RemoveReqQueueEntry(req);
            return 0;
        }

        ret = ShmSubmitRecv(req);
        if (ret == -1) {
            req->status = ShmFail;
            RemoveReqQueueEntry(req);
            return 0;
        }
        else if (ret == 0) {
            return -1;
        }

        if ((req->accLength == hdr->length) && 
            ((req->accLength > 0) && (hdr->length > 0))) {
            if (hdr->length == req->length)
                req->status = ShmSuccess;
            if (hdr->length != req->length) 
                req->status = ShmLengthError;
            RemoveReqQueueEntry(req);
        }
        return 0;

        break;
    default:
        return -1;
    }

    return 0;
}

int lmShmGetComp(uint64_t reqId, shm_request ** comp) {

    RequestQueue::iterator   it;
    shm_request            * req = NULL;

    pthread_mutex_lock(&shm_reqQueueMtx);
    for (it=shm_reqQueue->begin();it!=shm_reqQueue->end();++it) {
        if (reqId == (*it)->reqId) {
            req = *it;
            pthread_mutex_unlock(&shm_reqQueueMtx);
            break;
        }
    }
    pthread_mutex_unlock(&shm_reqQueueMtx);

    if (req == NULL)
        return -1;

    int ret = ShmGetComp(req, it);
    *comp = req;

    return ret;
}

int lmShmWaitComp(uint64_t reqId, shm_request ** comp) {

    RequestQueue::iterator   it;
    shm_request            * req = NULL;

    pthread_mutex_lock(&shm_reqQueueMtx);
    for (it=shm_reqQueue->begin();it!=shm_reqQueue->end();++it) {
        if (reqId == (*it)->reqId) {
            req = *it;
            pthread_mutex_unlock(&shm_reqQueueMtx);
            break;
        }
    }
    pthread_mutex_unlock(&shm_reqQueueMtx);

    if (req == NULL)
        return -1;

    int ret;
    do {
        ret = ShmGetComp(req, it);
    } while(ret != 0);

    return 0;
}

int lmShmReleaseComp(shm_request * comp) {

    delete comp;

    return 0;
}

