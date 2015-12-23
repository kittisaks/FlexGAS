#ifndef _LMSHM_INTERNAL_H
#define _LMSHM_INTERNAL_H

#include "lmshm.h"
#include <pthread.h>
#include <vector>

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

typedef std::vector<shm_request *> RequestQueue;
extern  RequestQueue    * shm_reqQueue;
extern  RequestQueue    * shm_compQueue;
extern  pthread_mutex_t   shm_reqQueueMtx;
extern  pthread_mutex_t   shm_compQueueMtx;

#endif //_LMSHM_INTERNAL_H

