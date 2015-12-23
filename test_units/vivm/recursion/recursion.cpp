#include "ivm.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

int * stack;

void * Computation_Lv_0(void * arg) {

    job_id    jid   = ivmGetMyJobId();
    group_id  gid   = ivmGetMyGroupId();
    device_id did   = ivmGetMyDeviceId();
    pe_id     peid  = ivmGetMyPeId();
    int       level = ivmGetMyRecursionLevel();

    //Gain access to the stack
    ivmMap((void **) &stack, "STACK", IVM_MEM_GLOBAL);

    for (int idx=0;idx<level;idx++) cout << "\t";
    cout << "Computation_0 Computation " 
        << jid << "-" << gid << "-" << did << "-" << peid 
        << " / " << level << " / " << stack[level-1] << endl;


    //Lauching next level of computation
    if (level < 3) {
        stack[level] = (level + 1) * 100;

        ivm_comp_config config;
        config.max_pes_num = 10;
        config.type = ivm_any;
        ivmUpdate(&stack[level], sizeof(int));
        ivmLaunchComp("Computation_Lv_0", config);
        sleep(2);
    }
    else {
        sleep(2);
//        usleep(100000);
    }

    return NULL;
}

int main(int argc, char ** argv) {

    if (ivmEnter(argc, argv) != 0) {
        cout << "Cannot initialize" << endl;
        exit(-1);
    }

    //Register computations
    ivmRegisterComp(Computation_Lv_0, "Computation_Lv_0");

    //Memory allocation for the stack
    ivmMalloc((void **) &stack, 1024 * sizeof(int), "STACK", IVM_MEM_GLOBAL);
    memset(stack, 0, 1024 * sizeof(int));
    stack[0] = 100;

    //Launching 1st-level computation
    ivm_comp_config config;
    config.max_pes_num = 10;
    config.type        = ivm_any;
    config.req_res     = 3;
    int ret = ivmLaunchComp("Computation_Lv_0", config);
    if (ret != 0)
        cerr << "Cannot launch computation 'Computation_Lv_0'" << endl;

    if (ivmExit() != 0) {
        cout << "Cannot uninitialize" << endl;
        exit(-1);
    }

    return 0;
}

