#include <pthread.h>
#include <stdint.h>

uint64_t        chEventIdCnt = 0xf;
pthread_mutex_t chEventIdCntMtx;
bool            chEventIdCntInit = false;

void InitChEventIdCnt() {
    if (chEventIdCntInit)
        return;

    pthread_mutex_init(&chEventIdCntMtx, NULL);
    chEventIdCnt = 0xf;
    chEventIdCntInit = true;
}

uint64_t GenerateChEventId() {

    uint64_t id;

    pthread_mutex_lock(&chEventIdCntMtx);
    id = chEventIdCnt++;
    pthread_mutex_unlock(&chEventIdCntMtx);

    return id;
}

