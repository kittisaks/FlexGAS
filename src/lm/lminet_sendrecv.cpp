#include "lminet.h"
#include "lminet_connection.h"
#include <iostream>

using namespace std;

int lmInetRegisterMemory(inet_context * context, void * addr,
    size_t length, inet_mr ** mr) {

    if (!isInitialized)
        return -1;

    inet_mr * mr_tmp = new inet_mr();

    //TODO: Please work on filling this
    /*mr_tmp->host*/
    mr_tmp->context = context;
    mr_tmp->addr    = addr;
    mr_tmp->length  = length;

    *mr = mr_tmp;

    return 0;
}

int lmInetSend(inet_context * context, uint64_t reqId,
    void * addr, size_t length) {

    if (!isInitialized)
        return -1;

    int rc = lmInetSendAsync(context, reqId, addr, length);
    if (rc != 0)
        return -1;

    inet_request * comp;
    rc = lmInetWaitComp(reqId, &comp);
    if (rc != 0)
        return -1;
    if (comp->type != Send)
        return -1;
    if (comp->status != Success)
        return -1;

    rc = lmInetReleaseComp(comp);
    if (rc != 0)
        return -1;

    return 0;
}

int lmInetRecv(inet_context * context, uint64_t reqId,
    void * addr, size_t length) {

    if (!isInitialized)
        return -1;

    int rc = lmInetRecvAsync(context, reqId, addr, length);
    if (rc != 0)
        return -1;

    inet_request * comp;
    rc = lmInetWaitComp(reqId, &comp);
    if (rc != 0)
        return -1;
    if (comp->type != Recv)
        return -1;
    if (comp->status != Success)
        return -1;

    rc = lmInetReleaseComp(comp);
    if (rc != 0)
        return -1;

    return 0;
}

int lmInetRemoteWrite(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length) {

    if (!isInitialized)
        return -1;

    int rc = lmInetRemoteWriteAsync(context, reqId, addr, rem_addr, length);
    if (rc != 0)
        return -1;

    inet_request * comp;
    rc = lmInetWaitComp(reqId, &comp);
    if (rc != 0)
        return -1;
    if (comp->type != RemWr)
        return -1;
    if (comp->status != Success)
        return -1;

    rc = lmInetReleaseComp(comp);
    if (rc != 0)
        return -1;

    return 0;
}

int lmInetRemoteRead(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length) {

    if (!isInitialized)
        return -1;

    int rc = lmInetRemoteReadAsync(context , reqId, addr, rem_addr, length);
    if (rc != 0)
        return -1;

    inet_request * comp;
    rc = lmInetWaitComp(reqId, &comp);
    if (rc != 0)
        return -1;
    if (comp->type != RemRd)
        return -1;
    if (comp->status != Success)
        return -1;

    rc = lmInetReleaseComp(comp);
    if (rc != 0)
        return -1;

    return 0;
}

int lmInetSendAsync(inet_context * context, uint64_t reqId,
    void * addr, size_t length) {

    if (!isInitialized)
        return -1;

    if (context == NULL)
        return -1;

    inet_request * req = new inet_request();
    req->reqId    = reqId;
    req->context  = context;
    req->type     = Send;
    req->status   = Pending;
    req->addr     = addr;
    req->rem_addr = NULL;
    req->length   = length;
    handler->SubmitRequest(req);

    return 0;
}

int lmInetRecvAsync(inet_context * context, uint64_t reqId,
    void * addr, size_t length) {

    if (!isInitialized)
        return -1;

    if (context == NULL)
        return -1;

    inet_request * req = new inet_request();
    req->reqId    = reqId;
    req->context  = context;
    req->type     = Recv;
    req->status   = Pending;
    req->addr     = addr;
    req->rem_addr = NULL;
    req->length   = length;
    handler->SubmitRequest(req);

    return 0;
}

int lmInetRemoteWriteAsync(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length) {

    if (!isInitialized)
        return -1;

    if (context == NULL)
        return -1;

    inet_request * req = new inet_request();
    req->reqId    = reqId;
    req->context  = context;
    req->type     = RemWr;
    req->status   = Pending;
    req->addr     = addr;
    req->rem_addr = rem_addr;
    req->length   = length;
    handler->SubmitRequest(req);

    return 0;
}

int lmInetRemoteReadAsync(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length) {

    if (!isInitialized)
        return -1;

    if (context == NULL)
        return -1;

    inet_request * req = new inet_request();
    req->reqId    = reqId;
    req->context  = context;
    req->type     = RemRd;
    req->status   = Pending;
    req->addr     = addr;
    req->rem_addr = rem_addr;
    req->length   = length;
    handler->SubmitRequest(req);

    return 0;
}

int lmInetGetComp(uint64_t reqId, inet_request ** comp) {

    if (!isInitialized)
        return -1;
    
    inet_request * comp_temp;
    int rc = compQueue->PopHead(&comp_temp);
    if (rc == 0) {
        if (comp_temp->reqId == reqId)
            *comp = comp_temp;
        else {
            compQueue->PushTail(comp_temp);
            return -1;
        }
    }

    return 0;
}

int lmInetWaitComp(uint64_t reqId, inet_request ** comp) 
    __attribute__((optimize(0)));

int lmInetWaitComp(uint64_t reqId, inet_request ** comp) {

    if (!isInitialized)
        return -1;

    int rc;
    inet_request * comp_temp;
    //Attention: This may not be scalable for large number of 
    //           completion notifications.
    do {
        rc = compQueue->PopHead(&comp_temp);
        if (rc == 0) {
            if (comp_temp->reqId == reqId)
                break;
            else
                compQueue->PushTail(comp_temp);
        }
    } while(1);

    *comp = comp_temp;

    return 0;
}

int lmInetReleaseComp(inet_request * comp) {

    if (!isInitialized)
        return -1;

    delete comp;

    return 0;
}

