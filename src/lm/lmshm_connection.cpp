#include "lmshm_internal.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <iostream>

using namespace std;

uint64_t eventIdCnt = 1;
bool     isShmInitialized = false;

int CreateShmContext(shm_context ** context) {

    shm_context * ct_temp = new shm_context();
    ct_temp->type = SHM_CONTEXT_UNINITIALIZED;
    ct_temp->init_attr.path = NULL;
    ct_temp->sock = NULL;
    ct_temp->status = SHM_CONTEXT_NULL;
    ct_temp->event = new shm_event();

    *context = ct_temp;
    return 0;
}

/**
 * @API:
 */

int lmShmInit() {

    if (isShmInitialized)
        return 0;

    shm_reqQueue = new RequestQueue();
    shm_compQueue = new RequestQueue();

    shm_reqQueue->clear();
    shm_compQueue->clear();

    TEST_Z(    pthread_mutex_init(&shm_reqQueueMtx, NULL));
    TEST_Z(    pthread_mutex_init(&shm_compQueueMtx, NULL));

    isShmInitialized = true;

    return 0;
}

/**
 * @API:
 */

int lmShmFinalize() {

    delete shm_reqQueue;
    delete shm_compQueue;

    return 0;
}

/**
 * @API:
 */

int lmShmListen(shm_context ** context, shm_init_attr * attr) {

    shm_context * ct_temp;
    int           fd;
    sockaddr_un * sock = new sockaddr_un();

    TEST_Z(    CreateShmContext(&ct_temp));
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == 0)
        return -1;

    //Set file descriptor properties
    int fd_flags = fcntl(fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    TEST_Z(    fcntl(fd, F_SETFL,
        fd_flags | FD_CLOEXEC));


    memset(sock, 0, sizeof(sockaddr_un));
    sock->sun_family = AF_UNIX;
    strcpy(sock->sun_path, attr->path);
    TEST_Z(    bind(fd, (struct sockaddr *) sock,
        sizeof(sockaddr_un)));
    TEST_Z(    listen(fd, 2000));

    ct_temp->fd = fd;
    ct_temp->type = SHM_CONTEXT_PASSIVE;
    ct_temp->init_attr = *attr;
    ct_temp->sock = sock;
    ct_temp->status = SHM_CONTEXT_ESTABLISHED;

    *context = ct_temp;

    return 0;
}

/**
 * @API:
 */

int lmShmConnect(shm_context ** context, shm_init_attr * attr) {

    shm_context * ct_temp;
    int           fd;
    sockaddr_un * sock = new sockaddr_un();

    TEST_Z(    CreateShmContext(&ct_temp));
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == 0)
        return -1;

    //Set file descriptor properties
    int fd_flags = fcntl(fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    TEST_Z(    fcntl(fd, F_SETFL,
        fd_flags | FD_CLOEXEC));


    memset(sock, 0, sizeof(sockaddr_un));
    sock->sun_family = AF_UNIX;
    strcpy(sock->sun_path, attr->path);
    size_t len = offsetof(sockaddr_un, sun_path) + strlen(sock->sun_path);
    TEST_Z(    connect(fd, (struct sockaddr *) sock, len));

    ct_temp->fd = fd;
    ct_temp->type = SHM_CONTEXT_ACTIVE;
    ct_temp->init_attr = *attr;
    ct_temp->sock = sock;
    ct_temp->status = SHM_CONTEXT_EST_WAIT_ACK;

    *context = ct_temp;

    return 0;
}

/**
 * @API:
 */

int lmShmAccept(shm_context ** context, shm_context * lcontext) {

    shm_context * ct_temp;
    int           fd;
    sockaddr_un * sock = new sockaddr_un();
    socklen_t     sock_len = sizeof(sockaddr_un);

    fd = accept(lcontext->fd, (struct sockaddr *) sock, &sock_len);
    if (fd == 0)
        return -1;

    //Set file descriptor properties
    int fd_flags = fcntl(fd, F_GETFL, 0);
    if (fd_flags == -1)
        return -1;
    TEST_Z(    fcntl(fd, F_SETFL,
        fd_flags | FD_CLOEXEC));

    TEST_Z(    CreateShmContext(&ct_temp));
    ct_temp->fd = fd;
    ct_temp->type = SHM_CONTEXT_ACTIVE;
    ct_temp->init_attr = lcontext->init_attr;
    ct_temp->sock = sock;
    ct_temp->status = SHM_CONTEXT_EST_WAIT_ACK;

    *context = ct_temp;

    return 0;
}

/**
 * @API:
 */

int lmShmClose(shm_context * context) {

    shutdown(context->fd, SHUT_RDWR);

    if (context->sock != NULL)
        delete context->sock;

    delete context->event;
    delete context;

    return 0;
}

/**
 * @API:
 */

int lmShmSetAcceptedContextAttr(shm_init_attr * attr) {

    return 0;
}

int ShmPassiveCreateEvent(shm_context * context, short revents) {

    if (context->status != SHM_CONTEXT_ESTABLISHED)
        return -1;
    if (context->event->type != SHM_EVENT_NOEVENT)
        return -1;

    //Either self or peer has disconnected.
    if ((revents & POLLHUP) || (revents & POLLRDHUP)) {
        context->event->context = context;
        context->event->eventId = eventIdCnt++;
        context->event->type = SHM_EVENT_DISCONNECTED;
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
        context->event->eventId = eventIdCnt++;
        context->event->type = SHM_EVENT_CONNECT_REQUEST;
    }

    return 0;
}

int ShmActiveCreateEvent(shm_context * context, short revents) {

    if (context->status != SHM_CONTEXT_ESTABLISHED)
        return -1;
    if (context->event->type != SHM_EVENT_NOEVENT)
        return -1;

    //Either self of peer has disconnected.
    if ((revents & POLLHUP) || (revents & POLLRDHUP)) {
        context->event->context = context;
        context->event->eventId = eventIdCnt++;
        context->event->type = SHM_EVENT_DISCONNECTED;
        return 0;
    }

    //Invalid connection of unsupported out-of-band message
    if ((revents & POLLNVAL) || (revents & POLLPRI)) {
        return -1;
    }

    return 0;
}

int ShmCreateEvent(shm_context * context, short revents) {

    switch(context->type) {
    case SHM_CONTEXT_UNINITIALIZED:
        return -1;
    case SHM_CONTEXT_PASSIVE:
        return ShmPassiveCreateEvent(context, revents);
    case SHM_CONTEXT_ACTIVE:
        return ShmActiveCreateEvent(context, revents);
    default:
        return -1;
    }

    return 0;
}

int ShmCreateEvent_est_wait_ack(shm_context * context,
    shm_event ** event) {

    shm_event * ev;

    ev = new shm_event();
    ev->context = context;
    ev->eventId = eventIdCnt;
    ev->type = SHM_EVENT_ESTABLISHED;
    memcpy(context->event, ev, sizeof(shm_event));
    context->status = SHM_CONTEXT_ESTABLISHED;
    *event = ev;

    return 0;
}

/**
 * @API:
 */

int lmShmGetEvent(shm_event ** event, shm_context * context) {

    shm_event * ev;
    struct pollfd fd_set;
    int num_fd;

    if (context->event->type == SHM_EVENT_DISCONNECTED)
        return -1;

    if (context->status == SHM_CONTEXT_EST_WAIT_ACK) {
        ShmCreateEvent_est_wait_ack(context, event);
        return 0;
    }

    fd_set.fd = context->fd;
    if (context->type == SHM_CONTEXT_PASSIVE) {
        fd_set.events = POLLIN | POLLPRI |
            POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;
    }
    else {
        fd_set.events = POLLPRI | POLLRDHUP | POLLIN | POLLOUT |
            POLLERR | POLLHUP | POLLNVAL;
    }
    num_fd = poll(&fd_set, 1, 1);
    if (num_fd <= 0)
        return -1;

    TEST_Z(    ShmCreateEvent(context, fd_set.revents));
    if (context->event->type == SHM_EVENT_NOEVENT)
        return -1;

    ev = new shm_event();
    memcpy(ev, context->event, sizeof(shm_event));
    *event = ev;

    return 0;
}

/**
 * @API:
 */

int lmShmWaitEvent(shm_event ** event, shm_context * context) {

    shm_event * ev;
    struct pollfd fd_set;
    int num_fd;

    if (context->event->type == SHM_EVENT_DISCONNECTED)
        return -1;

    if (context->status == SHM_CONTEXT_EST_WAIT_ACK) {
        ShmCreateEvent_est_wait_ack(context, event);
        return 0;
    }

    do {
        fd_set.fd = context->fd;
        if (context->type == SHM_CONTEXT_PASSIVE) {
            fd_set.events = POLLIN | POLLPRI |
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

    TEST_Z(    ShmCreateEvent(context, fd_set.revents));
    if (context->event->type == SHM_EVENT_NOEVENT)
        return -1;

    ev = new shm_event();
    memcpy(ev, context->event, sizeof(shm_event));
    *event = ev;

    return 0;
}

/**
 * @API:
 */

int lmShmAckEvent(shm_event * event) {

    if (event->eventId != event->context->event->eventId)
        return -1;

    if (event->context->event->type == SHM_EVENT_DISCONNECTED)
        return 0;

    event->context->event->eventId = 0;
    event->context->event->type = SHM_EVENT_NOEVENT;

    return 0;
}

/**
 * @API:
 */

int lmShmReleaseEvent(shm_event * event) {

    delete event;

    return 0;
}

