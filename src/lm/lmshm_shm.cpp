#include "lmshm_internal.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>

using namespace std;

int lmShmAlloc(shm_mr ** mr, void ** addr, size_t length,
    char * name, int flags) {

    shm_mr * mr_temp;
    void   * obj_ptr;
    int      fd;

    //Create the shared object.
    TEST_NZ(    fd = shm_open(name,
        O_CREAT | O_EXCL | O_RDWR,
        S_IRUSR | S_IWUSR |
        S_IRGRP | S_IWGRP |
        S_IROTH | S_IWOTH));

    if (fd == -1) {
        if (flags && LM_SHM_FCLEAN) {
            TEST_Z(    shm_unlink(name));
            TEST_NZ(    fd = shm_open(name,
                O_CREAT | O_EXCL | O_RDWR,
                S_IRUSR | S_IWUSR |
                S_IRGRP | S_IWGRP |
                S_IROTH | S_IWOTH));
        }
        else
            return -1;
    }

    //Set the size of the shared object.
    TEST_Z(    ftruncate(fd, length));

    //Map the object to the address space of the process
    TEST_NN(    obj_ptr = mmap(NULL, length,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    mr_temp = new shm_mr();
    memset(mr_temp, 0, sizeof(shm_mr));
    strcpy(mr_temp->name, name);
    mr_temp->fd = fd;
    mr_temp->addr = obj_ptr;
    mr_temp->length = length;

    *addr = obj_ptr;
    *mr = mr_temp;

    return 0;
}

int lmShmFree(shm_mr * mr) {

    //Unmap the object from the address space
    TEST_Z(    munmap(mr->addr, mr->length));

    //Globally destroy the object
    TEST_Z(    shm_unlink(mr->name));

    return 0;
}

int lmShmAcquire(shm_mr * mr, void ** addr, size_t * length) {

    void   * obj_ptr;
    int      fd;

    //Create the shared object.
    fd = shm_open(mr->name,
        O_RDWR,
        S_IRUSR | S_IWUSR |
        S_IRGRP | S_IWGRP |
        S_IROTH | S_IWOTH);

    if (fd == -1)
        return -1;

    //Map the object to the address space of the process
    TEST_NN(    obj_ptr = mmap(NULL, mr->length,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    mr->addr = obj_ptr;
    mr->fd = fd;

    if (addr != NULL)
        *addr = obj_ptr;

    if (length != NULL)
        *length = mr->length;

    return 0;
}

int lmShmRelease(shm_mr * mr) {

    //Unmap the object from the address space
    TEST_Z(    munmap(mr->addr, mr->length));

    return 0;
}

