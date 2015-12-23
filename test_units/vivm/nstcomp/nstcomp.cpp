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
//    usleep(50000);

#if 1
    int * vector;
    ivmMap((void **) &vector, "VECTOR", IVM_MEM_GLOBAL);

    vector[peid] = peid + 1;
//    cout << "\t\t\t" << gid << " - " << peid << " - " 
//        <<  vector[peid] << endl;
    int ret = ivmSyncPut(&vector[peid], sizeof(int));
    if (ret != 0)
        cout << "\t\tERROR" << endl;
#endif

#if 1
    ivm_comp_config config;
    config.max_pes_num = 5;
    config.type = ivm_any;
    ivmLaunchComp("Computation_Lv_1", config);
#endif

//    cout << peid << " - " << "completes" << endl;

    return NULL;
}

void * Computation_Lv_1(void * arg) {
    
    job_id    jid  = ivmGetMyJobId();
    group_id  gid  = ivmGetMyGroupId();
    device_id did  = ivmGetMyDeviceId();
    pe_id     peid = ivmGetMyPeId();

    cout << "\tComputation_1 Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid << endl;
    int * vector;
    ivmMap((void **) &vector, "VECTOR", IVM_MEM_GLOBAL);

    vector[peid] = (peid + 1) * 100;
//    cout << "\t\t\t" << gid << " - " << peid << " - " 
//        <<  vector[peid] << endl;
    int ret = ivmSyncPut(&vector[peid], sizeof(int));
    if (ret != 0)
        cout << "\t\tERROR" << endl;

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

    //Allocate memory
    int * vector;
    ivmMalloc((void **) &vector, 1 * 1048576, "VECTOR", IVM_MEM_GLOBAL);

    vector[0] = 1212121;
    cout << "VECTOR[0] (before): " << vector[0] << endl;
    cout << "VECTOR[1] (before): " << vector[1] << endl;
    cout << "VECTOR[2] (before): " << vector[2] << endl;
    cout << "VECTOR[3] (before): " << vector[3] << endl;

    //Launching 1st-level computation
    ivm_comp_config config;
    config.max_pes_num = 4;
    config.type = ivm_any;
    ivmLaunchComp("Computation_Lv_0", config);

    cout << "VECTOR[0] (after):  " << vector[0] << endl; 
    cout << "VECTOR[1] (after):  " << vector[1] << endl; 
    cout << "VECTOR[2] (after):  " << vector[2] << endl; 
    cout << "VECTOR[3] (after):  " << vector[3] << endl; 

    if (ivmExit() != 0) {
        cout << "Cannot uninitialize" << endl;
        exit(-1);
    }

    return 0;
}

