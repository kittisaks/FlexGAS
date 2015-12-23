#include "ivm.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

#define CHECK_Z(exp, str)                       \
    do {                                        \
        if (exp != 0) {                         \
            cerr << "Error: " << str << endl;   \
            exit(-1);                           \
        }                                       \
    } while (0)

void * Computation(void * arg) {

    int * obj = NULL;

    if (ivmGetMyPeId() == 0) {
        cout <<"TRY TO MALLOC - " << IVM_MEM_LOCAL << endl;
        ivmMalloc((void **) &obj, 1024, "SLAVE", IVM_MEM_LOCAL);
        obj[0] = 123;
    }
    else if (ivmGetMyPeId() > 3) {
#if 0
        CHECK_Z(    ivmMap((void **) &obj, "SLAVE", IVM_MEM_LOCAL),
            "ivmMap() (child-local)");
#else
//        ivmMap((void **) &obj, "SLAVE", IVM_MEM_LOCAL);
#endif
//        cout << "Mapping SLAVE" << endl;
    }

    int * master;
    CHECK_Z(    ivmMap((void **) &master, "MASTER", IVM_MEM_GLOBAL),
        "ivmMap() (child)");
    char hostname[64];
    gethostname(hostname, 64);
    cout << "Master value at [0]: " << master[0] << " - " << hostname << endl;

    return NULL;
}

int main(int argc, char ** argv) {

    CHECK_Z(    ivmEnter(argc, argv),
        "ivmEnter()");

    void * obj;
    CHECK_Z(    ivmMalloc(&obj, 1024, "MASTER", IVM_MEM_GLOBAL),
        "ivmMalloc()");

    memset(obj, 0, 1024);
    ((int *) obj)[0] = 12312121;

    CHECK_Z(    ivmRegisterComp(Computation, "Computation"),
        "ivmRegisterComp()");

    ivm_comp_config config;
    config.max_pes_num = 10;
    config.type = ivm_any;
    CHECK_Z(    ivmLaunchComp("Computation", config),
        "ivmLaunchComp()");


    CHECK_Z(    ivmExit(),
        "ivmExit()");

    return 0;
}

