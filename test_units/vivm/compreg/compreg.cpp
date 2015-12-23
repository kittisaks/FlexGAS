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
    usleep(30000);

    return NULL;
}

void * Calculation(void * arg) {
    
    job_id    jid  = ivmGetMyJobId();
    group_id  gid  = ivmGetMyGroupId();
    device_id did  = ivmGetMyDeviceId();
    pe_id     peid = ivmGetMyPeId();

    cout << "Calculation Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid << endl;
    usleep(30000);

    return NULL;
}

void * Finalize(void * arg) {

    job_id    jid  = ivmGetMyJobId();
    group_id  gid  = ivmGetMyGroupId();
    device_id did  = ivmGetMyDeviceId();
    pe_id     peid = ivmGetMyPeId();

    cout << "Finalization Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid << endl;
    usleep(30000);

    return NULL;
}

int main(int argc, char ** argv) {

    if (ivmEnter(argc, argv) != 0) {
        cout << "Cannot initialize" << endl;
        exit(-1);
    }

    ivmRegisterComp(Computation, "Computation_0");
    ivmRegisterComp(Finalize, "Finalization");
    ivmRegisterComp(Calculation, "Calculation");
    ivm_comp_config config, config1, config2;
    config.max_pes_num = 20;
    config.type = ivm_any;
    config1.max_pes_num = 100;
    config1.type = ivm_any;
    config2.max_pes_num = 30;
    config2.type = ivm_any;

    ivmLaunchComp("Finalization", config1);
    cout << "Finalization complete" << endl;
    ivmLaunchComp("Computation_0", config);
//    ivmLaunchComp("Calculation");

    if (ivmExit() != 0) {
        cout << "Cannot uninitialize" << endl;
        exit(-1);
    }

    return 0;
}

