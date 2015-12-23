#include "ibqp.h"
#include "ibqp_ops.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

typedef struct {
    char   * buf;
    ibv_mr * mr;
    size_t   length;
} buffer;

int PostSend(ib_attributes * attr, buffer buf, uint64_t reqId) {

    ibv_send_wr wr, * bad_wr = NULL;
    ibv_sge     sge;

    memset(&wr, 0, sizeof(ibv_send_wr));

    wr.wr_id   = reqId;
    wr.next    = NULL;
    wr.opcode  = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr   = (uint64_t) buf.mr->addr;
    sge.length = buf.length;
    sge.lkey   = buf.mr->lkey;

    int ret = ibv_post_send(attr->qp, &wr, &bad_wr);
    if (ret != 0)
        return -1;

    return 0;
}

int PostRecv(ib_attributes * attr, buffer buf, uint64_t reqId) {

    ibv_recv_wr wr,  * bad_wr;
    ibv_sge     sge;

    wr.wr_id   = reqId;
    wr.next    = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr   = (uint64_t) buf.mr->addr;
    sge.length = buf.length;
    sge.lkey   = buf.mr->lkey;

    int ret = ibv_post_recv(attr->qp, &wr, &bad_wr);
    if (ret != 0)
        return -1;

    return 0;
}

int WaitSendCompletion(ib_attributes * attr, ibv_wc * wc) {

    ibv_cq * cq;
    void   * ctx = NULL;
    int      ret;
/*
    int ret = ibv_get_cq_event(attr->cc, &cq, &ctx);
    if (ret != 0)
        return -1;
*/
    do {
        ret = ibv_poll_cq(attr->scq, 1, wc);
    } while (ret == 0);
    ibv_ack_cq_events(attr->scq, 1);
    ibv_req_notify_cq(attr->scq, 0);

    if (wc[0].status != IBV_WC_SUCCESS)
        return -1;

    return 0;
}

int WaitRecvCompletion(ib_attributes * attr, ibv_wc * wc) {

    ibv_cq * cq;
    void   * ctx = NULL;
    int      ret;

    do {
        ret = ibv_poll_cq(attr->rcq, 1, wc);
    } while (ret == 0);
    ibv_ack_cq_events(attr->rcq, 1);
    ibv_req_notify_cq(attr->rcq, 0);

    if (wc[0].status != IBV_WC_SUCCESS)
        return -1;

    return 0;
}

int ServerRoutine(ib_attributes * attr) {

    buffer sendBuf, recvBuf;
    ibv_wc wc[10];
    memset(wc, 0, 10 * sizeof(ibv_wc));

    sendBuf.buf = new char [DEFAULT_SEND_BUFSIZE];
    recvBuf.buf = new char [DEFAULT_RECV_BUFSIZE];
    sendBuf.length = DEFAULT_SEND_BUFSIZE;
    recvBuf.length = DEFAULT_RECV_BUFSIZE;
    memset(sendBuf.buf, 0, sendBuf.length);
    memset(recvBuf.buf, 0, recvBuf.length);
    sprintf(sendBuf.buf, "Hello, this is the server.");
    
    sendBuf.mr = ibv_reg_mr(attr->pd, sendBuf.buf, sendBuf.length,
        IBV_ACCESS_LOCAL_WRITE);
    if (sendBuf.mr == NULL)
        return -1;

    recvBuf.mr = ibv_reg_mr(attr->pd, recvBuf.buf, recvBuf.length,
        IBV_ACCESS_LOCAL_WRITE);
    if (recvBuf.mr == NULL)
        return -1;

    for (int idx=0;idx<1000;idx++) {

    int ret = PostSend(attr, sendBuf, 1);
    if (ret != 0)
        return -1;

    ret = WaitSendCompletion(attr, wc);
    if (ret != 0)
        return -1;

    ret = PostRecv(attr, recvBuf, 4);
    if (ret != 0)
        return -1;

    ret = WaitRecvCompletion(attr, wc);
    if (ret != 0)
        return -1;
    cout << "Idx: " << idx << endl;
    }

    cout << "MSG: " << recvBuf.buf << endl;
    cout << "Complete: Server" << endl;

    return 0;
}

int ClientRoutine(ib_attributes * attr) {

    buffer sendBuf, recvBuf;
    ibv_wc wc[10];
    memset(wc, 0, 10 * sizeof(ibv_wc));

    sendBuf.buf = new char [DEFAULT_SEND_BUFSIZE];
    recvBuf.buf = new char [DEFAULT_RECV_BUFSIZE];
    sendBuf.length = DEFAULT_SEND_BUFSIZE;
    recvBuf.length = DEFAULT_RECV_BUFSIZE;
    memset(sendBuf.buf, 0, sendBuf.length);
    memset(recvBuf.buf, 0, recvBuf.length);
    sprintf(sendBuf.buf, "Client acknowledges.");

    sendBuf.mr = ibv_reg_mr(attr->pd, sendBuf.buf, sendBuf.length,
        IBV_ACCESS_LOCAL_WRITE);
    if (sendBuf.mr == NULL)
        return -1;

    recvBuf.mr = ibv_reg_mr(attr->pd, recvBuf.buf, recvBuf.length,
        IBV_ACCESS_LOCAL_WRITE);
    if (recvBuf.mr == NULL)
        return -1;

    for (int idx=0;idx<1000;idx++) {
    int ret = PostRecv(attr, recvBuf, 2);
    if (ret != 0)
        return -1;

    ret = WaitRecvCompletion(attr, wc);
    if (ret != 0)
        return -1;

    ret = PostSend(attr, sendBuf, 3);
    if (ret != 0)
        return -1;

    ret = WaitSendCompletion(attr, wc);
    if (ret != 0)
        return -1;
    cout << "Idx: " << idx << endl;
    }

    cout << "MSG: " << recvBuf.buf << endl;
    cout << "Complete: Client" << endl;

    return 0;
}

