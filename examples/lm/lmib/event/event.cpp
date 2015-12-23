#include "lm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

#define DEFAULT_PORT "65201"
#define CHECK(str, err) do {             \
    int ret = str;                       \
    if (ret != 0) {                      \
        cerr << "Error:" << err << endl; \
        return -1;                       \
    }                                    \
} while (0);


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

int Server(int argc, char ** argv) {

    rdma_init_attr   init_attr;
    rdma_context   * context;
    rdma_context   * client_context;

    init_attr.node.self_service = (char *) DEFAULT_PORT;

    CHECK(    lmRdmaListen(&context, &init_attr),
        "Cannot start listening");

    rdma_event * event;
    int          ret;
    do {
#if 0
    do {
        ret = lmRdmaGetEvent(context, &event);
    } while (ret <= 0);
#else
    ret = lmRdmaWaitEvent(context, &event);
#endif
    if (ret != 0)
        return -1;
    cout << "[s0]: " << context->status << "/"
        << context->event.type << endl;
    CHECK(    lmRdmaAckEvent(event),
        "Cannot accept client (0)");
    cout << "[s1]: " << context->status << "/"
        << context->event.type << endl;

    CHECK(    lmRdmaAccept(&client_context, context),
        "Cannot accept client (1)");

    ret = lmRdmaGetEvent(client_context, &event);
    if (ret != 0)
        return -1;
    cout << "[c0]: " << client_context->status << "/" 
        << client_context->event.type << endl;
    CHECK(    lmRdmaAckEvent(event), 
        "Cannot accept client (2)");
    cout << "[c1]: " << client_context->status << "/"
        << client_context->event.type << endl;

    cout << endl;
    cout << "REMOTE IB INFOMATION" << endl;
    cout << "LID: " << client_context->peer_ib.lid << endl;
    cout << "QPN: " << client_context->peer_ib.qpn << endl;
    cout << "PSN: " << client_context->peer_ib.psn << endl;
    cout << endl;

    usleep(100000);
#if 0
    do {
        ret = lmRdmaGetEvent(client_context, &event);
    } while (ret != 0);
#else
    ret = lmRdmaWaitEvent(client_context, &event);
#endif
    if (ret != 0)
        return -1;
    cout << "[c2]: " << client_context->status << "/" 
        << client_context->event.type << endl;
    CHECK(    lmRdmaAckEvent(event), 
        "");
    cout << "[c3]: " << client_context->status << "/"
        << client_context->event.type << endl;

    } while (1);
    
    return 0;
}

int Client(int argc, char ** argv) {

    rdma_init_attr   init_attr;
    rdma_context   * context;

    init_attr.node.peer = argv[1];
    init_attr.node.peer_service = (char *) DEFAULT_PORT;

    CHECK(    lmRdmaConnect(&context, &init_attr),
        "Cannot connect");

    rdma_event * event;
    int          ret;
    ret = lmRdmaGetEvent(context, &event);
    if (ret != 0)
        return -1;
    cout << "[c0]: " << context->status << "/"
        << context->event.type << endl;
    CHECK(    lmRdmaAckEvent(event), "Cannot connect");
    cout << "[c1]: " << context->status << "/"
        << context->event.type << endl;

    cout << endl;
    cout << "REMOTE IB INFORMATION" << endl;
    cout << "LID: " << context->peer_ib.lid << endl;
    cout << "QPN: " << context->peer_ib.qpn << endl;
    cout << "PSN: " << context->peer_ib.psn << endl;
    cout << endl;

    lmRdmaClose(context);

    return 0;
}

