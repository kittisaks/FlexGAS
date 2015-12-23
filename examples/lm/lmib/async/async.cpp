#include "lm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>

using namespace std;

#define DEFAULT_PORT "65201"
#define MAX_ITER 10000

int Server(int argc, char ** argv);

int Client(int argc, char ** argv);

int main(int argc, char ** argv) {

    if (argc == 1) {
        int ret = Server(argc, argv);
        if (ret != 0)
            cerr << "Server Error" << endl;
    }
    else {
        int ret = Client(argc, argv);
        if (ret != 0)
            cerr << "Client Error" << endl;
    }

    return 0;
}

#define CHECK(str) do {  \
    int ret = str;       \
    if (ret != 0)        \
        return -1;       \
} while (0);

#define IS_SUCCESS(str) do {                 \
    if (str.wc.status == IBV_WC_SUCCESS)     \
        cout << "SUCCESS" << endl;           \
    else                                     \
        cout << "FAILED"  << endl;           \
} while(0)

int Server(int argc, char ** argv) {

    rdma_init_attr   init_attr;
    rdma_context   * context;
    rdma_context   * client_context;

    init_attr.node.self_service = (char *) DEFAULT_PORT;

    int ret;
    ret = lmRdmaListen(&context, &init_attr);
    if (ret != 0) {
        cerr << "Cannot start listening." << endl;
        return -1;
    }

    ret = lmRdmaAccept(&client_context, context);
    if (ret != 0) {
        cerr << "Cannot accept client." << endl;
        return -1;
    }

    cout << endl;
    cout << "REMOTE IB INFOMATION" << endl;
    cout << "LID: " << client_context->peer_ib.lid << endl;
    cout << "QPN: " << client_context->peer_ib.qpn << endl;
    cout << "PSN: " << client_context->peer_ib.psn << endl;
    cout << endl;

    char   ** sbuf = new char * [2];
    char   ** rbuf = new char * [2];
    sbuf[0] = new char [5000];
    sbuf[1] = new char [5000];
    rbuf[0] = new char [5000];
    rbuf[1] = new char [5000];

#if 0
    ibv_mr ** smr  = new ibv_mr * [2];
    CHECK(    lmRdmaRegisterMemory(client_context,
        sbuf[0], 1000000000, &smr[0]));
    CHECK(    lmRdmaRegisterMemory(client_context,
        sbuf[1], 1000000000, &smr[1]));
#endif

    rdma_request req[4];
    CHECK(    lmRdmaSendUnbufferedAsync(client_context, 0,
        sbuf[0], 5000, &req[0]));
    CHECK(    lmRdmaSendUnbufferedAsync(client_context, 1,
        sbuf[1], 5000, &req[1]));
    CHECK(    lmRdmaRecvUnbufferedAsync(client_context, 2,
        rbuf[0], 5000, &req[2]));
    CHECK(    lmRdmaRecvUnbufferedAsync(client_context, 3,
        rbuf[1], 5000, &req[3]));

    CHECK(    lmRdmaWaitComp(client_context, &req[1]));
    CHECK(    lmRdmaWaitComp(client_context, &req[0]));
    CHECK(    lmRdmaWaitComp(client_context, &req[3]));
    CHECK(    lmRdmaWaitComp(client_context, &req[2]));
    IS_SUCCESS(req[0]);
    IS_SUCCESS(req[1]);
    IS_SUCCESS(req[2]);
    IS_SUCCESS(req[3]);
    
    return 0;
}

int Client(int argc, char ** argv) {

    rdma_init_attr   init_attr;
    rdma_context   * context;

    init_attr.node.peer = argv[1];
    init_attr.node.peer_service = (char *) DEFAULT_PORT;

    int ret;
    ret = lmRdmaConnect(&context, &init_attr);
    if (ret != 0) {
        cerr << "Cannot connect to " << argv[1] << endl;
        return -1; 
    }

    cout << endl;
    cout << "REMOTE IB INFORMATION" << endl;
    cout << "LID: " << context->peer_ib.lid << endl;
    cout << "QPN: " << context->peer_ib.qpn << endl;
    cout << "PSN: " << context->peer_ib.psn << endl;
    cout << endl;

    char   ** sbuf = new char * [2];
    char   ** rbuf = new char * [2];
    sbuf[0] = new char [5000];
    sbuf[1] = new char [5000];
    rbuf[0] = new char [5000];
    rbuf[1] = new char [5000];

#if 0
    ibv_mr ** rmr  = new ibv_mr * [2];
    CHECK(    lmRdmaRegisterMemory(context,
        rbuf[0], 1000000000, &rmr[0]));
    CHECK(    lmRdmaRegisterMemory(context,
        rbuf[1], 1000000000, &rmr[1]));
#endif
    
    rdma_request req[4];
    CHECK(    lmRdmaSendUnbufferedAsync(context, 0,
        sbuf[0], 5000, &req[0]));
    CHECK(    lmRdmaSendUnbufferedAsync(context, 1,
        sbuf[1], 5000, &req[1]));
    CHECK(    lmRdmaRecvUnbufferedAsync(context, 2,
        rbuf[0], 5000, &req[2]));
    CHECK(    lmRdmaRecvUnbufferedAsync(context, 3,
        rbuf[1], 5000, &req[3]));

    CHECK(    lmRdmaWaitComp(context, &req[1]));
    CHECK(    lmRdmaWaitComp(context, &req[0]));
    CHECK(    lmRdmaWaitComp(context, &req[3]));
    CHECK(    lmRdmaWaitComp(context, &req[2]));
    IS_SUCCESS(req[0]);
    IS_SUCCESS(req[1]);
    IS_SUCCESS(req[2]);
    IS_SUCCESS(req[3]);

    return 0;
}

