#include "sched_threads.h"
#include "sched_trace.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARG_SHORT_SCHEDLIB "-l"
#define ARG_LONG_SCHEDLIB  "--libary"

char       emsg[256];
char       schedlib[256];
bool       is_schedlib_set = false;
SchedTrace scht;

int ParseArguments(int argc, char ** argv) {
#define IS_ARG(str) !(strcmp(argv[idx], str))

    int idx= 1;
    while (idx < argc) {
        if ((IS_ARG(ARG_SHORT_SCHEDLIB)) ||
            (IS_ARG(ARG_LONG_SCHEDLIB))) {
            if (++idx < argc) {
                sprintf(schedlib, "%s", argv[idx]);
                is_schedlib_set = true;
            }
            else {
                sprintf(emsg, "Please specify the scheduling library.");
                return -1;
            }
        }
        idx++;
    }

    return 0;
#undef IS_ARG
}

int main(int argc, char ** argv) {

    if (ParseArguments(argc, argv) != 0) {
        fprintf(stderr, "\nError: %s\n\n", emsg);
        exit(-1);
    }

    gethostname(local_hostname, MED_STR_LEN);

    SchedLocalListener  loc_listr;
#ifndef _DISABLE_LM_RDMA
    SchedRemoteListener rem_rdma_listr(SchedRemoteListener::Rdma);
#endif //_DISABLE_LM_RDMA
    SchedRemoteListener rem_inet_listr(SchedRemoteListener::Inet);

    SchedScheduler * scheduler;
    if (is_schedlib_set)
        scheduler = new SchedScheduler(schedlib);
    else
        scheduler = new SchedScheduler();
    scheduler->Start();

    loc_listr.Start();
#ifndef _DISABLE_LM_RDMA
    rem_rdma_listr.Start();
#endif //_DISABLE_LM_RDMA
    rem_inet_listr.Start();

    loc_listr.Wait();
#ifndef _DISABLE_LM_RDMA
    rem_rdma_listr.Wait();
#endif //_DISABLE_LM_RDMA
    rem_inet_listr.Wait();

    return 0;
}

