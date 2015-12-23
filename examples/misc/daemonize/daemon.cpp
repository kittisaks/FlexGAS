#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

using namespace std;

int main(int argc, char ** argv) {

    pid_t pid = fork();

    if (pid > 0)
        exit(0);
    else if (pid == -1)
        return -1;

    pid_t ss_pid = setsid();
    if (ss_pid == -1)
        cout << "Error! " << errno << endl;

    close(0);
    close(1);
    close(2);

    while (1) {
        cout << "Print" << endl;
        sleep(1);
    }
    
}

