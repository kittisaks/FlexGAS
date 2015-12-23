/**
 * Author: Kittisak Sajjapongse
 * Date:   2014 / Aug / 30
 */

#ifndef _LMSHM_H
#define _LMSHM_H

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

/**
 * Note: This library provides two methods for inter-process communucation in
 *       a node. The first methods (i) is based on send/recv primitives and is
 *       implemented on AF_UNIX socket. The second methods (ii) is based on
 *       POSIX shared-memory to enable fast communication between processes.
 *       
 *       Method (i) is intended to support method (ii) for exchanging
 *       information of a shared object such as object name, and access key.
 *       The expected library usage is to use method (i) as a mechanism to
 *       initially link processes and exchange memory information between them;
 *       after a connection between processes is established, method (ii) will
 *       primarily be used to exchange user's data.
 */

#include <stdint.h>
#include <sys/un.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHM_NAME_LENGTH 512

typedef struct sockaddr_un sockaddr_un;

typedef struct {
    char * path;
} shm_init_attr;

typedef enum {
    SHM_CONTEXT_UNINITIALIZED = 0,
    SHM_CONTEXT_PASSIVE,
    SHM_CONTEXT_ACTIVE
} shm_context_type;

typedef enum {
    SHM_EVENT_NOEVENT = 0,
    SHM_EVENT_CONNECT_REQUEST,
    SHM_EVENT_ESTABLISHED,
    SHM_EVENT_REJECTED,
    SHM_EVENT_DISCONNECTED
} shm_event_type;

typedef struct {
    struct shm_context_st * context;
    uint64_t                eventId;
    shm_event_type          type;
} shm_event;

/* Internally used only */
typedef enum {
    SHM_CONTEXT_NULL = 0,
    SHM_CONTEXT_EST_WAIT_ACK,
    SHM_CONTEXT_ESTABLISHED,
    SHM_CONTEXT_DISCONNECTED
} shm_context_status;

typedef struct shm_context_st {
    int                  fd;
    shm_context_type     type;
    shm_init_attr        init_attr;
    sockaddr_un        * sock;
    shm_context_status   status;
    shm_event          * event;
} shm_context;

/**
 * Request / Completion Notification
 */

/* Internally used only */
typedef enum {
    ShmSend = 0,
    ShmRecv
} shm_request_type;

typedef enum {
    ShmPending = 0,
    ShmRecvWaitHdr,
    ShmRecvWaitBdy,
    ShmSuccess,
    ShmFail,
    ShmLengthError
} shm_request_status;

typedef struct {
    uint64_t             reqId;
    shm_context        * context;
    shm_request_type     type;
    shm_request_status   status;
    void               * addr;
    size_t               length;
    size_t               accLength;
    void               * hdrbuf;
    size_t               accBufLength;
} shm_request;

typedef struct {
    char     name[SHM_NAME_LENGTH];
    int      fd;       //Only valid for the current process
    void   * addr;     //Only valid for the current process
    size_t   length;
} shm_mr;

/**
 * Connection Management
 */

int lmShmInit();

int lmShmFinalize();

int lmShmListen(shm_context ** context, shm_init_attr * attr);

int lmShmConnect(shm_context ** context, shm_init_attr * attr);

int lmShmAccept(shm_context ** context, shm_context * lcontext);

int lmShmClose(shm_context * context);

int lmShmSetAcceptedContextAttr(shm_init_attr * attr);

/**
 * Connection Handler
 */

int lmShmGetEvent(shm_event ** event, shm_context * context);

int lmShmWaitEvent(shm_event ** event, shm_context  * context);

int lmShmAckEvent(shm_event * event);

int lmShmReleaseEvent(shm_event * event);

int lmShmSend(shm_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmShmRecv(shm_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmShmSendAsync(shm_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmShmRecvAsync(shm_context * context, uint64_t reqId,
    void * addr, size_t length);

int lmShmGetComp(uint64_t reqId, shm_request ** comp);

int lmShmWaitComp(uint64_t reqId, shm_request ** comp);

int lmShmReleaseComp(shm_request * comp);

/**
 * Shared Memory
 */

#define LM_SHM_FCLEAN 0x01

int lmShmAlloc(shm_mr ** mr, void ** addr, size_t length,
    char * name, int flags);

int lmShmFree(shm_mr * mr);

int lmShmAcquire(shm_mr * mr, void ** addr, size_t * length);

int lmShmRelease(shm_mr * mr);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //_LMSHM_H

