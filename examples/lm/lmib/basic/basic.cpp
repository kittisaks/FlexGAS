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

#define CHECK(str) do { \
    int ret = str;       \
    if (ret != 0)        \
        return -1;       \
} while (0);

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

    char * sendbuf = new char [2048];
    char * recvbuf = new char [2048];
    char * rdbuf   = new char [2048];
    char * wrbuf   = new char [2048];
    memset(sendbuf, 0, 2048);
    memset(recvbuf, 0, 2048);
    memset(rdbuf, 0, 2048);
    memset(wrbuf, 0, 2048);

    ibv_mr * rdmr, * wrmr, * rem_rdmr, * rem_wrmr;
    ret = lmRdmaRegisterMemory(client_context, rdbuf, 2048, &rdmr);
    if (ret != 0)
        return -1;

    ret = lmRdmaRegisterMemory(client_context, wrbuf, 2048, &wrmr);
    if (ret != 0)
        return -1;

    char hostname[64];
    gethostname(hostname, 64);
    sprintf(sendbuf, "Hello client, this is the server: %s", hostname );
    sprintf(wrbuf, "Server write: %s", hostname);

    timeval s_tv, e_tv;
    gettimeofday(&s_tv, NULL);
    for (int idx=0;idx<MAX_ITER;idx++) {
        int base = idx * 7;
        CHECK(    lmRdmaSendUnbuffered(client_context,
            base + 0, sendbuf, 2048));
        CHECK(    lmRdmaSendMr(client_context, base + 1, rdmr));
        CHECK(    lmRdmaSendMr(client_context, base + 2, wrmr));
        CHECK(    lmRdmaRecvMr(client_context, base + 3, &rem_rdmr));
        CHECK(    lmRdmaRecvMr(client_context, base + 4, &rem_wrmr));
        CHECK(    lmRdmaRemoteRead(client_context, base + 5, rdbuf, rdmr,
            rem_rdmr->addr, rem_rdmr, 2048));
        CHECK(    lmRdmaRemoteWrite(client_context, base + 6, wrbuf, wrmr,
            rem_wrmr->addr, rem_wrmr, 2048));
    }
    gettimeofday(&e_tv, NULL);
    double s_time = ((double) s_tv.tv_sec) + (((double) s_tv.tv_usec) / 1e+6);
    double e_time = ((double) e_tv.tv_sec) + (((double) e_tv.tv_usec) / 1e+6);

    cout << "MSG: " << rdbuf << endl;
    cout << "Time: " << e_time - s_time << endl;
    lmRdmaClose(client_context);
    
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

    char * sendbuf = new char [2048];
    char * recvbuf = new char [2048];
    char * rdbuf   = new char [2048];
    char * wrbuf   = new char [2048];
    memset(sendbuf, 0, 2048);
    memset(recvbuf, 0, 2048);
    memset(rdbuf, 0, 2048);
    memset(wrbuf, 0, 2048);

    char hostname[64];
    gethostname(hostname, 64);
    sprintf(rdbuf, "Client: %s", hostname);

    ibv_mr * rdmr, * wrmr, * rem_rdmr, * rem_wrmr;
    CHECK(    lmRdmaRegisterMemory(context, rdbuf, 2048, &rdmr));
    CHECK(    lmRdmaRegisterMemory(context, wrbuf, 2048, &wrmr));

    for (int idx=0;idx<MAX_ITER;idx++) {
        int base = idx * 5;
        CHECK(    lmRdmaRecvUnbuffered(context, base + 0, recvbuf, 2048));
        CHECK(    lmRdmaRecvMr(context, base + 1, &rem_rdmr));
        CHECK(    lmRdmaRecvMr(context, base + 2, &rem_wrmr));
        CHECK(    lmRdmaSendMr(context, base + 3, rdmr));
        CHECK(    lmRdmaSendMr(context, base + 4, wrmr));
    }

    cout << "MSG: " << recvbuf << endl;
    cout << "MSG: " << wrbuf << endl;

    rdma_event * event;
    ret = lmRdmaGetEvent(context, &event);
    if (ret == 1)
        cout << "event.type: " << event->type << endl;

    return 0;
}

