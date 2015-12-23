/**
 * Author: Kittisak Sajjapngse
 * Date:   Aug/25/2015
 */

#ifndef _LMIB_H
#define _LMIB_H

#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define TEST_ACTIVE(context) do {                     \
    if (context->init_attr.type != RdmaContextActive) \
        return -1;                                    \
} while(0);


typedef enum {
    RdmaContextUnInit,
    RdmaContextPassive,
    RdmaContextActive
} rdma_context_type;

typedef struct {
    char * self;
    char * self_service;
    char * peer;
    char * peer_service;
} rdma_node_info;

typedef struct {
    rdma_context_type type;
    rdma_node_info    node;
    size_t            send_bufsize;
    size_t            recv_bufsize;
} rdma_init_attr;

typedef struct {
    uint16_t   lid;
    uint32_t   qpn;
    uint32_t   psn;
    char     * ppb; //Peer-Polling-Buffer
    ibv_mr   * oppb_mr;
    ibv_mr     ppb_mr;
} rdma_node_ib;

typedef enum {
    RdmaContextNull,
    RdmaContextEstWaitAck,
    RdmaContextEstablished,
    RdmaContextDisconnected
} rdma_context_status;

typedef enum {
    RdmaEventNoEvent,
    RdmaEventConnectRequest,
    RdmaEventEstablished,
    RdmaEventRejected,
    RdmaEventDisconnected
} rdma_event_type;

struct rdma_context_st;

typedef struct {
    rdma_context_st * context;
    rdma_event_type   type;
} rdma_event;

typedef std::vector<ibv_wc> wc_queue;
typedef wc_queue::iterator  wc_iter;
typedef struct rdma_context_st {
    //LM-IB specific attributes
    rdma_init_attr         init_attr;
    rdma_node_ib           self_ib;
    rdma_node_ib           peer_ib;
    rdma_context_status    status;
    rdma_event             event;

    //Socket properties for initiating IB connection
    int                    fd;
    sockaddr_in            sock;
    socklen_t              socklen;

    //Application-specific attributes
    ibv_device          ** dev_lst;
    ibv_device           * dev;
    int                    phy_port;
    ibv_context          * context;
    ibv_pd               * pd;

    //Process-specific attributes
    ibv_cq               * scq;
    ibv_cq               * rcq;
    ibv_comp_channel     * scc;
    ibv_comp_channel     * rcc;
    ibv_qp_init_attr       qp_init_attr;
    ibv_qp_attr            qp_attr_init;
    ibv_qp_attr            qp_attr_rtr;
    ibv_qp_attr            qp_attr_rts;
    ibv_qp               * qp;

    /**
     * Note:
     * No concurrent support for buffers and, therefore, bufferred routines.
     */
    void              * send_buf;
    size_t              send_bufsize;
    void              * recv_buf;
    size_t              recv_bufsize;
    wc_queue            rwq;
    wc_queue            swq;
    /**
     * Note:
     * wc_queue is the problem that prevents C users from using the library.
     */
} rdma_context;

typedef struct {
    uint64_t    reqId;
    enum {
        RdmaSendQueue,
        RdmaRecvQueue
    }           queue_type;
    ibv_wc      wc;
} rdma_request;

typedef struct {
    uint64_t   reqId;
    ibv_wc     wc;
    ibv_mr   * mr;
} rdma_wc;

//do we still need to expose the concept of registering memory region?
//also consult with lminet and ch.
//YES, WE NEED TO: IN ORDER TO SUPPORT ONE-SIDED COMMUNICATION


/**
 * Connection management
 */

int lmRdmaListen(rdma_context ** context, rdma_init_attr * attr);

int lmRdmaConnect(rdma_context ** context, rdma_init_attr * attr);

int lmRdmaAccept(rdma_context ** new_context, rdma_context * context);

int lmRdmaClose(rdma_context * context);

int lmRdmaGetEvent(rdma_context * context, rdma_event ** event);

int lmRdmaWaitEvent(rdma_context * context, rdma_event ** event);

int lmRdmaAckEvent(rdma_event * event);

int lmRdmaReleaseEvent(rdma_event * event);

/**
 * Send/Recv operations
 */
int lmRdmaRecvBufferred(rdma_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmRdmaSendBufferred(rdma_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmRdmaRecvBufferredAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req);

int lmRdmaSendBufferredAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req);

int lmRdmaRecvUnbuffered(rdma_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmRdmaSendUnbuffered(rdma_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmRdmaRecvUnbufferedAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req);

int lmRdmaSendUnbufferedAsync(rdma_context * context, uint64_t reqId,
    void * addr, size_t length, rdma_request * req);

int lmRdmaSendMr(rdma_context * context, uint64_t reqId,
    ibv_mr * mr);

int lmRdmaRecvMr(rdma_context * context, uint64_t reqId,
    ibv_mr ** mr);

int lmRdmaGetComp(rdma_context * context, rdma_request * req);

int lmRdmaWaitComp(rdma_context * context, rdma_request * req);

/**
 * RDMA operations
 */

int lmRdmaRegisterMemory(rdma_context * context,
    void * addr, size_t length, ibv_mr ** mr);

int lmRdmaUnregisterMemory(ibv_mr * mr);

int lmRdmaRemoteRead(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr,
    void * rem_addr, ibv_mr * rem_mr,
    size_t length);

int lmRdmaRemoteWrite(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr,
    void * rem_addr, ibv_mr * rem_mr,
    size_t length);

int lmRdmaRemoteReadAsync(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr,
    void * rem_addr, ibv_mr * rem_mr, 
    size_t length, rdma_request * req);

int lmRdmaRemoteWriteAsync(rdma_context * context, uint64_t reqId,
    void * addr, ibv_mr * mr,
    void * rem_addr, ibv_mr * rem_mr,
    size_t length, rdma_request * req);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_LMIB_H

