#include "ibqp.h"
#include "ibqp_ops.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

using namespace std;

#if 0
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
#endif

ib_attributes  ib_attr;
eth_attributes eth_attr;

/**
 * (1)
 */

int InitLocalConnection(ib_attributes * attr) {

    attr->dev_list = ibv_get_device_list(NULL);
    if (attr->dev_list == NULL)
        return -1;

    attr->dev      = attr->dev_list[0];
    if (attr->dev == NULL)
        return -1;

    attr->phy_port  = 1;

    attr->context  = ibv_open_device(attr->dev);
    if (attr->context == NULL)
        return -1;

    attr->pd       = ibv_alloc_pd(attr->context);
    if (attr->pd == NULL)
        return -1;

    attr->cc       = ibv_create_comp_channel(attr->context);
    if (attr->cc == NULL)
        return -1;

    attr->rcq      = ibv_create_cq(attr->context, 10, NULL, attr->cc, 0);
    if (attr->rcq == NULL)
        return -1;

    attr->scq      = ibv_create_cq(attr->context, 10, NULL, attr->cc, 0);
    if (attr->scq == NULL)
        return -1;

    attr->qp_init_attr.send_cq             = attr->scq;
    attr->qp_init_attr.recv_cq             = attr->rcq;
    attr->qp_init_attr.qp_type             = IBV_QPT_RC;
    attr->qp_init_attr.cap.max_send_wr     = 10;
    attr->qp_init_attr.cap.max_recv_wr     = 10;
    attr->qp_init_attr.cap.max_send_sge    = 1;
    attr->qp_init_attr.cap.max_recv_sge    = 1;
    attr->qp_init_attr.cap.max_inline_data = 0;

    attr->qp = ibv_create_qp(attr->pd, &attr->qp_init_attr);
    if (attr->qp == NULL)
        return -1;

    return 0;
}

/**
 * (2)
 */

int SetQpInit(ib_attributes * attr) {

    memset(&attr->qp_init, 0, sizeof(ibv_qp_init_attr));

    attr->qp_init.qp_state        = IBV_QPS_INIT;
    attr->qp_init.pkey_index      = 0;
    attr->qp_init.port_num        = attr->phy_port;
    attr->qp_init.qp_access_flags = IBV_ACCESS_REMOTE_WRITE;

    int ret = ibv_modify_qp(attr->qp, &attr->qp_init,
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

    return 0;
}

/**
 * (3)
 */

int _EstablishConnection_ExchangeInfo(eth_attributes * attr);

int EstablishConnection(ib_attributes * ib_attr, eth_attributes * eth_attr) {

    struct ibv_port_attr ib_port_attr;
    int ret = ibv_query_port(
        ib_attr->context, ib_attr->phy_port, &ib_port_attr);
    if (ret != 0)
        return -1;

    eth_attr->ib_self.lid = ib_port_attr.lid;
    eth_attr->ib_self.qpn = ib_attr->qp->qp_num;
    eth_attr->ib_self.psn = 0x123456;

    ret = _EstablishConnection_ExchangeInfo(eth_attr);
    if (ret != 0)
        return -1;

    return 0;
}

/**
 * (4)
 */

int SetQpRtr(ib_attributes * ib_attr, eth_attributes * eth_attr) {

    memset(&ib_attr->qp_rtr, 0, sizeof(ibv_qp_attr));
    ib_attr->qp_rtr.qp_state              = IBV_QPS_RTR;
    ib_attr->qp_rtr.path_mtu              = IBV_MTU_2048;
    ib_attr->qp_rtr.dest_qp_num           = eth_attr->ib_peer.qpn;
    ib_attr->qp_rtr.rq_psn                = eth_attr->ib_peer.psn;
    ib_attr->qp_rtr.max_dest_rd_atomic    = 1;
    ib_attr->qp_rtr.min_rnr_timer         = 12;
    ib_attr->qp_rtr.ah_attr.is_global     = 0;
    ib_attr->qp_rtr.ah_attr.dlid          = eth_attr->ib_peer.lid;
    ib_attr->qp_rtr.ah_attr.sl            = 1;
    ib_attr->qp_rtr.ah_attr.src_path_bits = 0;
    ib_attr->qp_rtr.ah_attr.port_num      = ib_attr->phy_port;

    int ret = ibv_modify_qp(ib_attr->qp, &ib_attr->qp_rtr,
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

int SetQpRts(ib_attributes * ib_attr, eth_attributes * eth_attr) {

    memset(&ib_attr->qp_rts, 0, sizeof(ibv_qp_attr));

    ib_attr->qp_rts.qp_state      = IBV_QPS_RTS;
    ib_attr->qp_rts.timeout       = 14;
    ib_attr->qp_rts.retry_cnt     = 7;
    ib_attr->qp_rts.rnr_retry     = 7;
    ib_attr->qp_rts.sq_psn        = eth_attr->ib_self.psn;
    ib_attr->qp_rts.max_rd_atomic = 1;

    int ret = ibv_modify_qp(ib_attr->qp, &ib_attr->qp_rts,
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

int main(int argc, char ** argv) {

    if (argc == 1) {
        eth_attr.type = eth_attributes::server;
        eth_attr.port = 65201;
    }
    else {
        eth_attr.type = eth_attributes::client;
        strcpy(eth_attr.peer, argv[1]);
        eth_attr.port = 65201;
    }

    cout << endl;
    cout << "Mode: "; 
    switch (eth_attr.type) {
    case eth_attributes::server: 
        cout << "Server" << endl; 
        break;
    case eth_attributes::client: 
        cout << "Client" << endl; 
        cout << "Peer: " << eth_attr.peer << endl;
        break;
    }
    cout << "Port: " << eth_attr.port << endl;
    cout << endl;

    int ret = InitLocalConnection(&ib_attr);
    if (ret != 0) {
        cerr << "Error: Cannot initialize IB connection." << endl;
        exit(-1);
    }

    ret = SetQpInit(&ib_attr);
    if (ret != 0) {
        cerr << "Error: Cannot transfer QP to init state." << endl;
        exit(-1);
    }

    ret = EstablishConnection(&ib_attr, &eth_attr);
    if (ret != 0) {
        cerr << "Error: Cannot set local connection." << endl;
        exit(-1);
    }

    ret = SetQpRtr(&ib_attr, &eth_attr);
    if (ret != 0) {
        cerr << "Error: Cannot set QP state to RTR." << endl;
        exit(-1);
    }

    ret = SetQpRts(&ib_attr, &eth_attr);
    if (ret != 0) {
        cerr << "Error: Cannot set QP state to RTS." << endl;
        exit(-1);
    }

    switch(eth_attr.type) {
    case eth_attributes::server:
        ServerRoutine(&ib_attr);
        break;
    case eth_attributes::client:
        ClientRoutine(&ib_attr);
        break;
    }

    return 0;
}

int _EstablishConnection_Server(eth_attributes * attr) {

    int           fd, ret;
    sockaddr_in   sock;

cout << "SERVER-0" << endl;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return -1;
    int so_reuseaddr_val = 1;
    setsockopt(fd, SOL_SOCKET,
        SO_REUSEADDR, &so_reuseaddr_val, sizeof(int));

    sock.sin_family      = AF_INET;
    sock.sin_addr.s_addr = htonl(INADDR_ANY);
    sock.sin_port        = htons(attr->port);

cout << "SERVER-1" << endl;
    ret = bind(fd, (sockaddr *) &sock, sizeof(sockaddr_in));
    if (ret == -1)
        return -1;

cout << "SERVER-2" << endl;
    ret = listen(fd, 10);
    if (ret == -1)
        return -1;

cout << "SERVER-3" << endl;
    sockaddr_in  client_sock;
    socklen_t    client_socklen;
    int client_fd = accept(fd, (sockaddr *) &client_sock, &client_socklen);
    if (client_fd == -1)
        return -1;

cout << "SERVER-4" << endl;
    ret = send(client_fd, &attr->ib_self, sizeof(ib_peer_info), MSG_WAITALL);
    if (ret != sizeof(ib_peer_info))
        return -1;

cout << "SERVER-5" << endl;
    ret = recv(client_fd, &attr->ib_peer, sizeof(ib_peer_info), MSG_WAITALL);
    if (ret != sizeof(ib_peer_info))
        return -1;

cout << "SERVER-6" << endl;
    usleep(50000);
    close(client_fd);
    close(fd);
    cout << "EXCHANGE COMPLETE" << endl;

    return 0;
}

int _EstablishConnection_Client(eth_attributes * attr) {

    int           fd, ret;
    sockaddr_in   sock;
    addrinfo    * addr, addr_hints;

cout << "\tCLIENT-0" << endl;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return -1;
    int so_reuseaddr_val = 1;
    setsockopt(fd, SOL_SOCKET,
        SO_REUSEADDR, &so_reuseaddr_val, sizeof(int));

    addr_hints.ai_family    = AF_INET;
    addr_hints.ai_socktype  = SOCK_STREAM;
 

cout << "\tCLIENT-1" << endl;
    stringstream port_str;
    port_str << attr->port;   
    ret = getaddrinfo(attr->peer, port_str.str().c_str(), NULL, &addr);
    if (ret == -1)
        return -1;
    if (addr->ai_family != AF_INET)
        return -1;
    memcpy(&sock, addr->ai_addr, addr->ai_addrlen);

    cout << "CONNECTING" << endl;

cout << "\tCLIENT-2" << endl;
    ret = connect(fd, (sockaddr *) &sock, addr->ai_addrlen);
    if( ret == -1)
        return -1;

    cout << "CONNECTED" << endl;

cout << "\tCLIENT-3" << endl;
    ret = recv(fd, &attr->ib_peer, sizeof(ib_peer_info), MSG_WAITALL);
    if (ret != sizeof(ib_peer_info))
        return -1;

cout << "\tCLIENT-4" << endl;
    ret = send(fd, &attr->ib_self, sizeof(ib_peer_info), MSG_WAITALL);
    if (ret != sizeof(ib_peer_info))
        return -1;

    usleep(50000);
    close(fd);
    cout << "EXCHANGE COMPLETE" << endl;

    return 0;
}

int _EstablishConnection_ExchangeInfo(eth_attributes * attr) {

    int ret;
    switch(attr->type) {
    case eth_attributes::server:
        ret = _EstablishConnection_Server(attr);
        break;
    case eth_attributes::client:
        ret = _EstablishConnection_Client(attr);
        break;
    }
    if (ret != 0)
        return -1;

    cout << " ===SELF=== " << endl;
    cout << "LID: " << attr->ib_self.lid << endl;
    cout << "QPN: " << hex << "0x" << attr->ib_self.qpn << dec << endl;
    cout << "PSN: " << hex << "0x" << attr->ib_self.psn << dec << endl;
    cout << " ===PEER=== " << endl;
    cout << "LID: " << attr->ib_peer.lid << endl;
    cout << "QPN: " << hex << "0x" << attr->ib_peer.qpn << dec << endl;
    cout << "PSN: " << hex << "0x" << attr->ib_peer.psn << dec << endl;

    return 0;
}


