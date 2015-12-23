#include "channel.h"
#include "channel_internal.h"
#include <iostream>

using namespace std;

extern "C" {

#ifndef _DISABLE_LM_RDMA
inline int SendLinkRdma(ch channel, void * addr, size_t length) {

    int reqId = GenerateChEventId();

    return lmRdmaSendUnbuffered(channel->ctx.rdma, reqId, addr, length);
}
#endif //_DISABLE_LM_RDMA

inline int SendLinkInet(ch channel, void * addr, size_t length) {

    int reqId = GenerateChEventId();

    return lmInetSend(channel->ctx.inet, reqId, addr, length);
}

inline int SendLinkShm(ch channel, void * addr, size_t length) {

    int reqId = GenerateChEventId();

    return lmShmSend(channel->ctx.shm, reqId, addr, length);
}

/**
 * @API:
 */

int chSend(ch channel, void * addr, size_t length) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return SendLinkRdma(channel, addr, length);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return SendLinkInet(channel, addr, length);
    case ch_link_shm:
        return SendLinkShm(channel, addr, length);
    default:
        return -1;
    }

    return 0;
}

#ifndef _DISABLE_LM_RDMA
int SendAsyncLinkRdma(ch channel, void * addr,
    size_t length, ch_event * event) {

    ch_event ev_tmp = new ch_event_t();
    ev_tmp->wc.rdma = new rdma_request();

    uint64_t reqId = GenerateChEventId();
    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.rdma = NULL;

    int ret = lmRdmaSendUnbufferedAsync(channel->ctx.rdma, reqId,
        addr, length, ev_tmp->wc.rdma);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}
#endif //_DISABLE_LM_RDMA

int SendAsyncLinkInet(ch channel, void * addr,
    size_t length, ch_event * event) {

    ch_event ev_tmp = new ch_event_t();

    uint64_t reqId = GenerateChEventId();
    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.inet = NULL;

    int ret = lmInetSendAsync(channel->ctx.inet, reqId,
        addr, length);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}


int SendAsyncLinkShm(ch channel, void * addr,
    size_t length, ch_event * event) {

    ch_event ev_tmp = new ch_event_t();

    uint64_t reqId = GenerateChEventId();
    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.shm = NULL;

    int ret = lmShmSendAsync(channel->ctx.shm, reqId,
        addr, length);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}

/**
 * @API:
 */

int chSendAsync(ch channel, void * addr, size_t length, ch_event * event) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return SendAsyncLinkRdma(channel, addr, length, event);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return SendAsyncLinkInet(channel, addr, length, event);
    case ch_link_shm:
        return SendAsyncLinkShm(channel, addr, length, event);
    default:
        return -1;
    }

    return 0;
}

#ifndef _DISABLE_LM_RDMA
inline int RecvLinkRdma(ch channel, void * addr, size_t length) {

    int reqId = GenerateChEventId();

    return lmRdmaRecvUnbuffered(channel->ctx.rdma, reqId, addr ,length);
}
#endif //_DISABLE_LM_RDMA

inline int RecvLinkInet(ch channel, void * addr, size_t length) {

    int reqId = GenerateChEventId();

    return lmInetRecv(channel->ctx.inet, reqId, addr ,length);
}

inline int RecvLinkShm(ch channel, void * addr, size_t length) {

    int reqId = GenerateChEventId();

    return lmShmRecv(channel->ctx.shm, reqId, addr ,length);
}

/**
 * @API:
 */

int chRecv(ch channel, void * addr, size_t length) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return RecvLinkRdma(channel, addr, length);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return RecvLinkInet(channel, addr, length);
    case ch_link_shm:
        return RecvLinkShm(channel, addr, length);
    default:
        return -1;
    }

    return 0;
}

#ifndef _DISABLE_LM_RDMA
inline int RecvAsyncLinkRdma(ch channel, void * addr, size_t length,
    ch_event * event) {

    ch_event ev_tmp = new ch_event_t();
    ev_tmp->wc.rdma = new rdma_request();

    int reqId = GenerateChEventId();
    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.rdma = NULL;

    int ret = lmRdmaRecvUnbufferedAsync(channel->ctx.rdma, reqId,
        addr, length, ev_tmp->wc.rdma);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}
#endif //_DISABLE_LM_RDMA

inline int RecvAsyncLinkInet(ch channel, void * addr, size_t length,
    ch_event * event) {

    ch_event ev_tmp = new ch_event_t();

    int reqId = GenerateChEventId();
    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.inet = NULL;

    int ret = lmInetRecvAsync(channel->ctx.inet, reqId, addr, length);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}

inline int RecvAsyncLinkShm(ch channel, void * addr, size_t length,
    ch_event * event) {

    ch_event ev_tmp = new ch_event_t();

    int reqId = GenerateChEventId();
    ev_tmp->type = ch_pending;
    ev_tmp->channel = channel;
    ev_tmp->id = reqId;
    ev_tmp->event.shm = NULL;

    int ret = lmShmRecvAsync(channel->ctx.shm, reqId, addr, length);
    if (ret != 0)
        return -1;

    *event = ev_tmp;

    return 0;
}

/**
 * @API:
 */

int chRecvAsync(ch channel, void * addr, size_t length, ch_event * event) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return RecvAsyncLinkRdma(channel, addr, length, event);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return RecvAsyncLinkInet(channel, addr, length, event);
    case ch_link_shm:
        return RecvAsyncLinkShm(channel, addr, length, event);
    default:
        return -1;
    }

    return 0;
}

/**
 * @API:
 */

int chSendMr(ch channel, ch_mr mr) {

    int ret = 0;

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        if (!(mr->link_ena & CH_RDMA_ENA)) {
            ret = chRegisterMemory(channel, mr->addr, mr->length, &mr,
                CH_ADD_LINK_ENA);
            if (ret != 0)                
                return -1;
        }

        ret = chSend(channel, mr, sizeof(ch_mr_t));
        if (ret != 0)
            return -1;
        break;
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        if (!(mr->link_ena & CH_INET_ENA)) {
            ret = chRegisterMemory(channel, mr->addr, mr->length, &mr,
                CH_ADD_LINK_ENA);
            if (ret != 0)
                return -1;
        }

        ret = chSend(channel, mr, sizeof(ch_mr_t));
        if (ret != 0)
            return -1;
        break;
    case ch_link_shm:
        /**
         * Attention: If the memory region is not initially enabled with
         *            CH_SHM_ENA, then currently there is no way to further
         *            make the memory region sharable by multiple processes.
         *            We immediately return an error for this case.
         */
        if (!(mr->link_ena & CH_SHM_ENA))
            return -1;

        ret = chSend(channel, mr, sizeof(ch_mr_t));
        if (ret != 0)
            return -1;
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @API:
 */

int chRecvMr(ch channel, ch_mr * mr) {

    ch_mr mr_tmp = new ch_mr_t();

    int ret = 0;

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        ret = chRecv(channel, mr_tmp, sizeof(ch_mr_t));
        if (ret != 0)
            return -1;
        mr_tmp->type = ch_mr_t::ch_mr_remote;
        break;
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        ret = chRecv(channel, mr_tmp, sizeof(ch_mr_t));
        if (ret != 0)
            return -1;
        mr_tmp->type = ch_mr_t::ch_mr_remote;
        break;
    case ch_link_shm:
        ret = chRecv(channel, mr_tmp, sizeof(ch_mr_t));
        if (ret != 0)
            return -1;
        mr_tmp->type = ch_mr_t::ch_mr_remote;
        break;
    default:
        return -1;
    }

    //TODO: What if channe->link is not the same as mr_tmp->link
//    if (channel->link != mr_tmp->link)
//        return -1;

    mr_tmp->type = ch_mr_t::ch_mr_remote;

    *mr = mr_tmp;

    return 0;
}

} //extern "C"

