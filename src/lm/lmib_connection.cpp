#include "lmib.h"
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

ibv_device      ** dev_lst      = NULL;
ibv_device       * dev          = NULL;
ibv_context      * context      = NULL;
ibv_device_attr    device_attr;
ibv_pd           * pd           = NULL;
bool               isLibInit    = false;

int InitRdmaLib() {

    if (isLibInit)
        return 0;

    dev_lst = ibv_get_device_list(NULL);
    if (dev_lst == NULL)
        return -1;

    dev = dev_lst[0];
    if (dev == NULL)
        return -1;

    context = ibv_open_device(dev);
    if (context == NULL)
        return -1;

    int ret = ibv_query_device(context, &device_attr);
    if (ret != 0)
        return -1;
#if 0
    cout << "max_mr_size:         " << device_attr.max_mr_size         << endl;
    cout << "max_qp:              " << device_attr.max_qp              << endl;
    cout << "max_qp_wr:           " << device_attr.max_qp_wr           << endl;
    cout << "max_sge:             " << device_attr.max_sge             << endl;
    cout << "max_sge_rd:          " << device_attr.max_sge_rd          << endl;
    cout << "max_cq:              " << device_attr.max_cq              << endl;
    cout << "max_cqe:             " << device_attr.max_cqe             << endl;
    cout << "max_mr:              " << device_attr.max_mr              << endl;
    cout << "max_pd:              " << device_attr.max_pd              << endl;
    cout << "max_qp_rd_atom:      " << device_attr.max_qp_rd_atom      << endl;
    cout << "max_ee_rd_atom:      " << device_attr.max_ee_rd_atom      << endl;
    cout << "max_res_rd_atom:     " << device_attr.max_res_rd_atom     << endl;
    cout << "max_qp_init_rd_atom: " << device_attr.max_qp_init_rd_atom << endl;
    cout << "max_ee_init_rd_atom: " << device_attr.max_ee_init_rd_atom << endl;
    cout << "max_ee:              " << device_attr.max_ee              << endl;
    cout << "max_rdd:             " << device_attr.max_rdd             << endl;
    cout << "max_mw:              " << device_attr.max_mw              << endl;
    cout << "max_raw_ipv6_qp:     " << device_attr.max_raw_ipv6_qp     << endl;
    cout << "max_raw_ethy_qp:     " << device_attr.max_raw_ethy_qp     << endl;
    cout << "max_mcast_grp:       " << device_attr.max_mcast_grp       << endl;
    cout << "max_mcast_qp_attach: " << device_attr.max_mcast_qp_attach << endl;
    cout << "max_total_mcast_qp_attach: " 
        << device_attr.max_total_mcast_qp_attach << endl;
    cout << "max_ah:              " << device_attr.max_ah              << endl;
    cout << "max_fmr:             " << device_attr.max_fmr             << endl;
    cout << "max_map_per_fmr:     " << device_attr.max_map_per_fmr     << endl;
    cout << "max_srq:             " << device_attr.max_srq             << endl;
    cout << "max_srq_wr:          " << device_attr.max_srq_wr          << endl;
    cout << "max_srq_sge:         " << device_attr.max_srq_sge         << endl;
    cout << "max_pkeys:           " << device_attr.max_pkeys           << endl;
#endif

    pd = ibv_alloc_pd(context);
    if (pd == NULL)
        return -1;

    isLibInit = true;

    return 0;
}

int GetLocalIbInfo(rdma_context * ctx) {

    ibv_port_attr ib_port_attr;
    int ret = ibv_query_port(
        ctx->context, ctx->phy_port, &ib_port_attr);
    if (ret != 0)
        return -1;

    ctx->self_ib.lid = ib_port_attr.lid;
    ctx->self_ib.qpn = ctx->qp->qp_num;
    ctx->self_ib.psn = 0x123456;

    ctx->self_ib.ppb = new char [16];
    ctx->self_ib.oppb_mr  = ibv_reg_mr(ctx->pd, ctx->self_ib.ppb, 16,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ);
    if (ctx->self_ib.oppb_mr == NULL)
        return -1;
    memcpy(&ctx->self_ib.ppb_mr, ctx->self_ib.oppb_mr, sizeof(ibv_mr));

    return 0;
}

int SetQpInit(rdma_context * ctx) {
    memset(&ctx->qp_attr_init, 0, sizeof(ibv_qp_attr));

    ctx->qp_attr_init.qp_state        = IBV_QPS_INIT;
    ctx->qp_attr_init.pkey_index      = 0;
    ctx->qp_attr_init.port_num        = ctx->phy_port;
    ctx->qp_attr_init.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | 
        IBV_ACCESS_REMOTE_READ;

    int ret = ibv_modify_qp(ctx->qp, &ctx->qp_attr_init,
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
    if (ret != 0)
        return -1;

    return 0;
}

int SetQpRtr(rdma_context * ctx) {

    memset(&ctx->qp_attr_rtr, 0, sizeof(ibv_qp_attr));
    ctx->qp_attr_rtr.qp_state              = IBV_QPS_RTR;
    ctx->qp_attr_rtr.path_mtu              = IBV_MTU_2048;
    ctx->qp_attr_rtr.dest_qp_num           = ctx->peer_ib.qpn;
    ctx->qp_attr_rtr.rq_psn                = ctx->peer_ib.psn;
    ctx->qp_attr_rtr.max_dest_rd_atomic    = device_attr.max_qp_rd_atom;
    ctx->qp_attr_rtr.min_rnr_timer         = 12;
    ctx->qp_attr_rtr.ah_attr.is_global     = 0;
    ctx->qp_attr_rtr.ah_attr.dlid          = ctx->peer_ib.lid;
    ctx->qp_attr_rtr.ah_attr.sl            = 1;
    ctx->qp_attr_rtr.ah_attr.src_path_bits = 0;
    ctx->qp_attr_rtr.ah_attr.port_num      = ctx->phy_port;

    int ret = ibv_modify_qp(ctx->qp, &ctx->qp_attr_rtr,
        IBV_QP_STATE              |
        IBV_QP_AV                 |
        IBV_QP_PATH_MTU           |
        IBV_QP_DEST_QPN           |
        IBV_QP_RQ_PSN             |
        IBV_QP_MAX_DEST_RD_ATOMIC |
        IBV_QP_MIN_RNR_TIMER);
    if (ret == -1)
        return -1;

    return 0;
}

int SetQpRts(rdma_context * ctx) {

    memset(&ctx->qp_attr_rts, 0, sizeof(ibv_qp_attr));

    ctx->qp_attr_rts.qp_state      = IBV_QPS_RTS;
    ctx->qp_attr_rts.timeout       = 14;
    ctx->qp_attr_rts.retry_cnt     = 7;
    ctx->qp_attr_rts.rnr_retry     = 7;
    ctx->qp_attr_rts.sq_psn        = ctx->self_ib.psn;
    ctx->qp_attr_rts.max_rd_atomic = device_attr.max_qp_rd_atom;

    int ret = ibv_modify_qp(ctx->qp, &ctx->qp_attr_rts,
        IBV_QP_STATE            |
        IBV_QP_TIMEOUT          |
        IBV_QP_RETRY_CNT        |
        IBV_QP_RNR_RETRY        |
        IBV_QP_SQ_PSN           |
        IBV_QP_MAX_QP_RD_ATOMIC);
    if (ret == -1)
        return -1;

    return 0;
}

int InitActiveContext(rdma_context ** ctx) {

    if (ctx == NULL)
        return -1;

    int ret = InitRdmaLib();
    if (ret != 0)
        return -1;

    rdma_context * ctx_temp = new rdma_context();

    ctx_temp->status = RdmaContextNull; 
    ctx_temp->dev_lst = dev_lst;
    //The physical HCA is fixed.
    ctx_temp->dev = dev;
    //The physical port of the HCA is fixed.
    ctx_temp->phy_port = 1;
    ctx_temp->context = context;
    ctx_temp->pd = pd;

    ctx_temp->scc = ibv_create_comp_channel(ctx_temp->context);
    if (ctx_temp->scc == NULL)
        return -1;

    ctx_temp->rcc = ibv_create_comp_channel(ctx_temp->context);
    if (ctx_temp->rcc == NULL)
        return -1;

    ctx_temp->scq = ibv_create_cq(
        ctx_temp->context, 10, NULL, ctx_temp->scc, 0);
    if (ctx_temp->scq == NULL)
        return -1;

    ctx_temp->rcq = ibv_create_cq(
        ctx_temp->context, 10, NULL, ctx_temp->rcc, 0);
    if (ctx_temp->rcq == NULL)
        return -1;

    ctx_temp->qp_init_attr.send_cq             = ctx_temp->scq;
    ctx_temp->qp_init_attr.recv_cq             = ctx_temp->rcq;
    ctx_temp->qp_init_attr.qp_type             = IBV_QPT_RC;
    ctx_temp->qp_init_attr.cap.max_send_wr     = 100;
    ctx_temp->qp_init_attr.cap.max_recv_wr     = 100;
    ctx_temp->qp_init_attr.cap.max_send_sge    = 1;
    ctx_temp->qp_init_attr.cap.max_recv_sge    = 1;
    ctx_temp->qp_init_attr.cap.max_inline_data = 0;

    ctx_temp->qp = ibv_create_qp(ctx_temp->pd, &ctx_temp->qp_init_attr);
    if (ctx_temp->qp == NULL)
        return -1;

    *ctx = ctx_temp;

    return 0;
}

/**
 * @API
 */

int lmRdmaListen(rdma_context ** ctx, rdma_init_attr * init_attr) {

    if (init_attr == NULL)
        return -1;
    if (init_attr->node.self_service == NULL)
        return -1;

    rdma_context * ctx_temp = new rdma_context();
    memset(ctx_temp, 0, sizeof(rdma_context));

    //Initiate IB connection with a socket
    ctx_temp->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx_temp->fd == -1)
        return -1;

    //Set the socket to be close-on-exec and blocking
    int fd_flags = fcntl(ctx_temp->fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    int ret = fcntl(ctx_temp->fd, F_SETFL,
        (fd_flags | O_NONBLOCK));
    if (ret == -1)
        return -1;
    ret = fcntl(ctx_temp->fd, F_SETFD, FD_CLOEXEC);
    if (ret == -1)
        return -1;

    //Reuse the port number even it is in the TIME_WAIT state.
    int so_reuseaddr_val = 1;
    setsockopt(ctx_temp->fd, SOL_SOCKET,
        SO_REUSEADDR, &so_reuseaddr_val, sizeof(int));

    ctx_temp->sock.sin_family      = AF_INET;
    ctx_temp->sock.sin_addr.s_addr = htonl(INADDR_ANY);
    ctx_temp->sock.sin_port        = htons(atoi(init_attr->node.self_service));

    //Bind and listen using the socket
    ret = bind(ctx_temp->fd, (sockaddr *) &ctx_temp->sock,
        sizeof(sockaddr_in));
    if (ret == -1)
        return -1;

    ret = listen(ctx_temp->fd, 2000);
    if (ret == -1)
        return -1;

    //Finalize and set status and event properly
    memcpy(&ctx_temp->init_attr, init_attr, sizeof(rdma_init_attr));
    ctx_temp->init_attr.type = RdmaContextPassive;
    ctx_temp->status         = RdmaContextEstablished;
    ctx_temp->event.type     = RdmaEventNoEvent;
    *ctx = ctx_temp;

    return 0;
}

/**
 * @API
 */

int lmRdmaConnect(rdma_context ** ctx, rdma_init_attr * init_attr) {

    if (init_attr == NULL)
        return -1;
    if (init_attr->node.peer == NULL)
        return -1;
    if (init_attr->node.peer_service == NULL)
        return -1;

    //Initialize local IB connection
    rdma_context * ctx_temp;
    int            ret;

    ret = InitActiveContext(&ctx_temp);
    if (ret != 0)
        return -1;

    //Prepare local IB information to be sent to the peer
    ret = GetLocalIbInfo(ctx_temp);
    if (ret != 0)
        return -1;

    //Start initiating IB connection using a socket connection
    ctx_temp->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx_temp->fd == -1)
        return -1;

    //Set the socket to be close-on-exec and blocking
    int fd_flags = fcntl(ctx_temp->fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    ret = fcntl(ctx_temp->fd, F_SETFL,
        (fd_flags) & (~O_NONBLOCK));
    if (ret == -1)
        return -1;
    ret = fcntl(ctx_temp->fd, F_SETFD, FD_CLOEXEC);
    if (ret == -1)
        return -1;

    //Set Send/Recv buffer size
    uint64_t send_buf_size = 1048576;
    uint64_t recv_buf_size = 1048576;
    setsockopt(ctx_temp->fd, SOL_SOCKET,
        SO_SNDBUF, &send_buf_size, sizeof(uint64_t));
    setsockopt(ctx_temp->fd, SOL_SOCKET,
        SO_RCVBUF, &recv_buf_size, sizeof(uint64_t));

    //Reuse the port number evenr it is in the TIME_WAIT state
    int so_reuseaddr_val = 1;
    setsockopt(ctx_temp->fd, SOL_SOCKET,
        SO_REUSEADDR, &so_reuseaddr_val, sizeof(int));

    //Resolve peer address
    addrinfo * addr;
    ret = getaddrinfo(init_attr->node.peer, init_attr->node.peer_service,
        NULL, &addr);
    if (ret == -1)
        return -1;
    memcpy(&ctx_temp->sock, addr->ai_addr, addr->ai_addrlen);
//    free(addr);

    //Conenct to the peer
    ret = connect(ctx_temp->fd, (sockaddr *) &ctx_temp->sock, addr->ai_addrlen);
    if (ret == -1)
        return -1;

    //Exchange IB information
    ret = recv(ctx_temp->fd, &ctx_temp->peer_ib,
        sizeof(rdma_node_ib), MSG_WAITALL);
    if (ret != sizeof(rdma_node_ib))
        return -1;

    ret = send(ctx_temp->fd, &ctx_temp->self_ib,
        sizeof(rdma_node_ib), MSG_WAITALL);
    if (ret != sizeof(rdma_node_ib))
        return -1;

    //Bring up the queue pair
    ret = SetQpInit(ctx_temp);
    if (ret != 0)
        return -1;

    ret = SetQpRtr(ctx_temp);
    if (ret != 0)
        return -1;

    ret = SetQpRts(ctx_temp);
    if (ret != 0)
        return -1;

    //Finalize and set status and event properly
    memcpy(&ctx_temp->init_attr, init_attr, sizeof(rdma_init_attr));
    ctx_temp->init_attr.type = RdmaContextActive;
    ctx_temp->status         = RdmaContextEstWaitAck;
    ctx_temp->event.type     = RdmaEventEstablished;
    *ctx = ctx_temp;

    //Set the socket to be close-on-exec and blocking
    fd_flags = fcntl(ctx_temp->fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    ret = fcntl(ctx_temp->fd, F_SETFL,
        fd_flags | O_NONBLOCK);
    if (ret == -1)
        return -1;

    return 0;
}

/**
 * @API
 */

int lmRdmaAccept(rdma_context ** n_ctx, rdma_context * ctx) {

    rdma_context * ctx_temp;
    int            ret;

    ret = InitActiveContext(&ctx_temp);
    if (ret != 0)
        return -1;

    //Prepare local IB information to be sent to the peer
    ret = GetLocalIbInfo(ctx_temp);
    if (ret != 0)
        return -1;

    if (ctx->init_attr.type != RdmaContextPassive)
        return -1;

    //Since the listening socket is non-blocking,
    //we use use poll to detect the incoming connection
    pollfd fd_set;

    fd_set.fd     = ctx->fd;
    fd_set.events = POLLIN;
    poll(&fd_set, 1, -1);

    //Accept the socket connection
    memset(&ctx_temp->sock, 0, sizeof(struct sockaddr_in)); //**//
    ctx_temp->socklen = sizeof(struct sockaddr_in); //**//
    // ===== //**// ===== - Necessary to obtain sockaddr_in correctly
    ctx_temp->fd = accept(
        ctx->fd, (sockaddr *) &ctx_temp->sock, &ctx_temp->socklen);
    if (ctx_temp->fd == -1)
        return -1;

    //Set Send/Recv buffer size
    uint64_t send_buf_size = 1048576;
    uint64_t recv_buf_size = 1048576;
    setsockopt(ctx_temp->fd, SOL_SOCKET,
        SO_SNDBUF, &send_buf_size, sizeof(uint64_t));
    setsockopt(ctx_temp->fd, SOL_SOCKET,
        SO_RCVBUF, &recv_buf_size, sizeof(uint64_t));

    //Set the socket to be close-on-exec and blocking
    int fd_flags = fcntl(ctx_temp->fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    ret = fcntl(ctx_temp->fd, F_SETFL,
        (fd_flags) & (~O_NONBLOCK));
    if (ret == -1)
        return -1;
    ret = fcntl(ctx_temp->fd, F_SETFD, FD_CLOEXEC);
    if (ret == -1)
        return -1;

    //Exchange IB information
    ret = send(ctx_temp->fd, &ctx_temp->self_ib,
        sizeof(rdma_node_ib), MSG_WAITALL);
    if (ret != sizeof(rdma_node_ib))
        return -1;

    ret = recv(ctx_temp->fd, &ctx_temp->peer_ib,
        sizeof(rdma_node_ib), MSG_WAITALL);
    if (ret != sizeof(rdma_node_ib))
        return -1;

    //Bring up the queue pair
    ret = SetQpInit(ctx_temp);
    if (ret != 0)
        return -1;

    ret = SetQpRtr(ctx_temp);
    if (ret != 0)
        return -1;

    ret = SetQpRts(ctx_temp);
    if (ret != 0)
        return -1;
    

    //TODO: Fill ctx_temp->init_attr;
    ctx_temp->init_attr.type = RdmaContextActive;
    ctx_temp->status         = RdmaContextEstWaitAck;
    ctx_temp->event.type     = RdmaEventEstablished;
    *n_ctx = ctx_temp;

    //Set the socket to be close-on-exec and blocking
    fd_flags = fcntl(ctx_temp->fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    ret = fcntl(ctx_temp->fd, F_SETFL,
        fd_flags | O_NONBLOCK);
    if (ret == -1)
        return -1;

    return 0;
}

/**
 * @API
 */

int lmRdmaClose(rdma_context * context) {

    ibv_dereg_mr(context->self_ib.oppb_mr);
    delete [] context->self_ib.ppb;

    ibv_destroy_cq(context->rcq);
    ibv_destroy_cq(context->scq);
    ibv_destroy_comp_channel(context->rcc);
    ibv_destroy_comp_channel(context->scc);
    ibv_destroy_qp(context->qp);
    close(context->fd);

    return 0;
}

int VerifyValidConnection(rdma_context * context) {

    /**
     * This tests whether the local qp is connected to the remote qp by
     * remotely read from an internal localtion defined in the rdma_context
     * (Peer-Polling-Buffer)
     */
    ibv_send_wr wr,  * bad_wr = NULL;
    ibv_sge     sge;

    memset(&wr, 0, sizeof(ibv_send_wr));

    wr.wr_id      = 0;
    wr.next       = NULL;
    wr.opcode     = IBV_WR_RDMA_READ;
    wr.sg_list    = &sge;
    wr.num_sge    = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    wr.wr.rdma.remote_addr = (uint64_t) context->peer_ib.ppb;
    wr.wr.rdma.rkey        = context->peer_ib.ppb_mr.rkey;

    sge.addr   = (uint64_t) context->self_ib.ppb;
    sge.length = 16;
    sge.lkey   = context->self_ib.ppb_mr.lkey;

    int ret = ibv_post_send(context->qp, &wr, &bad_wr);
    if (ret != 0)
        return -1;

    ibv_wc wc;
    do {
        ret = ibv_poll_cq(context->scq, 1, &wc);
    } while (ret <= 0);

    if (wc.status != IBV_WC_SUCCESS) {
        return -1;
    }

    return 0;
}

int PollConnectionPassive(rdma_context * context, rdma_event ** event,
    bool blocking) {

    //Here, we check only the status of the socket connection
    pollfd fd_set;
    int    num_fd, timeout;

    fd_set.fd     = context->fd;
    fd_set.events = POLLIN | POLLPRI | POLLOUT | POLLERR | POLLNVAL;
    timeout       = (blocking) ? -1 : 0;
    num_fd        = poll(&fd_set, 1, timeout);

    //No event
    if (num_fd <= 0)
        return -1;

    //Invalid operations and unsupported out-of-band message
    if ((fd_set.revents & POLLOUT) || (fd_set.revents & POLLPRI)
        || (fd_set.revents & POLLNVAL) || (fd_set.revents & POLLERR)) 
        return -1;

    //Incoming connection available for acceptance
    if (!(fd_set.revents & POLLIN))
        return -1;

    rdma_event * event_temp, * c_event = &context->event;

    event_temp       = new rdma_event();
    c_event->context = context;
    c_event->type    = RdmaEventConnectRequest;
    memcpy(event_temp, c_event, sizeof(rdma_event));
    *event           = event_temp;

    return 0;
}

int PollConnectionActive(rdma_context * context, rdma_event ** event,   
    bool blocking) {

    //Here, we check only the status of the IB connection
    rdma_event * event_temp, * c_event = &context->event;

    do {
        int ret = VerifyValidConnection(context);
        if (ret != 0) {
            event_temp       = new rdma_event();
            c_event->context = context;
            c_event->type    = RdmaEventDisconnected;
            memcpy(event_temp, c_event, sizeof(rdma_event));
            *event           = event_temp;
            return 0;
        }
        usleep(10000);
    } while(blocking);

    //TODO: We should check more IB statuses in the future.

    return -1;
}

int RetrieveEventFromContext(rdma_context * context, rdma_event ** event,
    int flags) {

    if (context->status == RdmaContextDisconnected)
        return -1;

    rdma_event * event_temp, * c_event = &context->event;
    int          ret;

    if (context->status == RdmaContextEstWaitAck) {
        event_temp       = new rdma_event();
        c_event->context = context;
        c_event->type    = RdmaEventEstablished;
        memcpy(event_temp, c_event, sizeof(rdma_event));
        *event           = event_temp;
        return 0;
    }

    bool blocking = (flags == 0) ? false : true;

    switch (context->init_attr.type) {
    case RdmaContextPassive:
        ret = PollConnectionPassive(context, &event_temp, blocking);
        break;
    case RdmaContextActive:
        ret = PollConnectionActive(context, &event_temp, blocking);
        break;
    default:
        return -1;
    }

    if (ret == 0)
        *event = event_temp;

    return ret;
}

/**
 * @API
 */

int lmRdmaGetEvent(rdma_context * context, rdma_event ** event) {

    return RetrieveEventFromContext(context, event, 0);
}

/**
 * @API
 */

int lmRdmaWaitEvent(rdma_context * context, rdma_event ** event) {

    return RetrieveEventFromContext(context, event, 1);
}

int RdmaAckEventPassive(rdma_event * event) {

    rdma_context * context = event->context;

    switch (event->type) {
    case RdmaEventNoEvent:
        return 0;
    case RdmaEventConnectRequest:
        context->event.type = RdmaEventNoEvent;
        return 0;
    case RdmaEventEstablished:
        if (context->status != RdmaContextEstWaitAck)
            return -1;
        context->status        = RdmaContextEstablished;
        context->event.type    = RdmaEventNoEvent;
        return 0;
    case RdmaEventRejected:
        return -1;
    case RdmaEventDisconnected:
        return -1;
    default:
        return -1;
    }

    return 0;
}

int RdmaAckEventActive(rdma_event * event) {

    rdma_context * context = event->context;

    switch(event->type) {
    case RdmaEventNoEvent:
        return 0;
    case RdmaEventConnectRequest:
        return -1;
    case RdmaEventEstablished:
        if (context->status != RdmaContextEstWaitAck)
            return -1;
        context->status     = RdmaContextEstablished;
        context->event.type = RdmaEventNoEvent;
        return 0;
    case RdmaEventRejected:
        context->status     = RdmaContextDisconnected;
        context->event.type = RdmaEventNoEvent;
        return 0;
    case RdmaEventDisconnected:
        context->status     = RdmaContextDisconnected;
        context->event.type = RdmaEventNoEvent;
    }

    return 0;
}

/**
 * @API
 */

int lmRdmaAckEvent(rdma_event * event) {

    int ret;

    switch (event->context->init_attr.type) {
    case RdmaContextPassive:
        ret = RdmaAckEventPassive(event);
        break;
    case RdmaContextActive:
        ret = RdmaAckEventActive(event);
        break;
    default:
        return -1;
    }

    return ret;
}

/**
 * @API
 */

int lmRdmaReleaseEvent(rdma_event * event) {

    delete event;

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

