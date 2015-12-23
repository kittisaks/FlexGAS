#ifndef _IBQP_H
#define _IBQP_H

#include <infiniband/verbs.h>
#define DEFAULT_SEND_BUFSIZE 4096
#define DEFAULT_RECV_BUFSIZE 4096

typedef struct {
    ibv_device       ** dev_list;
    ibv_device       *  dev;
    int                 phy_port;
    ibv_context      *  context;
    ibv_pd           *  pd;
    ibv_cq           *  rcq;
    ibv_cq           *  scq;
    ibv_qp_init_attr    qp_init_attr;
    ibv_qp_attr         qp_init;
    ibv_qp_attr         qp_rtr;
    ibv_qp_attr         qp_rts;
    ibv_qp           *  qp;
    ibv_comp_channel *  cc;
} ib_attributes;

typedef struct {
    uint16_t lid;
    uint32_t qpn;
    uint32_t psn;
} ib_peer_info;

typedef struct {
    enum type_en{
        server, client
    }            type;
    char         peer[256];
    int          port;
    ib_peer_info ib_self;
    ib_peer_info ib_peer;
} eth_attributes;

#endif //_IBQP_H

