#include "ivm.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

void * Computation_Lv_0(void * arg) {

    job_id    jid  = ivmGetMyJobId();
    group_id  gid  = ivmGetMyGroupId();
    device_id did  = ivmGetMyDeviceId();
    pe_id     peid = ivmGetMyPeId();

    cout << "Computation_0 Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid << endl;


    ivm_comp_config config;
    config.max_pes_num = 4;
    config.type = ivm_any;
    ivmLaunchComp("Computation_Lv_1", config);

    return NULL;
}

void * Computation_Lv_1(void * arg) {
    
    job_id    jid  = ivmGetMyJobId();
    group_id  gid  = ivmGetMyGroupId();
    device_id did  = ivmGetMyDeviceId();
    pe_id     peid = ivmGetMyPeId();

    cout << "\tComputation_1 Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid << endl;

    ivm_comp_config config;
    config.max_pes_num = 3;
    config.type = ivm_any;
    ivmLaunchComp("Computation_Lv_2", config);

    return NULL;
}

void * Computation_Lv_2(void * arg) {
    
    job_id    jid  = ivmGetMyJobId();
    group_id  gid  = ivmGetMyGroupId();
    device_id did  = ivmGetMyDeviceId();
    pe_id     peid = ivmGetMyPeId();

    cout << "\t\tComputation_2 Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid << endl;

    return NULL;
}

int main(int argc, char ** argv) {

    if (ivmEnter(argc, argv) != 0) {
        cout << "Cannot initialize" << endl;
        exit(-1);
    }

    //Register computations
    ivmRegisterComp(Computation_Lv_0, "Computation_Lv_0");
    ivmRegisterComp(Computation_Lv_1, "Computation_Lv_1");
    ivmRegisterComp(Computation_Lv_2, "Computation_Lv_2");

    //Launching 1st-level computation
    ivm_comp_config config;
    config.max_pes_num = 4;
    config.type = ivm_any;
    ivmLaunchComp("Computation_Lv_0", config);

    if (ivmExit() != 0) {
        cout << "Cannot uninitialize" << endl;
        exit(-1);
    }

    return 0;
}

