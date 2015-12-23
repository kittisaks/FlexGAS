#include "channel.h"
#include "channel_internal.h"
#include <iostream>

using namespace std;

extern "C" {

#ifndef _DISABLE_LM_RDMA
int RegisterMemoryLinkRdma(ch channel,
    void * addr, size_t length, ch_mr * mr, int flags) {

    ch_mr mr_tmp;
    if (flags & CH_ADD_LINK_ENA) {
        mr_tmp = *mr;
        if (mr_tmp->type != ch_mr_t::ch_mr_local)
            return -1;
    }
    else
        mr_tmp = new ch_mr_t();

    mr_tmp->type = ch_mr_t::ch_mr_local;
    mr_tmp->channel = channel;
    mr_tmp->link_ena |= CH_RDMA_ENA;

    ibv_mr * lm_mr_tmp;
    int ret = lmRdmaRegisterMemory(channel->ctx.rdma,
        addr, length, &lm_mr_tmp);
    if (ret != 0)
        return -1;

    mr_tmp->addr     = addr;
    mr_tmp->length   = length;
    mr_tmp->handle   = lm_mr_tmp->handle;
    mr_tmp->lkey     = lm_mr_tmp->lkey;
    mr_tmp->rkey     = lm_mr_tmp->rkey;
    mr_tmp->local_mr = lm_mr_tmp;
    *mr = mr_tmp;

    return 0;
}
#endif //_DISABLE_LM_RDMA

int RegisterMemoryLinkInet(ch channel,
    void * addr, size_t length, ch_mr * mr, int flags) {

    ch_mr mr_tmp ;
    if (flags & CH_ADD_LINK_ENA) { 
        mr_tmp = *mr;
        if (mr_tmp->type != ch_mr_t::ch_mr_local)
            return -1;
    }
    else
        mr_tmp = new ch_mr_t();

    mr_tmp->type = ch_mr_t::ch_mr_local;
    mr_tmp->channel = channel;
    mr_tmp->link_ena |= CH_INET_ENA;

    inet_mr * lm_mr_tmp;
    int ret = lmInetRegisterMemory(channel->ctx.inet,
        addr, length, &lm_mr_tmp);
    if (ret != 0)
        return -1;

    mr_tmp->addr = addr;
    mr_tmp->length = length;
    //TODO: Fille mr_tmp->host and mr_tmp->service
    *mr = mr_tmp;

    return 0;
}

int RegisterMemoryLinkShm(ch channel,
    void * addr, size_t length, ch_mr * mr, int flags) {

    ch_mr mr_tmp;
    if (flags & CH_ADD_LINK_ENA) {
        mr_tmp = *mr;
        if (mr_tmp->type != ch_mr_t::ch_mr_local)
            return -1;
    }
    else
        mr_tmp = new ch_mr_t();

    mr_tmp->type = ch_mr_t::ch_mr_local;
    mr_tmp->channel = channel;
    mr_tmp->link_ena |= CH_SHM_ENA;

    void * shm_addr;
    char * name = reinterpret_cast<char *>(addr);
    int    lm_flags = 0;

    shm_mr * lm_mr_tmp;
    lm_flags |= (flags && CH_SHM_FCLEAN) ? LM_SHM_FCLEAN : 0;
    int ret = lmShmAlloc(&lm_mr_tmp, &shm_addr, length,
        name, lm_flags);
    if (ret != 0)
        return -1;

    mr_tmp->addr = shm_addr;
    mr_tmp->length = length;
    strcpy(mr_tmp->name, lm_mr_tmp->name);
    mr_tmp->fd = lm_mr_tmp->fd;
    *mr = mr_tmp;

    return 0;
}

/**
 * @API:
 */

int chRegisterMemory(ch channel, void * addr, size_t length,
    ch_mr * mr, int flags) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return RegisterMemoryLinkRdma(channel, addr, length, mr, flags);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return RegisterMemoryLinkInet(channel, addr, length, mr, flags);
    case ch_link_shm:
        return RegisterMemoryLinkShm(channel, addr, length, mr, flags);
    }

    return 0;
}

int UnregisterMemoryLinkShm(ch channel, ch_mr mr) {

    shm_mr lm_mr;
    strcpy(lm_mr.name, mr->name);
    lm_mr.fd = mr->fd;
    lm_mr.addr = mr->addr;
    lm_mr.length = mr->length;

    int ret = lmShmFree(&lm_mr);
    if (ret != 0)
        return -1;

    return 0;
}

/**
 * @API:
 */
int chUnregisterMemory(ch channel, ch_mr mr) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        //TODO: We will need to implement unregistration at the LM-IB level.
        return -1;
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        //TODO: We will need to implement unregistration at the LM-INET level.
        return -1;
    case ch_link_shm:
        return UnregisterMemoryLinkShm(channel, mr);
    }

    return 0;
}

/**
 * @API:
 */
int chMapMemory(ch channel, ch_mr mr, void ** addr, size_t * length) {

#ifndef _DISABLE_LM_RDMA
    if (channel->link == ch_link_rdma)
        return -1;
#endif //_DISABLE_LM_RDMA

    if (channel->link == ch_link_inet)
        return -1;

    void * addr_tmp; size_t length_tmp; shm_mr lm_mr_tmp;
    strcpy(lm_mr_tmp.name, mr->name);
    lm_mr_tmp.length = mr->length;

    int ret = lmShmAcquire(&lm_mr_tmp, &addr_tmp, &length_tmp);
    if (ret != 0)
        return -1;

    mr->addr = addr_tmp;
    mr->length = length_tmp;
    mr->fd = lm_mr_tmp.fd;

    if (addr != NULL)
        *addr = addr_tmp;

    if (length != NULL)
        *length = length_tmp;

    return 0;
}

/**
 * @API:
 */
int chUnmapMemory(ch channel, ch_mr mr) {
#ifndef _DISABLE_LM_RDMA
    if (channel->link == ch_link_rdma)
        return -1;
#endif //_DISABLE_LM_RDMA

    if (channel->link == ch_link_inet)
        return -1;

    shm_mr lm_mr;
    strcpy(lm_mr.name, mr->name);
    lm_mr.fd = mr->fd;
    lm_mr.addr = mr->addr;
    lm_mr.length = mr->length;

    int ret = lmShmRelease(&lm_mr);
    if (ret != 0)
        return -1;

    return 0;
}

/**
 * @API:
 */

int chRemoteRead(ch channel, void * addr, void * rem_addr, ch_event * event) {

    return 0;
}

#ifndef _DISABLE_LM_RDMA
inline int RemoteReadAsyncLinkRdma(ch channel, ch_mr mr, void * addr, 
    ch_mr rem_mr, void * rem_addr, size_t length, ch_event * event) {

    if (!(rem_mr->link_ena & CH_RDMA_ENA))
        return -1;

    //Check whether 'rem_addr' is valid
    uint64_t rem_addr_uint = (uint64_t) rem_addr;
    uint64_t rem_addr_mr_uint = (uint64_t) rem_mr->addr;
    if ((rem_addr_uint < rem_addr_mr_uint) ||
        (rem_addr_uint > rem_addr_mr_uint + rem_mr->length))
        return -1;

    //Check whether the length exceeds
    if ((rem_addr_uint + length) > 
        (rem_addr_mr_uint + rem_mr->length))
        return -1;

    uint64_t reqId = GenerateChEventId();

    //Generate channel event
    ch_event ev_tmp = new ch_event_t();
    ev_tmp->wc.rdma = new rdma_request();

    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.rdma = NULL;

    ibv_mr lm_mr_tmp;
    lm_mr_tmp.addr   = rem_mr->addr;
    lm_mr_tmp.length = rem_mr->length;
    lm_mr_tmp.handle = rem_mr->handle;
    lm_mr_tmp.lkey   = rem_mr->lkey;
    lm_mr_tmp.rkey   = rem_mr->rkey;
    int ret = lmRdmaRemoteReadAsync(channel->ctx.rdma, reqId,
        addr, mr->local_mr, rem_addr, &lm_mr_tmp, length, ev_tmp->wc.rdma);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}
#endif //_DISABLE_LM_RDMA

inline int RemoteReadAsyncLinkInet(ch channel, void * addr, ch_mr rem_mr,
    void * rem_addr, size_t length, ch_event * event) {

    if (!(rem_mr->link_ena & CH_INET_ENA))
        return -1;

    //Check whether 'rem_addr' is valid
    uint64_t rem_addr_uint = (uint64_t) rem_addr;
    uint64_t rem_addr_mr_uint = (uint64_t) rem_mr->addr;
    if ((rem_addr_uint < rem_addr_mr_uint) ||
        (rem_addr_uint > rem_addr_mr_uint + rem_mr->length)) 
        return -1;

    //Check whether the lengh exceeds
    if ((rem_addr_uint + length) >
        (rem_addr_mr_uint + rem_mr->length)) 
        return -1;

    uint64_t reqId = GenerateChEventId();
    int ret = lmInetRemoteReadAsync(channel->ctx.inet, reqId,
        addr, rem_addr, length);
    if (ret != 0) 
        return -1;

    //Generate channel event
    ch_event ev_tmp    = new ch_event_t();
    ev_tmp->type       = ch_pending;
    ev_tmp->channel    = channel;
    ev_tmp->id         = reqId;
    ev_tmp->event.inet = NULL;

    *event = ev_tmp;
    return 0;
}

/**
 * @API:
 */

int chRemoteReadAsync(ch channel, ch_mr mr, void * addr, 
    ch_mr rem_mr, void * rem_addr, size_t length, ch_event * event) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return RemoteReadAsyncLinkRdma(
            channel, mr, addr, rem_mr, rem_addr, length, event);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return RemoteReadAsyncLinkInet(
            channel, addr, rem_mr, rem_addr, length, event);
    case ch_link_shm:
        return -1; //Attention: We will not implement remote read for LM-SHM
    }

    return 0;
}

/**
 * @API:
 */

int chRemoteWrite(ch channel, void * addr, void * rem_addr, ch_event * event) {

    return 0;
}

#ifndef _DISABLE_LM_RDMA
int RemoteWriteAsyncLinkRdma(ch channel, ch_mr mr, void * addr,
    ch_mr rem_mr, void * rem_addr, size_t length, ch_event * event) {

    if (!(rem_mr->link_ena & CH_RDMA_ENA))
        return -1;

    //Check whether 'rem_addr' is valid
    uint64_t rem_addr_uint = (uint64_t) rem_addr;
    uint64_t rem_addr_mr_uint = (uint64_t) rem_mr->addr;
    if ((rem_addr_uint < rem_addr_mr_uint) ||
        (rem_addr_uint > rem_addr_mr_uint + rem_mr->length))
        return -1;

    //Check whether the length exceeds
    if ((rem_addr_uint + length) > 
        (rem_addr_mr_uint + rem_mr->length))
        return -1;

    uint64_t reqId = GenerateChEventId();

    //Generate channel event
    ch_event ev_tmp = new ch_event_t();
    ev_tmp->wc.rdma = new rdma_request();

    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.rdma = NULL;

    ibv_mr lm_mr_tmp;
    lm_mr_tmp.addr   = rem_mr->addr;
    lm_mr_tmp.length = rem_mr->length;
    lm_mr_tmp.handle = rem_mr->handle;
    lm_mr_tmp.lkey   = rem_mr->lkey;
    lm_mr_tmp.rkey   = rem_mr->rkey;
    int ret = lmRdmaRemoteWriteAsync(channel->ctx.rdma, reqId,
        addr, mr->local_mr, rem_addr, &lm_mr_tmp, length, ev_tmp->wc.rdma);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}
#endif //_DISABLE_LM_RDMA

int RemoteWriteAsyncLinkInet(ch channel, void * addr, ch_mr rem_mr,
    void * rem_addr, size_t length, ch_event * event) {

    if (!(rem_mr->link_ena & CH_INET_ENA))
        return -1;

    //Check whether 'rem_addr' is valid
    uint64_t rem_addr_uint = (uint64_t) rem_addr;
    uint64_t rem_addr_mr_uint = (uint64_t) rem_mr->addr;
    if ((rem_addr_uint < rem_addr_mr_uint) ||
        (rem_addr_uint > rem_addr_mr_uint + rem_mr->length))
        return -1;

    //Check whether the lengh exceeds
    if ((rem_addr_uint + length) >
        (rem_addr_mr_uint + rem_mr->length))
        return -1;

    uint64_t reqId = GenerateChEventId();
    int ret = lmInetRemoteWriteAsync(channel->ctx.inet, reqId,
        addr, rem_addr, length);
    if (ret != 0)
        return -1;

    //Generate channel event
    ch_event ev_tmp    = new ch_event_t();
    ev_tmp->type       = ch_pending;
    ev_tmp->channel    = channel;
    ev_tmp->id         = reqId;
    ev_tmp->event.inet = NULL;

    *event = ev_tmp;

    return 0;
    return 0;
}

/**
 * @API:
 */

int chRemoteWriteAsync(ch channel, ch_mr mr, void * addr,
    ch_mr rem_mr, void * rem_addr, size_t length, ch_event * event) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return RemoteWriteAsyncLinkRdma(channel, mr, addr, rem_mr,
            rem_addr, length, event);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return RemoteWriteAsyncLinkInet(channel, addr, rem_mr,
            rem_addr, length, event);
    case ch_link_shm:
        return -1; //Attention: We will not implement remote write for LM-SHM
    }

    return 0;
}

} //extern "C"
