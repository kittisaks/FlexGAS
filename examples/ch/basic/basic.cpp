#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

#define MODE_SERVER 0
#define MODE_CLIENT 1
#define LINK_RDMA   ch_link_rdma
#define LINK_INET   ch_link_inet
#define LINK_SHM    ch_link_shm

int      mode;
char     emsg[256];
char     peer[64];
char     port[64];
ch_link  chlink = LINK_INET;

#define COMPARE(var_1, var_2, str) \
    if (++var_1 >= var_2) {        \
        sprintf(emsg, "%s", str);  \
        return -1;                 \
    }

void PrintHelp(int argc, char ** argv) {
    cerr << endl << "Testbench for Channel API" << endl;
    cerr << "Usage: " << argv[0] << " [Options]" << endl << endl;
    cerr << "\tOptions:" << endl;
    cerr << "\t\t-s, --server  \t" << endl;
    cerr << "\t\t-c, --client  \t" << endl;
    cerr << "\t\t-a, --address \t" << endl;
    cerr << "\t\t-p, --port    \t" << endl;
    cerr << endl;
    exit(0);
}

int SetPreference(int argc, char ** argv) {

    strcpy(port, "65201");

    if (argc <= 1) 
        PrintHelp(argc, argv);

    int idx = 1;
    int mode_flag = 1, client_flag = 0;

    do {
        if ((!strcmp(argv[idx], "-s")) ||
            (!strcmp(argv[idx], "--server"))) {
            mode = MODE_SERVER;
            mode_flag = 0;
        }
        else if ((!strcmp(argv[idx], "-c")) ||
            (!strcmp(argv[idx], "--client"))) {
            mode = MODE_CLIENT;
            client_flag = 1;
            mode_flag = 0;
        }
        else if ((!strcmp(argv[idx], "-a")) ||
            (!strcmp(argv[idx], "--address"))) {
            COMPARE(idx, argc,
                "Please specify address to connect.");
            strcpy(peer, argv[idx]);
            client_flag = 0;
        }
        else if ((!strcmp(argv[idx], "-p")) ||
            (!strcmp(argv[idx], "--port"))) {
            COMPARE(idx, argc,
                "Please specify port.");
            strcpy(port, argv[idx]);
        }
        else if ((!strcmp(argv[idx], "-l")) ||
            (!strcmp(argv[idx], "--link"))) {
            COMPARE(idx, argc,
                "Please specify link type (rdma/inet/shm).");
            if (!strcmp(argv[idx], "rdma"))
                chlink = LINK_RDMA;
            else if (!strcmp(argv[idx], "inet"))
                chlink = LINK_INET;
            else if (!strcmp(argv[idx], "shm"))
                chlink = LINK_SHM;
            else {
                sprintf(emsg, "Unrecognized link type: %s", argv[idx]);
                return -1;
            }
        }
        else {
            sprintf(emsg, "Unrecognized option: %s", argv[idx]);
            return -1;
        }
        idx++;
    } while (idx<argc);

    if (mode_flag) {
        sprintf(emsg, "Please specify operation mode (server/client)");
        return -1;
    }

    if (client_flag) {
        sprintf(emsg, "Please specify peer address in client mode");
        return -1;
    }

    return 0;
}

void Server();

void Client();

int main(int argc, char ** argv) {

    if (SetPreference(argc, argv)) {
        cerr << "Error: " << emsg << endl;
        exit(-1);
    }

    switch(mode) {
    case MODE_SERVER:
        Server();
        break;
    case MODE_CLIENT:
        Client();
        break;
    }

    return 0;
}

#define BUFSIZE 10000
#define CHECK(str, step)                         \
    if (str != 0) {                              \
        cerr << "Error: step: " << step << endl; \
    }

void Server() {

    ch           channel, client_channel;
    ch_init_attr attr;
    ch_event     event;

    attr.link      = chlink;
    attr.self_port = port;

    chOpen(&channel, attr, 1);
    chWaitChannel(&event, channel);
    client_channel = event->content.channel;

    char * buf   = new char [BUFSIZE];
    char * buf_1 = new char [BUFSIZE];
    char   hostname[64];
    gethostname(hostname, 64);

    //Sending
    sprintf(buf, "Server send: %s", hostname);
    CHECK(    chSend(client_channel, buf, BUFSIZE), 1);
    cout << "SEND PASSED" << endl;

    //Receiving
    memset(buf, 0, BUFSIZE);
    CHECK(    chRecv(client_channel, buf, BUFSIZE), 2);
    cout << "RECV PASSED: " << buf << endl;

    //Reading
    ch_mr lmr, lmr_1, rmr;
    chRegisterMemory(client_channel, buf, BUFSIZE, &lmr, 0);
    chRegisterMemory(client_channel, buf_1, BUFSIZE, &lmr_1, 0);
    CHECK(    chSendMr(client_channel, lmr), 3);
    CHECK(    chRecvMr(client_channel, &rmr), 4);
    CHECK(    chRemoteReadAsync(client_channel, lmr, buf,
        rmr, rmr->addr, rmr->length, &event), 5);
    CHECK(    chWaitEvent(event), 6);
    cout << "READ PASSED: " << buf << endl;

    //Writing
    sprintf(buf, "Writing from: %s", hostname);
    CHECK(    chRemoteWriteAsync(client_channel, lmr, buf,
        rmr, rmr->addr, rmr->length, &event), 7);
    CHECK(    chWaitEvent(event), 8);
    cout << "WRITE PASSED" << endl;

    sleep(1);
}

void Client() {

    ch           channel;
    ch_init_attr attr;
    ch_event     event;

    attr.link      = chlink;
    attr.peer_id   = peer;
    attr.peer_port = port;

    chOpen(&channel, attr, 0);
    chWaitChannel(&event, channel);

    char * buf   = new char [BUFSIZE];
    char * buf_1 = new char [BUFSIZE];
    char   hostname[64];
    gethostname(hostname, 64);

    //Receving
    memset(buf, 0, BUFSIZE);
    CHECK(    chRecv(channel, buf, BUFSIZE), 1);
    cout << "RECV PASSED: " << buf << endl;

    //Sending
    sprintf(buf, "Client send: %s", hostname);
    CHECK(    chSend(channel, buf, BUFSIZE), 2);
    cout << "SEND PASSED" << endl;

    //Reading
    ch_mr lmr, lmr_1, rmr;
    sprintf(buf, "Content from: %s", hostname);
    chRegisterMemory(channel, buf, BUFSIZE, &lmr, 0);
    chRegisterMemory(channel, buf_1, BUFSIZE, &lmr_1, 0);
    CHECK(    chRecvMr(channel, &rmr), 3);
    CHECK(    chSendMr(channel, lmr), 4);
#if 0
    CHECK(    chRemoteReadAsync(channel, lmr, buf,
        rmr, rmr->addr, rmr->length, &event), 5);
    CHECK(    chWaitEvent(event), 6);
#endif

    sleep(1);
    cout << "MSG: " << buf << endl;
}


