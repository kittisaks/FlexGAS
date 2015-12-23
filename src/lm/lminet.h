/**
 * Author: Kittisak Sajjapongse
 * Date:   2014 / Aug / 21
 */

#ifndef _LMINET_H
#define _LMINET_H

/**
 * Note: Non-blocking primitives are provided for implementing a multiplexing
 *       connection manager (either passive or active connection manager) as
 *       well as allow communication and computation to overlap.
 *
 *       Invoking non-blocking transfers (i.e. send, recv, remote rd/wr) on
 *       a connection simultaneously is not supported and does not guarantee
 *       correct order of arrival of information at the recepient. Hence,
 *       using multiple threads or an iterative loop to invoke non-blocking
 *       transfers on a single connection is highly not recommended.
 */

#include <arpa/inet.h>

#define TEST_Z(str)     \
do {                    \
    int rc = str;       \
    if (rc)             \
        return rc;      \
} while (0);

#define TEST_NZ(str)    \
do {                    \
    int rc = str;       \
    if (!rc)            \
        return -1;      \
} while (0);

#define TEST_NN(str)    \
if ((str) == NULL)      \
    return -1;

#ifdef __cplusplus
extern "C" {
#endif

#define LMINET_MTU 1048576

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct {
    char     * self;
    char     * self_port;
    char     * peer;
    char     * peer_port;
} inet_init_attr;

struct inet_context_st;

typedef enum {
    INET_CONTEXT_UNINITIALIZED,
    INET_CONTEXT_PASSIVE,
    INET_CONTEXT_ACTIVE
} inet_context_type;

typedef enum {
    INET_EVENT_NOEVENT,
    INET_EVENT_CONNECT_REQUEST,
    INET_EVENT_ESTABLISHED,
    INET_EVENT_REJECTED,
    INET_EVENT_DISCONNECTED
} inet_event_type;

typedef struct {
    struct inet_context_st * context;
    uint64_t                 eventId;
    inet_event_type          type;
} inet_event;

/* Internally used only */
typedef enum {
    INET_CONTEXT_NULL,
    INET_CONTEXT_EST_WAIT_ACK,
    INET_CONTEXT_ESTABLISHED,
    INET_CONTEXT_DISCONNECTED,
} inet_context_status;

typedef struct inet_context_st {
    inet_context_type     type;
    int                   fd;
    inet_init_attr        init_attr;
    sockaddr_in         * sock;
    inet_event          * event;
    inet_context_status   status;
} inet_context;

/**
 * Request / Completion Notification
 */

/* Internally used only */
typedef enum {
    Send,
    Recv,
    RemRd,
    RemWr,
    RemRdRpy,  /* Header used only */
    RemWrRpy,  /* Header used only */
    RemRdNotf,
    RemWrNotf
} inet_request_type;

typedef enum {
    Pending = 0x10,
    Success,
    Fail,
    LengthError
} inet_request_status;

typedef struct {
    uint64_t              reqId;
    inet_context        * context;
    inet_request_type     type;
    inet_request_status   status;
    void                * addr;
    void                * rem_addr;
    size_t                length;
} inet_request;

typedef struct {
    char         * host;
    inet_context * context;
    void         * addr;
    size_t         length;
} inet_mr;

/**
 * Inet Event
 */

typedef void * (* inet_handler) (void *, inet_context *);

typedef struct {
    inet_handler noevent;
    inet_handler connect_request;
    inet_handler established;
    inet_handler rejected;
    inet_handler disconnected;
} inet_handler_list;

/**
 * Connection Management
 */

int lmInetInit();

int lmInetFinalize();

int lmInetAccept(inet_context ** accepted_context,
    inet_context * context);

int lmInetListen(inet_context ** context, inet_init_attr * attr);

int lmInetConnect(inet_context ** context, inet_init_attr * attr);

int lmInetSetAcceptedContextAttr(inet_init_attr * attr);

int lmInetClose(inet_context * context);

/**
 * Connection Handler
 */

int lmInetRegisterHandler(inet_handler handlerFn,
    inet_event_type event_type);

int lmInetGetEvent(inet_context * context, inet_event ** event);

int lmInetWaitEvent(inet_context * context, inet_event ** event);

int lmInetAckEvent(inet_event * event);

int lmInetReleaseEvent(inet_event * event);

int lmInetHandleEvent(void ** ret, inet_event * event, void * arg);

/**
 * Send/Recv + Remote Rd/Wr
 */

int lmInetRegisterMemory(inet_context * context, void * addr,
    size_t length, inet_mr ** mr);

int lmInetSend(inet_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmInetRecv(inet_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmInetRemoteWrite(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length);

int lmInetRemoteRead(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length);

int lmInetSendAsync(inet_context * context, uint64_t reqId,
    void * addr, size_t legth);

int lmInetRecvAsync(inet_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmInetRemoteWriteAsync(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length);

int lmInetRemoteReadAsync(inet_context * context, uint64_t reqId,
    void * addr, void * rem_addr, size_t length);

int lmInetGetComp(uint64_t reqId, inet_request ** comp);

int lmInetWaitComp(uint64_t reqId, inet_request ** comp);

int lmInetReleaseComp(inet_request * comp);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //_LNINET_H

