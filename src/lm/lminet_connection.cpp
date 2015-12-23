#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif //_GNU_SOURCE
#define _LMINET_CONNECTION_CPP
#include "lminet.h"
//REVISE: #include "lminet_req_proc.h"
#include "lminet_connection.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include <sys/select.h>
#include <sys/time.h>

//CONNECT_REQUEST, ESTABLISHED, REJECTED, DISCONNECTED

using namespace std;

//REVISE: RequestProcessor * reqProc;
//REVISE: RequestQueue     * inet_reqQueue;
//REVISE: RequestQueue     * inet_compQueue;
InetHandler  * handler;
RequestQueue * reqQueue;
RequestQueue * compQueue;
uint64_t       inet_eventIdCnt;
bool           isInitialized = false;

extern "C" {

static inet_handler_list inetHandlerList = {
    NULL, NULL, NULL, NULL, NULL
};

int CreateInetContext(inet_context ** context) {

    inet_context * ct_temp;

    TEST_NN(    ct_temp = new inet_context);
    ct_temp->type = INET_CONTEXT_UNINITIALIZED;
    ct_temp->fd = 0;
    ct_temp->init_attr.self = NULL;
    ct_temp->init_attr.self_port = NULL;
    ct_temp->init_attr.peer = NULL;
    ct_temp->init_attr.peer_port = NULL;
    TEST_NN(    ct_temp->sock = new sockaddr_in());
    memset(ct_temp->sock, 0, sizeof(sockaddr_in));
    TEST_NN(    ct_temp->event = new inet_event());
    ct_temp->event->context = ct_temp;
    ct_temp->event->type = INET_EVENT_NOEVENT;
    ct_temp->status = INET_CONTEXT_NULL;

    *context = ct_temp;
    return 0;
}

int InetDestroyContext(inet_context * context) {

    close(context->fd);

    delete context->event;
    delete context->sock;
    delete context;

    return 0;
}

int lmInetAccept(inet_context ** accepted_context,
    inet_context * context) {

    inet_context * ct_temp;
    socklen_t      socklen;

    TEST_Z(    CreateInetContext(&ct_temp));
    memset(ct_temp->sock, 0, sizeof(sockaddr_in));
    socklen = sizeof(struct sockaddr_in);
    ct_temp->type = INET_CONTEXT_ACTIVE;
    ct_temp->fd = accept(context->fd, (sockaddr *) ct_temp->sock, &socklen);
    ct_temp->status = INET_CONTEXT_EST_WAIT_ACK;

    //Set Send/Recv buffer size
    uint64_t send_buf_size = LMINET_MTU;
    uint64_t recv_buf_size = LMINET_MTU;
    TEST_Z(    setsockopt(ct_temp->fd, SOL_SOCKET,
        SO_SNDBUF, &send_buf_size, sizeof(uint64_t)));
    TEST_Z(    setsockopt(ct_temp->fd, SOL_SOCKET,
        SO_RCVBUF, &recv_buf_size, sizeof(uint64_t)));

    //Set socket to reuse address and port in case if the port still in the
    //TIME_WAIT state
    int so_reuseaddr_val = 1;
    TEST_Z(    setsockopt(ct_temp->fd, SOL_SOCKET,
        SO_REUSEADDR, &so_reuseaddr_val, sizeof(int)));

    //Set file descriptor properties
    int fd_flags = fcntl(ct_temp->fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    TEST_Z(    fcntl(ct_temp->fd, F_SETFL,
        fd_flags | O_NONBLOCK));
    TEST_Z(    fcntl(ct_temp->fd, F_SETFD,
        FD_CLOEXEC));

//REVISE:    TEST_Z(    reqProc->RegisterFileDescriptor(ct_temp->fd));
    TEST_Z(    handler->RegisterFileDescriptor(ct_temp->fd));

    *accepted_context = ct_temp;

    return 0;
}

/**
 * @API:
 */

int lmInetInit() {

    if (isInitialized)
        return 0;

    inet_eventIdCnt = 1;

    try {
//REVISE:        reqProc   = new RequestProcessor();
        handler = new InetHandler();
        handler->HandlerStart();
    }
    catch (int e) {
        return e;
    }

//REVISE:    inet_reqQueue  = reqProc->GetRequestQueue();
//REVISE:    inet_compQueue = reqProc->GetCompQueue();
    handler->GetRequestQueue(&reqQueue);
    handler->GetCompletionQueue(&compQueue);

    isInitialized = true;

    return 0;
}

/**
 * @API:
 */

int lmInetFinalize() {

//REVISE:    delete reqProc;

    return 0;
}

/**
 * @API:
 */

int lmInetListen(inet_context ** context, inet_init_attr * attr) {

    inet_context * ct_temp;

    //Create a file descriptor
    TEST_Z(    CreateInetContext(&ct_temp));
    ct_temp->type = INET_CONTEXT_PASSIVE;
    ct_temp->fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_NZ(ct_temp->fd);

    //Set file descriptor properties
//    int fd_flags = fcntl(ct_temp->fd, F_GETFL, 0);
//    if (fd_flags == -1)
//        return -1;
//    TEST_Z(    fcntl(ct_temp->fd, F_SETFL,
//        fd_flags | FD_CLOEXEC));
     TEST_Z(    fcntl(ct_temp->fd, F_SETFD, FD_CLOEXEC));

    ct_temp->sock->sin_family = AF_INET;
    ct_temp->sock->sin_addr.s_addr = htonl(INADDR_ANY);
    ct_temp->sock->sin_port = htons(atoi(attr->self_port));

    //Set socket to reuse address and port in case if the port still in the
    //TIME_WAIT state
    int so_reuseaddr_val = 1;
    TEST_Z(    setsockopt(ct_temp->fd, SOL_SOCKET,
        SO_REUSEADDR, &so_reuseaddr_val, sizeof(int)));

    //Bind and Listen
    TEST_Z(    bind(ct_temp->fd, (sockaddr *) ct_temp->sock,
        sizeof(sockaddr_in)));
    TEST_Z(    listen(ct_temp->fd, 1000));

    ct_temp->status = INET_CONTEXT_ESTABLISHED;
    ct_temp->init_attr.self = attr->self;
    ct_temp->init_attr.self_port = attr->self_port;
    ct_temp->init_attr.peer = attr->peer;
    ct_temp->init_attr.peer_port = attr->peer_port;

    *context = ct_temp;

    return 0;
}

/**
 * @API:
 */

int lmInetConnect(inet_context ** context, inet_init_attr * attr) {

    inet_context * ct_temp;
    addrinfo     * addr = NULL, addr_hints;

    //Create a file descriptor
    TEST_Z(    CreateInetContext(&ct_temp));
    ct_temp->type = INET_CONTEXT_ACTIVE;
    ct_temp->fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_NZ(ct_temp->fd);

    //Resolve host infomation
    memset(&addr_hints, 0, sizeof(addrinfo));
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    TEST_Z(    getaddrinfo(attr->peer, attr->peer_port, NULL, &addr));

    if (addr->ai_family != AF_INET)
        return -1;

    memcpy(ct_temp->sock, addr->ai_addr, addr->ai_addrlen);

    //Connect to remote peer
    int ret = connect(ct_temp->fd, (sockaddr *) ct_temp->sock,
        addr->ai_addrlen);
    if (ret == -1) {
        cout << "errno: " << errno << endl;
        return -1;
    }

    ct_temp->status = INET_CONTEXT_EST_WAIT_ACK;
    ct_temp->init_attr.self = attr->self;
    ct_temp->init_attr.self_port = attr->self_port;
    ct_temp->init_attr.peer = attr->peer;
    ct_temp->init_attr.peer_port = attr->peer_port;

    //Set Send/Recv buffer size
    uint64_t send_buf_size = LMINET_MTU;
    uint64_t recv_buf_size = LMINET_MTU;
    TEST_Z(    setsockopt(ct_temp->fd, SOL_SOCKET,
        SO_SNDBUF, &send_buf_size, sizeof(uint64_t)));
    TEST_Z(    setsockopt(ct_temp->fd, SOL_SOCKET,
        SO_RCVBUF, &recv_buf_size, sizeof(uint64_t)));

    //Set socket to reuse address and port in case if the port still in the
    //TIME_WAIT state
    int so_reuseaddr_val = 1;
    TEST_Z(    setsockopt(ct_temp->fd, SOL_SOCKET,
        SO_REUSEADDR, &so_reuseaddr_val, sizeof(int)));

    //Set file descriptor properties
    int fd_flags = fcntl(ct_temp->fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    TEST_Z(    fcntl(ct_temp->fd, F_SETFL,
        fd_flags | O_NONBLOCK));
    TEST_Z(    fcntl(ct_temp->fd, F_SETFD,
        FD_CLOEXEC));

    freeaddrinfo(addr);
    
//REVISE:    TEST_Z(    reqProc->RegisterFileDescriptor(ct_temp->fd));
    TEST_Z(    handler->RegisterFileDescriptor(ct_temp->fd));

    *context = ct_temp;

    return 0;
}

/**
 * @API:
 */

int lmInetSetAcceptedContextAttr(inet_init_attr * attr) {

    return 0;
}

/**
 * @API:
 */

int lmInetClose(inet_context * context) {

    usleep(200000);
//REVISE:    TEST_Z(    reqProc->UnregisterFileDescriptor(context->fd));
    TEST_Z(    InetDestroyContext(context));

    return 0;
}

int InetPassiveCreateEvent(inet_context * context,
    short revents) {

    if (context->status != INET_CONTEXT_ESTABLISHED)
        return -1;
    if (context->event->type != INET_EVENT_NOEVENT)
        return -1;

    //Either self or peer has disconnected.
    if ((revents & POLLHUP) || (revents & POLLRDHUP)) {
        context->event->context = context;
        context->event->eventId = inet_eventIdCnt++;
        context->event->type = INET_EVENT_DISCONNECTED;
        return 0;
    }

    //Invalid operations and unsupported out-of-band message
    if ((revents & POLLOUT) || (revents & POLLPRI)
        || (revents & POLLNVAL) || (revents & POLLERR)) {
        return -1;
    }

    //Incoming connection available for acceptance
    if (revents & POLLIN) {
        context->event->context = context;
        context->event->eventId = inet_eventIdCnt++;
        context->event->type = INET_EVENT_CONNECT_REQUEST;
    }

    return 0;
}

int InetActiveCreateEvent(inet_context * context,
    short revents) {

    if (context->status != INET_CONTEXT_ESTABLISHED)
        return -1;
    if (context->event->type != INET_EVENT_NOEVENT)
        return -1;

    //Either self or peer has disconnected.
    if ((revents & POLLHUP) || (revents & POLLRDHUP)) {
        context->event->context = context;
        context->event->eventId = inet_eventIdCnt++;
        context->event->type = INET_EVENT_DISCONNECTED;
        return 0;
    }

    //Invalid connection of unsupported out-of-band message
    if ((revents & POLLNVAL) || (revents & POLLPRI)) {
        return -1;
    }

    //Send/Recv will not block = connection established
    if ((revents & POLLIN) || (revents & POLLOUT)) {
        context->event->context = context;
        context->event->eventId = inet_eventIdCnt++;
        context->event->type = INET_EVENT_ESTABLISHED;
    }

    return 0;
}

int InetCreateEvent(inet_context * context,
    short revents) {

    switch(context->type) {
    case INET_CONTEXT_UNINITIALIZED:
        return -1;
    case INET_CONTEXT_PASSIVE:
        return InetPassiveCreateEvent(context, revents);
    case INET_CONTEXT_ACTIVE:
        return InetActiveCreateEvent(context, revents);
    default:
        return -1;
    }

    return 0;
}

int InetCreateEvent_est_wait_ack(inet_context * context,
    inet_event ** event) {

    inet_event * ev;

    ev = new inet_event();
    ev->context = context;
    ev->eventId = inet_eventIdCnt;
    ev->type = INET_EVENT_ESTABLISHED;
    memcpy(context->event, ev, sizeof(inet_event));
    context->status = INET_CONTEXT_ESTABLISHED;
    *event = ev;

    return 0;
}

/**
 * @API:
 */

int lmInetRegisterHandler(inet_handler handlerFn,
    inet_event_type event_type) {

    switch (event_type) {
    case INET_EVENT_NOEVENT:
        inetHandlerList.noevent = handlerFn;         break;
    case INET_EVENT_CONNECT_REQUEST:
        inetHandlerList.connect_request = handlerFn; break;
    case INET_EVENT_ESTABLISHED:
        inetHandlerList.established = handlerFn;     break;
    case INET_EVENT_REJECTED:
        inetHandlerList.rejected = handlerFn;        break;
    case INET_EVENT_DISCONNECTED:
        inetHandlerList.disconnected = handlerFn;    break;
    }

    return 0;
}

/**
 * @API:
 */

int lmInetGetEvent(inet_context * context, inet_event ** event) {

    inet_event * ev;
    struct pollfd fd_set;
    int num_fd;

    if (context->event->type == INET_EVENT_DISCONNECTED)
        return -1;

    if (context->status == INET_CONTEXT_EST_WAIT_ACK) {
        InetCreateEvent_est_wait_ack(context, event);
        return 0;
    }
/*
        ev = new inet_event();
        ev->context = context;
        ev->eventId = inet_eventIdCnt++;
        ev->type = INET_EVENT_ESTABLISHED;
        memcpy(context->event, ev, sizeof(inet_event));
        context->status = INET_CONTEXT_ESTABLISHED;
        *event = ev;
        return 0;
    }
*/

    fd_set.fd = context->fd;
    if (context->type == INET_CONTEXT_PASSIVE) {
        fd_set.events = POLLIN | POLLPRI | POLLOUT |
            POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;
    }
    else {
        fd_set.events = POLLPRI | POLLRDHUP |
            POLLERR | POLLHUP | POLLNVAL;
    }
    num_fd = poll(&fd_set, 1, 1);
    if (num_fd <= 0)
        return -1;

    //Fill event
    TEST_Z(    InetCreateEvent(context, fd_set.revents));
    if (context->event->type == INET_EVENT_NOEVENT)
        return -1;

    ev = new inet_event();
    memcpy(ev, context->event, sizeof(inet_event));
    *event = ev;

    return 0;
}

/**
 * @API:
 */

int lmInetWaitEvent(inet_context * context, inet_event ** event) {

    inet_event * ev;
    struct pollfd fd_set;
    int num_fd;

    if (context->event->type == INET_EVENT_DISCONNECTED)
        return -1;

    if (context->status == INET_CONTEXT_EST_WAIT_ACK) {
        InetCreateEvent_est_wait_ack(context, event);
        return 0;
    }

    do {
        fd_set.fd = context->fd;
        if (context->type == INET_CONTEXT_PASSIVE) {
            fd_set.events = POLLIN | POLLPRI | POLLOUT |
                POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;
        }
        else {
            fd_set.events = POLLPRI | POLLRDHUP |
                POLLERR | POLLHUP | POLLNVAL;
        }
        num_fd = poll(&fd_set, 1, -1);
        if (num_fd == -1) 
            return -1;

    } while (num_fd == 0);

    //Fill event
    TEST_Z(    InetCreateEvent(context, fd_set.revents));

    ev = new inet_event();
    memcpy(ev, context->event, sizeof(inet_event));
    *event = ev;

    return 0;
}

/**
 * @API:
 */

int lmInetAckEvent(inet_event * event) {

    if (event->eventId != event->context->event->eventId)
        return -1;

    if (event->context->event->type == INET_EVENT_DISCONNECTED)
        return 0;

    event->context->event->eventId = 0;
    event->context->event->type = INET_EVENT_NOEVENT;

    return 0;
}

/**
 * @API:
 */

int lmInetReleaseEvent(inet_event * event) {

    delete event;

    return 0;
}

/**
 * @API:
 */

int lmInetHandleEvent(void ** ret, inet_event * event, void * arg) {
#define VERIFY_HANDLER(str, arg, context)             \
    if (inetHandlerList.str != NULL) {                \
        ret_temp = inetHandlerList.str(arg, context); \
        if (ret != NULL)                              \
            *ret = ret_temp;                          \
        return 0;                                     \
    }                                                 \
    else                                              \
        return -1

    inet_event ev;
    memcpy(&ev, event, sizeof(inet_event));
    lmInetAckEvent(event);

    void * ret_temp = NULL;
    switch (ev.type) {
    case INET_EVENT_NOEVENT:
        VERIFY_HANDLER(noevent, arg, ev.context);
    case INET_EVENT_CONNECT_REQUEST:
        inet_context * accepted_context;
        lmInetAccept(&accepted_context, ev.context);
        VERIFY_HANDLER(connect_request, arg, accepted_context);
    case INET_EVENT_ESTABLISHED:
        VERIFY_HANDLER(established, arg, ev.context);
    case INET_EVENT_REJECTED:
        VERIFY_HANDLER(rejected, arg, ev.context);
    case INET_EVENT_DISCONNECTED:
        VERIFY_HANDLER(disconnected, arg, ev.context);
    default:
        return -1;
    }

    return 0;

#undef VERIFY_HANDLER
}

} //extern "C"

