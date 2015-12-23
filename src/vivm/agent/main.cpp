#include "agent_threads.h"
#include "agent_trace.h"
#include <stdlib.h>
#include <unistd.h>

AgentTrace agt;

int main(int argc, char ** argv) {

    /**
     * ISSUE: Because the libibverbs may have some limitations with fork(),
     *        it is better to just the environment for child processes at
     *        the start-up time of the parent process. So that childs processes
     *        can inherit the environment from its parent. Having child
     *        processes set the environment on its own cause hang with 
     *        the libibverbs.
     */
    setenv("VIVM_IS_DESCENDANT", "1", 1);

    gethostname(local_hostname, MED_STR_LEN);

    AgentLocalListener  loc_listr;
#ifndef _DISABLE_LM_RDMA
    AgentRemoteListener rem_rdma_listr(AgentRemoteListener::Rdma);
#endif //_DISABLE_LM_RDMA
    AgentRemoteListener rem_inet_listr(AgentRemoteListener::Inet);

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

