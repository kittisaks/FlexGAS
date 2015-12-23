#include "channel.h"
#include "channel_internal.h"
#include <iostream>

using namespace std;

extern "C" {

#ifndef _DISABLE_LM_RDMA
inline int OpenPassiveLinkRdma(ch channel, ch_init_attr attr) {

    rdma_context   * rdma_ctx;
    rdma_init_attr   rdma_attr;

    channel->type = ch_passive;
    channel->link = ch_link_rdma;
    channel->init_attr = attr;

    rdma_attr.node.self = attr.self_id;
    rdma_attr.node.self_service = attr.self_port;
    rdma_attr.node.peer = NULL;
    rdma_attr.node.peer_service = NULL;
    rdma_attr.send_bufsize = 1048576;
    rdma_attr.recv_bufsize = 1048576;

    int ret = lmRdmaListen(&rdma_ctx, &rdma_attr);
    if (ret)
        return ret;

    channel->ctx.rdma = rdma_ctx;

    return 0;
}
#endif //_DISABLE_LM_RDMA

inline int OpenPassiveLinkInet(ch channel, ch_init_attr attr) {

    inet_context   * inet_ctx;
    inet_init_attr   inet_attr;

    channel->type = ch_passive;
    channel->link = ch_link_inet;
    channel->init_attr = attr;

    inet_attr.self = attr.self_id;
    inet_attr.self_port = attr.self_port;
    inet_attr.peer = NULL;
    inet_attr.peer_port = NULL;

    int ret = lmInetInit();
    if (ret)
        return ret;

    ret = lmInetListen(&inet_ctx, &inet_attr);
    if (ret)
        return ret;

    channel->ctx.inet = inet_ctx;

    return 0;
}

inline int OpenPassiveLinkShm(ch channel, ch_init_attr attr) {

    shm_context   * shm_ctx;
    shm_init_attr   shm_attr;

    channel->type = ch_passive;
    channel->link = ch_link_shm;
    channel->init_attr = attr;

    shm_attr.path = attr.self_id;

    int ret = lmShmInit();
    if (ret)
        return ret;

    ret = lmShmListen(&shm_ctx, &shm_attr);
    if (ret)
        return ret;

    channel->ctx.shm = shm_ctx;

    return 0;
}

inline int OpenPassive(ch * channel, ch_init_attr attr) {

    int ret;
    ch  ch_tmp = new ch_t();

    switch(attr.link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        ret = OpenPassiveLinkRdma(ch_tmp, attr);
        break;
#endif //_DISABLE_LM_RDMA

    case ch_link_inet:
        ret = OpenPassiveLinkInet(ch_tmp, attr);
        break;

    case ch_link_shm:
        ret = OpenPassiveLinkShm(ch_tmp, attr);
        break;

    default:
        return -1;
    }

    if (!ret)
        *channel = ch_tmp;
    else
        delete ch_tmp;

    return ret;
}


#ifndef _DISABLE_LM_RDMA
inline int OpenActiveLinkRdma(ch channel, ch_init_attr attr) {

    rdma_context   * rdma_ctx;
    rdma_init_attr   rdma_attr;

    channel->type = ch_passive;
    channel->link = ch_link_rdma;
    channel->init_attr = attr;

    rdma_attr.node.self = NULL;
    rdma_attr.node.self_service = NULL;
    rdma_attr.node.peer = attr.peer_id;
    rdma_attr.node.peer_service = attr.peer_port;
    rdma_attr.send_bufsize = 1048576;
    rdma_attr.recv_bufsize = 1048576;

    int ret = lmRdmaConnect(&rdma_ctx, &rdma_attr);
    if (ret)
        return ret;

    channel->ctx.rdma = rdma_ctx;

    return 0;
}
#endif //_DISABLE_LM_RDMA

inline int OpenActiveLinkInet(ch channel, ch_init_attr attr) {

    inet_context   * inet_ctx;
    inet_init_attr   inet_attr;

    channel->type = ch_passive;
    channel->link = ch_link_inet;
    channel->init_attr = attr;

    inet_attr.self = NULL;
    inet_attr.self_port = NULL;
    inet_attr.peer = attr.peer_id;
    inet_attr.peer_port = attr.peer_port;

    int ret = lmInetInit();
    if (ret)
        return ret;

    ret = lmInetConnect(&inet_ctx, &inet_attr);
    if (ret)
        return ret;

    channel->ctx.inet = inet_ctx;

    return 0;
}

inline int OpenActiveLinkShm(ch channel, ch_init_attr attr) {

    shm_context   * shm_ctx;
    shm_init_attr   shm_attr;

    channel->type = ch_passive;
    channel->link = ch_link_shm;
    channel->init_attr = attr;

    shm_attr.path = attr.peer_id;

    int ret = lmShmInit();
    if (ret)
        return ret;

    ret = lmShmConnect(&shm_ctx, &shm_attr);
    if (ret)
        return ret;

    channel->ctx.shm = shm_ctx;

    return 0;
}

inline int OpenActive(ch * channel, ch_init_attr attr) {

    int ret;
    ch  ch_tmp = new ch_t();

    switch(attr.link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        ret = OpenActiveLinkRdma(ch_tmp, attr);
        break;
#endif
    case ch_link_inet:
        ret = OpenActiveLinkInet(ch_tmp, attr);
        break;

    case ch_link_shm:
        ret = OpenActiveLinkShm(ch_tmp, attr);
        break;

    default:
        return -1;
    }

    if (!ret)
        *channel = ch_tmp;
    else
        delete ch_tmp;

    return ret;
}

/**
 * @API:
 */

int chOpen(ch * channel, ch_init_attr attr, int passive) {

    int ret;

    InitChEventIdCnt();

    if (passive)
        ret = OpenPassive(channel, attr);
    else
        ret = OpenActive(channel, attr);

    return ret;
}

#ifndef _DISABLE_LM_RDMA
inline int CloseLinkRdma(ch channel) {

    int ret = lmRdmaClose(channel->ctx.rdma);
    if (ret != 0)
        return -1;

    delete channel;

    return 0;
}
#endif //_DISABLE_LM_RDMA

inline int CloseLinkInet(ch channel) {

    int ret = lmInetClose(channel->ctx.inet);
    if (ret != 0)
        return -1;

    delete channel;

    return 0;
}

inline int CloseLinkShm(ch channel) {

    int ret = lmShmClose(channel->ctx.shm);
    if (ret != 0)
        return -1;

    delete channel;

    return 0;
}

/**
 * @API:
 */

int chClose(ch channel) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return CloseLinkRdma(channel);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return CloseLinkInet(channel);
    case ch_link_shm:
        return CloseLinkShm(channel);
    default:
        return -1;
    }

    return 0;
}

#ifndef _DISABLE_LM_RDMA
inline int PollChannelLinkRdma_SetEvent(ch_event * event,
    rdma_event * cmev,  ch channel) {

    rdma_context * ctx;
    ch_event       chev;
    int            ret;

    switch(cmev->type) {
    case RdmaEventNoEvent:
        lmRdmaReleaseEvent(cmev);
        return -1;
    case RdmaEventConnectRequest:
        chev = new ch_event_t();

        ret = lmRdmaAccept(&ctx, channel->ctx.rdma);
        if (ret == 0) {
            chev->type = ch_new_connection;
            chev->content.channel = new ch_t();
            chev->content.channel->type = ch_active;
            chev->content.channel->link = ch_link_rdma;
            chev->content.channel->ctx.rdma = ctx;
            //TODO: Fill init_attr with proper self_id etc.
            memset(&(chev->content.channel->init_attr),
                0, sizeof(ch_init_attr));

            //Fill init_attr.peer_id with the address of the remote peer
            char * peer_id_buf = inet_ntoa(ctx->sock.sin_addr);
            size_t peer_id_len = strlen(peer_id_buf);
            chev->content.channel->init_attr.peer_id = 
                new char [peer_id_len + 10];
            strcpy(chev->content.channel->init_attr.peer_id, peer_id_buf);
        }
        else
            chev->type = ch_accept_error;

        chev->channel = channel;
        chev->event.rdma = cmev;
        chev->wc.rdma = NULL;
        break;
    case RdmaEventRejected:
        chev = new ch_event_t();
        chev->type = ch_rejected;
        chev->channel = channel;
        chev->event.rdma = cmev;
        chev->wc.rdma = NULL;
        break;
    case RdmaEventEstablished:
        chev = new ch_event_t();
        chev->type = ch_established;
        chev->channel = channel;
        chev->event.rdma = cmev;
        chev->wc.rdma = NULL;
        break;
    case RdmaEventDisconnected:
        chev = new ch_event_t();
        chev->type = ch_close_connection;
        chev->channel = channel;
        chev->event.rdma = cmev;
        chev->wc.rdma = NULL;
        break;
    default:
        return -1;
    }

    *event = chev;

    return 0;
}

inline int PollChannelLinkRdma(ch_event * event, ch channel) {

    rdma_event * cmev;
    ch_event chev;

    int ret = lmRdmaGetEvent(channel->ctx.rdma, &cmev);
    if (ret != 0)
        return -1;

    ret = lmRdmaAckEvent(cmev);
    if (ret != 0)
        return -1;

    ret = PollChannelLinkRdma_SetEvent(&chev, cmev, channel);
    if (ret != 0)
        return ret;

    *event = chev;

    return 0;
}
#endif

inline int PollChannelLinkInet_SetEvent(ch_event * event,
    inet_event * inev, ch channel) {

    inet_context * ctx;
    ch_event       chev;
    int            ret;

    switch(inev->type) {
    case INET_EVENT_NOEVENT:
        lmInetReleaseEvent(inev);
        return -1;
    case INET_EVENT_CONNECT_REQUEST:
        chev = new ch_event_t();
        ret = lmInetAccept(&ctx, channel->ctx.inet);
        if (ret == 0) {
            chev->type = ch_new_connection;
            chev->content.channel = new ch_t();
            chev->content.channel->type = ch_active;
            chev->content.channel->link = ch_link_inet;
            chev->content.channel->ctx.inet = ctx;
            //TODO: Fill init_attr with proper self_id etc.
            memset(&(chev->content.channel->init_attr),
                0, sizeof(ch_init_attr));

            //Fill init_attr.peer_id with the address of the remote peer
            char * peer_id_buf = inet_ntoa(ctx->sock->sin_addr);
            size_t peer_id_len = strlen(peer_id_buf);
            chev->content.channel->init_attr.peer_id = 
                new char [peer_id_len + 10];
            strcpy(chev->content.channel->init_attr.peer_id, peer_id_buf);
        }
        else {
            chev->type = ch_accept_error;
        }
        chev->channel = channel;
        chev->event.inet = inev;
        chev->wc.inet = NULL;
        break;
    case INET_EVENT_ESTABLISHED:
        chev = new ch_event_t();
        chev->type = ch_established;
        chev->channel = channel;
        chev->event.inet = inev;
        chev->wc.inet = NULL;
        break;
    case INET_EVENT_REJECTED:
        chev = new ch_event_t();
        chev->type = ch_rejected;
        chev->channel = channel;
        chev->event.inet = inev;
        chev->wc.inet = NULL;
        break;
    case INET_EVENT_DISCONNECTED:
        chev = new ch_event_t();
        chev->type = ch_close_connection;
        chev->channel = channel;
        chev->event.inet = inev;
        chev->wc.inet = NULL;
        break;
    default:
        return -1;
    }

    *event = chev;

    return 0;
}

inline int PollChannelLinkInet(ch_event * event, ch channel) {

    inet_event   * inev;
    ch_event       chev;
    int            ret;

    ret = lmInetGetEvent(channel->ctx.inet, &inev);
    if (ret != 0)
        return ret;

    ret = lmInetAckEvent(inev);
    if (ret != 0)
        return ret;

    ret = PollChannelLinkInet_SetEvent(&chev, inev, channel);
    if (ret != 0)
        return ret;

    *event = chev;

    return 0;
}

inline int PollChannelLinkShm_SetEvent(ch_event * event,
    shm_event * shev, ch channel) {

    shm_context * ctx;
    ch_event      chev;
    int           ret;

    switch(shev->type) {
    case SHM_EVENT_NOEVENT:
        lmShmReleaseEvent(shev);
        return -1;
    case SHM_EVENT_CONNECT_REQUEST:
        chev = new ch_event_t();
        ret = lmShmAccept(&ctx, channel->ctx.shm);
        if (ret == 0) {
            chev->type = ch_new_connection;
            chev->content.channel = new ch_t();
            chev->content.channel->type = ch_active;
            chev->content.channel->link = ch_link_shm;
            chev->content.channel->ctx.shm = ctx;
            //TODO: Fill init_attr with proper self_id etc.
            memset(&(chev->content.channel->init_attr),
                0, sizeof(ch_init_attr));
        }
        else
            chev->type = ch_accept_error;
        chev->channel = channel;
        chev->event.shm = shev;
        chev->wc.shm = NULL;
        break;
    case SHM_EVENT_ESTABLISHED:
        chev = new ch_event_t();
        chev->type = ch_established;
        chev->channel = channel;
        chev->event.shm = shev;
        chev->wc.shm = NULL;
        break;
    case SHM_EVENT_REJECTED:
        chev = new ch_event_t();
        chev->type = ch_rejected;
        chev->channel = channel;
        chev->event.shm = shev;
        chev->wc.shm = NULL;
        break;
    case SHM_EVENT_DISCONNECTED:
        chev = new ch_event_t();
        chev->type = ch_close_connection;
        chev->channel = channel;
        chev->event.shm = shev;
        chev->wc.shm = NULL;
        break;
    default:
        return -1;
    }

    *event = chev;

    return 0;
}

inline int PollChannelLinkShm(ch_event * event, ch channel) {

    shm_event   * shev;
    ch_event      chev;
    int           ret;

    ret = lmShmGetEvent(&shev, channel->ctx.shm);
    if (ret != 0)
        return ret;

    ret = lmShmAckEvent(shev);
    if (ret != 0)
        return ret;

    ret = PollChannelLinkShm_SetEvent(&chev, shev, channel);
    if (ret != 0)
        return ret;

    *event = chev;

    return 0;
}

/**
 * @API:
 */

int chPollChannel(ch_event * event, ch channel) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return PollChannelLinkRdma(event, channel);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return PollChannelLinkInet(event, channel);
    case ch_link_shm:
        return PollChannelLinkShm(event, channel);
    default:
        return -1;
    }

    return 0;
}

#ifndef _DISABLE_LM_RDMA
int WaitChannelLinkRdma(ch_event * event, ch channel) {

    rdma_event * cmev;
    ch_event chev;

    int ret = lmRdmaWaitEvent(channel->ctx.rdma, &cmev);
    if (ret != 0)
        return -1;

    ret = lmRdmaAckEvent(cmev);
    if (ret != 0)
        return -1;

    ret = PollChannelLinkRdma_SetEvent(&chev, cmev, channel);
    if (ret != 0)
        return ret;

    *event = chev;

    return 0;
}
#endif //_DISABLE_LM_RDMA

int WaitChannelLinkInet(ch_event * event, ch channel) {

    inet_event   * inev;
    ch_event       chev;
    int            ret;

    ret = lmInetWaitEvent(channel->ctx.inet, &inev);
    if (ret != 0)
        return ret;

    ret = lmInetAckEvent(inev);
    if (ret != 0)
        return ret;

    ret = PollChannelLinkInet_SetEvent(&chev, inev, channel);
    if (ret != 0)
        return ret;

    *event = chev;

    return 0;
}

int WaitChannelLinkShm(ch_event * event, ch channel) {

    shm_event   * shev;
    ch_event      chev;
    int           ret;

    ret = lmShmWaitEvent(&shev, channel->ctx.shm);
    if (ret != 0)
        return ret;

    ret = lmShmAckEvent(shev);
    if (ret != 0)
        return ret;

    ret = PollChannelLinkShm_SetEvent(&chev, shev, channel);
    if (ret != 0)
        return ret;

    *event = chev;

    return 0;
}

/**
 * @API:
 */

int chWaitChannel(ch_event * event, ch channel) {

    switch(channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return WaitChannelLinkRdma(event, channel);
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return WaitChannelLinkInet(event, channel);
    case ch_link_shm:
        return WaitChannelLinkShm(event, channel);
    default:
        return -1;
    }

    return 0;
}

#ifndef _DISABLE_LM_RDMA
int PollEventLinkRdma(ch_event event) {

    event->wc.rdma = new rdma_request();

    int ret = lmRdmaGetComp(event->channel->ctx.rdma,
        event->wc.rdma);
    if (ret != 0)
        return ret;

    switch(event->wc.rdma->wc.status) {
    case IBV_WC_SUCCESS:
        event->type = ch_success;
        break;
    case IBV_WC_LOC_LEN_ERR:
        event->type = ch_length_error;
        break;
    case IBV_WC_LOC_QP_OP_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_LOC_EEC_OP_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_LOC_PROT_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_WR_FLUSH_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_MW_BIND_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_BAD_RESP_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_LOC_ACCESS_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_REM_INV_REQ_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_REM_ACCESS_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_REM_OP_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_RETRY_EXC_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_RNR_RETRY_EXC_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_LOC_RDD_VIOL_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_REM_INV_RD_REQ_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_REM_ABORT_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_INV_EECN_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_INV_EEC_STATE_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_FATAL_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_RESP_TIMEOUT_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_GENERAL_ERR:
        event->type = ch_link_unhandled;
        break;
    default:
        event->type = ch_unknown_error;
        break;
    }

    return 0;
}
#endif //_DISABLE_LM_RDMA

int PollEventLinkInet(ch_event event) {

    int ret = lmInetGetComp(event->id, &(event->wc.inet));
    if (ret != 0)
        return ret;

    switch(event->wc.inet->status) {
    case Pending:
        event->type = ch_pending; break;
        break;
    case Success:
        event->type = ch_success; break;
        break;
    case Fail:
        event->type = ch_error; break;
        break;
    case LengthError:
        event->type = ch_length_error; break;
        break;
    default:
        event->type = ch_unknown_error; break;
    }

    return 0;
}

int PollEventLinkShm(ch_event event) {

    int ret = lmShmGetComp(event->id, &(event->wc.shm));
    if (ret != 0)
        return ret;

    switch(event->wc.shm->status) {
    case ShmPending:
        event->type = ch_pending; break;
        break;
    case ShmSuccess:
        event->type = ch_success; break;
        break;
    case ShmFail:
        event->type = ch_error; break;
        break;
    case ShmLengthError:
        event->type = ch_length_error; break;
        break;
    default:
        event->type = ch_unknown_error; break;
    }

    return ret;
}

/**
 * @API:
 */

int chPollEvent(ch_event event) {

    switch (event->channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return PollEventLinkRdma(event); 
#endif //_DISABLE_LM_RDMA
    case ch_link_inet:
        return PollEventLinkInet(event);
    case ch_link_shm:
        return PollEventLinkShm(event);
    default:
        return -1;
    }

    return -1;
}

#ifndef _DISABLE_LM_RDMA
int WaitEventLinkRdma(ch_event event) {

    int ret = lmRdmaWaitComp(event->channel->ctx.rdma,
        event->wc.rdma);
    if (ret != 0)
        return ret;

    switch(event->wc.rdma->wc.status) {
    case IBV_WC_SUCCESS:
        event->type = ch_success;
        break;
    case IBV_WC_LOC_LEN_ERR:
        event->type = ch_length_error;
        break;
    case IBV_WC_LOC_QP_OP_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_LOC_EEC_OP_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_LOC_PROT_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_WR_FLUSH_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_MW_BIND_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_BAD_RESP_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_LOC_ACCESS_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_REM_INV_REQ_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_REM_ACCESS_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_REM_OP_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_RETRY_EXC_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_RNR_RETRY_EXC_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_LOC_RDD_VIOL_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_REM_INV_RD_REQ_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_REM_ABORT_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_INV_EECN_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_INV_EEC_STATE_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_FATAL_ERR:
        event->type = ch_link_unhandled;
        break;
    case IBV_WC_RESP_TIMEOUT_ERR:
        event->type = ch_error;
        break;
    case IBV_WC_GENERAL_ERR:
        event->type = ch_link_unhandled;
        break;
    default:
        event->type = ch_unknown_error;
        break;
    }

    return 0;
}
#endif //_DISABLE_LM_RDMA

int WaitEventLinkInet(ch_event event) {

    int ret = lmInetWaitComp(event->id, &(event->wc.inet));
    if (ret != 0)
        return ret;

    switch(event->wc.inet->status) {
    case Pending:
        event->type = ch_pending; break;
        break;
    case Success:
        event->type = ch_success; break;
        break;
    case Fail:
        event->type = ch_error; break;
        break;
    case LengthError:
        event->type = ch_length_error; break;
        break;
    default:
        event->type = ch_unknown_error; break;
    }

    return 0;
}

int WaitEventLinkShm(ch_event event) {

    int ret = lmShmWaitComp(event->id, &(event->wc.shm));
    if (ret != 0)
        return ret;

    switch(event->wc.shm->status) {
    case ShmPending:
        event->type = ch_pending; break;
        break;
    case ShmSuccess:
        event->type = ch_success; break;
        break;
    case ShmFail:
        event->type = ch_error; break;
        break;
    case ShmLengthError:
        event->type = ch_length_error; break;
        break;
    default:
        event->type = ch_unknown_error; break;
    }

    return 0;
}

/**
 * @API:
 */

int chWaitEvent(ch_event event) {

    switch(event->channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        return WaitEventLinkRdma(event);
#endif
    case ch_link_inet:
        return WaitEventLinkInet(event);
    case ch_link_shm:
        return WaitEventLinkShm(event);
    default:
        return -1;
    }

    return -1;
}

/**
 * @API:
 */

int chReleaseEvent(ch_event event) {

    switch(event->channel->link) {
#ifndef _DISABLE_LM_RDMA
    case ch_link_rdma:
        if (event->event.rdma != NULL)
            lmRdmaReleaseEvent(event->event.rdma);
        if (event->wc.rdma != NULL)
            delete event->wc.rdma;
        break;
#endif //DISABLE_LM_RDMA
    case ch_link_inet:
        if (event->event.inet != NULL)
            lmInetReleaseEvent(event->event.inet);
        if (event->wc.inet != NULL)
            lmInetReleaseComp(event->wc.inet);
        break;
    case ch_link_shm:
        if (event->event.shm != NULL)
            lmShmReleaseEvent(event->event.shm);
        if (event->wc.shm != NULL)
            lmShmReleaseComp(event->wc.shm);
        break;
    default:
        return -1;
    }

    delete event;

    return 0;
}

} //extern "C"

