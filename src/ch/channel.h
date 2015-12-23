#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "lm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ch_passive = 0,
    ch_active
} ch_type;

typedef enum {
#ifndef _DISABLE_LM_RDMA
    ch_link_rdma=0x0100,
#endif //_DISABLE_LM_RDMA
    ch_link_inet=0x0101,
    ch_link_shm=0x0102
} ch_link;

typedef struct {
    ch_link   link;
    char    * self_id;
    char    * self_port;
    char    * peer_id;
    char    * peer_port;
} ch_init_attr;

typedef struct {
    ch_type      type;
    ch_link      link;
    union {
        shm_context  * shm;
        inet_context * inet;
#ifndef _DISABLE_LM_RDMA
        rdma_context * rdma;
#endif //_DISABLE_LM_RDMA
    } ctx;
    ch_init_attr init_attr;
} ch_t, * ch;

typedef enum {
    //Connection-related
    ch_new_connection = 0xf,
    ch_established,
    ch_accept_error,
    ch_connect_error,
    ch_rejected,
    ch_close_connection,

    //Operation-related
    ch_pending,
    ch_success,
    ch_error,
    ch_length_error,
    ch_link_unhandled,
    ch_unknown_error
} ch_event_type;

typedef struct {
    ch_event_type type;
    ch            channel;
    uint64_t      id;
    //For returning an accepted channel of object with an event.
    union {
        ch     channel;
        void * obj;
    } content;
    //For returning LM-specific channel event.
    union {
#ifndef _DISABLE_LM_RDMA
        rdma_event * rdma;
#endif //_DISABLE_LM_RDMA
        inet_event * inet;
        shm_event  * shm;
    } event;
    //For returning LM-specific work completion.
    union {
#ifndef _DISABLE_LM_RDMA
        rdma_request  * rdma;
#endif //DISABLE_LM_RDMA
        inet_request  * inet;
        shm_request   * shm;
    } wc;
} ch_event_t, * ch_event;

#define CH_RDMA_ENA 0x01
#define CH_INET_ENA 0x02
#define CH_SHM_ENA  0x04

typedef struct {
    enum {
        ch_mr_local,
        ch_mr_remote
    } type;
    ch            channel;
    int           link_ena;
    void        * addr;
    size_t        length;

    //rdma
    uint32_t      handle;
    uint32_t      lkey;
    uint32_t      rkey; 
#ifndef _DISABLE_LM_RDMA
    ibv_mr      * local_mr;
#endif //_DISABLE_LM_RDMA

    //inet
    char          host[128];
    char          service[128];

    //shm
    char          name[128];
    int           fd;
} ch_mr_t, * ch_mr;

/**
 * Connection handling
 */

int chOpen(ch * channel, ch_init_attr attr, int passive);

int chClose(ch channel);

/**
 * Event and channel handling
 */

int chPollChannel(ch_event * event, ch channel);

int chWaitChannel(ch_event * event, ch channel);

int chPollEvent(ch_event event);

int chWaitEvent(ch_event event);

int chReleaseEvent(ch_event event);

/**
 * Send/Recv
 */

int chSend(ch channel, void * addr, size_t length);

int chSendAsync(ch channel, void * addr, size_t length, ch_event * event);

int chRecv(ch channel, void * addr, size_t length);

int chRecvAsync(ch channel, void * addr, size_t length, ch_event * event);

int chSendMr(ch channel, ch_mr mr);

int chRecvMr(ch channel, ch_mr * mr);

/**
 * Remote Rd/Wr
 */

#define CH_SHM_FCLEAN   0x01
#define CH_ADD_LINK_ENA 0x02 

int chRegisterMemory(ch channel, void * addr, size_t length,
    ch_mr * mr, int flags);

int chUnregisterMemory(ch channel, ch_mr mr);

//This call is available to only LM-SHM
int chMapMemory(ch  channel, ch_mr mr, void ** addr, size_t * length);

//This call is available to only LM-SHM
int chUnmapMemory(ch channel, ch_mr);

int chRemoteRead(ch channel, void * addr, void * rem_addr, ch_event * event);

int chRemoteReadAsync(ch channel, ch_mr mr, void * addr, ch_mr rem_mr,
    void * rem_addr, size_t length, ch_event * event);

int chRemoteWrite(ch channel, void * addr, void * rem_addr, ch_event * event);

int chRemoteWriteAsync(ch channel, ch_mr mr, void * addr, ch_mr rem_mr,
    void * rem_addr, size_t length, ch_event * event);

#ifdef __cplusplus
}
#endif

#endif //_CHANNEL_H

