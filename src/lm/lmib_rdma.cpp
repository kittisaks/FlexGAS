#include "lmib.h"
#include <string.h>

#include <iostream>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int PostRdmaWrite(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr, void * rem_addr, ibv_mr * rem_mr,
    size_t length) {

    ibv_send_wr wr,  * bad_wr = NULL;
    ibv_sge     sge;

    memset(&wr, 0, sizeof(ibv_send_wr));

    wr.wr_id      = reqId;
    wr.next       = NULL;
    wr.opcode     = IBV_WR_RDMA_WRITE;
    wr.sg_list    = &sge;
    wr.num_sge    = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    /* TODO: wr.wr.rdma.remote_addr will eventually be changed to rem_addr */
    wr.wr.rdma.remote_addr = (uint64_t) rem_addr; //rem_mr->addr;
    wr.wr.rdma.rkey        = rem_mr->rkey;

    /* TODO: sge.addr will eventually be changed to addr */
    sge.addr   = (uint64_t) addr; //mr->addr;
    sge.length = length;
    sge.lkey   = mr->lkey;

    int ret = ibv_post_send(context->qp, &wr, &bad_wr);
    if (ret != 0)
        return -1;

    return 0;
}

int PostRdmaRead(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr, void * rem_addr, ibv_mr * rem_mr,
    size_t length) {

    ibv_send_wr wr,  * bad_wr = NULL;
    ibv_sge     sge;

    memset(&wr, 0, sizeof(ibv_send_wr));

    wr.wr_id      = reqId;
    wr.next       = NULL;
    wr.opcode     = IBV_WR_RDMA_READ;
    wr.sg_list    = &sge;
    wr.num_sge    = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    /* TODO: wr.wr.rdma.remote_addr will eventually be changed to rem_addr */
    wr.wr.rdma.remote_addr = (uint64_t) rem_addr; //rem_mr->addr;
    wr.wr.rdma.rkey        = rem_mr->rkey;

    /* TODO: sge.addr will eventually be changed to addr */
    sge.addr   = (uint64_t) addr; //mr->addr;
    sge.length = length;
    sge.lkey   = mr->lkey;

    int ret = ibv_post_send(context->qp, &wr, &bad_wr);
    if (ret != 0)
        return -1;

    return 0;
}


int lmRdmaRegisterMemory(rdma_context * context,
    void * addr, size_t length, ibv_mr ** mr) {

    TEST_ACTIVE(context); 

    ibv_mr * mr_temp = ibv_reg_mr(context->pd, addr,
        length, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ);
    if (mr_temp == NULL)
        return -1;

    *mr = mr_temp;

    return 0;
}

int lmRdmaUnregisterMemory(ibv_mr * mr) {

    int ret = ibv_dereg_mr(mr);
    if (ret != 0)
        return -1;

    return 0;
}

int lmRdmaRemoteRead(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr,
    void * rem_addr, ibv_mr * rem_mr, 
    size_t length) {

    int ret = PostRdmaRead(context, reqId, addr, mr, rem_addr, rem_mr, length);
    if (ret != 0)
        return -1;

    ibv_wc wc;
    do {
        ret = ibv_poll_cq(context->scq, 1, &wc);
    } while (ret <= 0);

    if (wc.status != IBV_WC_SUCCESS) {
        cout << "Error: " << wc.status << endl;
        return -1;
    }

    return 0;
}

int lmRdmaRemoteWrite(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr,
    void * rem_addr, ibv_mr * rem_mr,
    size_t length) {

    int ret = PostRdmaWrite(context, reqId, addr, mr, rem_addr, rem_mr, length);
    if (ret != 0)
        return -1;

    ibv_wc wc;
    do {
        ret = ibv_poll_cq(context->scq, 1, &wc);
    } while (ret <= 0);

    if (wc.status != IBV_WC_SUCCESS) {
        cout << "Error: " << wc.status << endl;
        return -1;
    }

    return 0;
}

int lmRdmaRemoteReadAsync(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr, 
    void * rem_addr, ibv_mr * rem_mr, 
    size_t length, rdma_request * req) {

    int ret = PostRdmaRead(context, reqId, addr, mr, rem_addr, rem_mr, length);
    if (ret != 0)
        return -1;

    req->reqId      = reqId;
    req->queue_type = rdma_request::RdmaSendQueue;

    return 0;
}

int lmRdmaRemoteWriteAsync(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr,
    void * rem_addr, ibv_mr * rem_mr,
    size_t length, rdma_request * req) {

    int ret =PostRdmaWrite(context, reqId, addr, mr, rem_addr, rem_mr, length);
    if (ret != 0)
        return -1;

    req->reqId      = reqId;
    req->queue_type = rdma_request::RdmaSendQueue;

    return 0;
}

#ifdef __cplusplus
}
#endif //__cplusplus

