#include "ivm.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

void * Computation(void * arg) {

    job_id    jid  = ivmGetMyJobId();
    group_id  gid  = ivmGetMyGroupId();
    device_id did  = ivmGetMyDeviceId();
    pe_id     peid = ivmGetMyPeId();

    cout << "Computation_0 Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid << endl;

    sleep(1);

    return NULL;
}

int main(int argc, char ** argv) {

    if (ivmEnter(argc, argv) != 0) {
        cout << "Cannot initialize" << endl;
        exit(-1);
    }

    ivmRegisterComp(Computation, "Computation_0");
    ivm_comp_config config;
    config.max_pes_num = 10;
    config.type = ivm_any;
//    ivmConfigureComp("Computation_0", config);
    ivmLaunchComp("Computation_0", config);

    if (ivmExit() != 0) {
        cout << "Cannot uninitialize" << endl;
        exit(-1);
    }

    return 0;
}

