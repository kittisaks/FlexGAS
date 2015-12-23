#include "lmib.h"
#include <string.h>
#include <unistd.h>

#include <iostream>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int RdmaPostSend(rdma_context * context, uint64_t reqId,
    ibv_mr * mr, size_t length) {

    ibv_send_wr wr, * bad_wr = NULL;
    ibv_sge     sge;

    memset(&wr, 0, sizeof(ibv_send_wr));

    wr.wr_id   = reqId;
    wr.next    = NULL;
    wr.opcode  = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr   = (uint64_t) mr->addr;
    sge.length = length;
    sge.lkey   = mr->lkey;

    int ret = ibv_post_send(context->qp, &wr, &bad_wr);
    if (ret != 0)
        return -1;

    return 0;
}

int RdmaPostRecv(rdma_context * context, uint64_t reqId,
    ibv_mr * mr, size_t length) {

    ibv_recv_wr wr,  * bad_wr;
    ibv_sge     sge;

    wr.wr_id   = reqId;
    wr.next    = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr   = (uint64_t) mr->addr;
    sge.length = length;
    sge.lkey   = mr->lkey;

    int ret = ibv_post_recv(context->qp, &wr, &bad_wr);
    if (ret != 0)
        return -1;

    return 0;
}

/**
 * @API
 */
int lmRdmaRecvBufferred(rdma_context * context, uint64_t reqId,
    void * addr, size_t length) {

    return 0;
}

/**
 * @API
 */
int lmRdmaSendBufferred(rdma_context * context, uint64_t reqId,
    void * addr, size_t length) {

    return 0;
}

/**
 * @API
 */
int lmRdmaRecvBufferredAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req) {

    return 0;
}

/**
 * @API
 */
int lmRdmaSendBufferredAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req) {

    return 0;
}

/**
 * @API
 */
#include <iostream>
using namespace std;
#if 0
int lmRdmaRecvUnbuffered(rdma_context * context, uint64_t reqId,
    void * addr, size_t length) {

    ibv_mr * mr;
    ibv_wc   wc;
    int      ret;

    TEST_ACTIVE(context);

    mr = ibv_reg_mr(context->pd, addr, length, IBV_ACCESS_LOCAL_WRITE);
    if (mr == NULL) {
        cout << "ERROR 1" << endl;
        return -1;
    }
    ret = RdmaPostRecv(context, reqId, mr, length);
    if (ret != 0) {
        cout << "ERROR 2" << endl;
        return -1;
    }

    do {
        ret = ibv_poll_cq(context->rcq, 1, &wc);
    } while (ret <= 0);

    if (wc.status != IBV_WC_SUCCESS) {
        cout << "ERROR 3" << endl;
        cout << "\tibv_wc_status: " << wc.status << "/" 
            << IBV_WC_WR_FLUSH_ERR << endl;
        return -1;
    }

    ret = ibv_dereg_mr(mr);
    if (ret != 0) {
        cout << "ERROR 4" << endl;
        return -1;
    }

    return 0;
}
#else
int lmRdmaRecvUnbuffered(rdma_context * context, uint64_t reqId,
    void * addr, size_t length) {

    int ret;

    TEST_ACTIVE(context);

    int recv_bytes = 0;
    do {
        ret = recv(context->fd, ((char *) addr) + recv_bytes, length - recv_bytes, 0);
        if ((ret == -1) && (errno != EAGAIN)) {
            cout << "recv error ??? " << errno << endl;
            return -1;
        }
        else if (ret > 0)
            recv_bytes += ret;
    } while(recv_bytes < (int) length);

    return 0;
}
#endif

/**
 * @API
 */
#if 0
int lmRdmaSendUnbuffered(rdma_context * context, uint64_t reqId,
    void * addr, size_t length) {

    ibv_mr * mr;
    ibv_wc   wc;
    int      ret;

    TEST_ACTIVE(context);

    mr = ibv_reg_mr(context->pd, addr, length, IBV_ACCESS_LOCAL_WRITE);
    if (mr == NULL)
        return -1;
    ret = RdmaPostSend(context, reqId, mr, length);
    if (ret != 0)
        return -1;

    do {
        ret = ibv_poll_cq(context->scq, 1, &wc);
    } while (ret <= 0);

    if (wc.status != IBV_WC_SUCCESS)
        return -1;

    ret = ibv_dereg_mr(mr);
    if (ret != 0)
        return -1;

    return 0;
}
#else
int lmRdmaSendUnbuffered(rdma_context * context, uint64_t reqId,
    void * addr, size_t length) {

    int ret;

    TEST_ACTIVE(context);

    int sent_bytes = 0;
    do {
        ret = send(context->fd, ((char *) addr + sent_bytes), length - sent_bytes, 0);
        if (ret == -1) {
            cout << "send error ???" << endl;
            return -1;
        }
        else if (ret > 0)
            sent_bytes += ret;
    } while (ret < (int) length); 

    return 0;
}
#endif

/**
 * @API
 */
int lmRdmaRecvUnbufferedAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req) {

    ibv_mr * mr;
    int      ret;

    TEST_ACTIVE(context);

    mr = ibv_reg_mr(context->pd, addr, length, IBV_ACCESS_LOCAL_WRITE);
    if (mr == NULL)
        return -1;
    ret = RdmaPostRecv(context, reqId, mr, length);
    if (ret != 0)
        return -1;

    req->reqId      = reqId;
    req->queue_type = rdma_request::RdmaRecvQueue;

    return 0;
}

/**
 * @API
 */
int lmRdmaSendUnbufferedAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req) {

    ibv_mr * mr;
    int      ret;

    TEST_ACTIVE(context);

    mr = ibv_reg_mr(context->pd, addr, length, IBV_ACCESS_LOCAL_WRITE);
    if (mr == NULL)
        return -1;
    ret = RdmaPostSend(context, reqId, mr, length);
    if (ret != 0)
        return -1;

    req->reqId      = reqId;
    req->queue_type = rdma_request::RdmaSendQueue;

    return 0;
}

/**
 * @API
 */
int lmRdmaSendMr(rdma_context * context, uint64_t reqId,
    ibv_mr * mr) {

    ibv_mr * mr_mr;
    ibv_wc   wc;
    int      ret;

    TEST_ACTIVE(context);

    mr_mr = ibv_reg_mr(context->pd, mr, sizeof(ibv_mr), IBV_ACCESS_LOCAL_WRITE);
    if (mr_mr == NULL)
        return -1;
    ret = RdmaPostSend(context, reqId, mr_mr, sizeof(ibv_mr));
    if (ret != 0)
        return -1;

    do {
        ret = ibv_poll_cq(context->scq, 1, &wc);
    } while (ret <= 0);

    if (wc.status != IBV_WC_SUCCESS)
        return -1;

    ret = ibv_dereg_mr(mr_mr);
    if (ret != 0)
        return -1;

    return 0;
}

/**
 * @API
 */
int lmRdmaRecvMr(rdma_context * context, uint64_t reqId,
    ibv_mr ** mr) {

    ibv_mr * mr_mr, * mr_temp;
    ibv_wc   wc;
    int      ret;

    TEST_ACTIVE(context);

    mr_temp = new ibv_mr();
    mr_mr = ibv_reg_mr(context->pd, mr_temp,
        sizeof(ibv_mr), IBV_ACCESS_LOCAL_WRITE);
    if (mr == NULL)
        return -1;
    ret = RdmaPostRecv(context, reqId, mr_mr, sizeof(ibv_mr));
    if (ret != 0)
        return -1;

    do {
        ret = ibv_poll_cq(context->rcq, 1, &wc);
    } while (ret <= 0);

    if (wc.status != IBV_WC_SUCCESS)
        return -1;

    ret = ibv_dereg_mr(mr_mr);
    if (ret != 0)
        return -1;

    *mr = mr_temp;

    return 0;
}

/**
 * @API
 */
int lmRdmaGetComp(rdma_context * context, rdma_request * req) {

    /**
     * Note: This function is not thread-safe. Since the LM-IB layer assumes
     *       threads to be associated with different rdma_context.
     */

    TEST_ACTIVE(context);

    wc_queue * wq;
    ibv_cq   * cq;

    if (req->queue_type == rdma_request::RdmaSendQueue) {
        wq = &context->swq;
        cq = context->scq;
    }
    else if (req->queue_type == rdma_request::RdmaRecvQueue) {
        wq = &context->rwq;
        cq = context->rcq;
    }
    else
        return -1;

    for (wc_iter idx=wq->begin();idx!=wq->end();++idx) {
        if ((*idx).wr_id == req->reqId) {
            req->wc = *idx;
            wq->erase(idx);
            return 0;
        }
    }

    ibv_wc wc;

    int ret = ibv_poll_cq(cq, 1, &wc);
    if (ret <= 0)
        return -1;

    if (wc.wr_id != req->reqId) {
        wq->push_back(wc);
        return -1;
    }

    req->wc = wc;

    return 0;
}

/**
 * @API
 */
int lmRdmaWaitComp(rdma_context * context, rdma_request * req) {

    int ret;
    do {
        ret = lmRdmaGetComp(context, req);
        usleep(100);
    } while (ret != 0);

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

