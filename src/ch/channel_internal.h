#ifndef _CHANNEL_INTERNAL_H
#define _CHANNEL_INTERNAL_H

#include <pthread.h>
#include <stdint.h>

extern uint64_t        chEventIdCnt;
extern pthread_mutex_t chEventIdCntMtx;

void     InitChEventIdCnt();
uint64_t GenerateChEventId();

#endif //_CHANNEL_INTERNAL_H

