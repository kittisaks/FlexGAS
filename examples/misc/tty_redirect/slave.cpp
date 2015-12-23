#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

using namespace std;

int main(int argc, char ** argv) {

    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    else if (pid == -1) {
        cerr << "Cannot fork process" << endl;
        exit(-1);
    }

    int ret = setsid();
    if (ret == -1) {
        cerr << "Cannot set new session" << endl;
        exit(-1);
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        cerr << "Cannot create AF_UNIX socket" << endl;
        exit(-1);
    }

    dup2(fd, 1);
    dup2(fd, 2);
    dup2(fd, fileno(stdout));
    dup2(fd, fileno(stderr));

    sockaddr_un sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr_un));
    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, "master_addr");
    size_t len = offsetof(sockaddr_un, sun_path) + strlen(sockaddr.sun_path);
    ret = connect(fd, (struct sockaddr *) &sockaddr, len);
    if (ret != 0) {
        cerr << "Cannot connect" << endl;
        exit(-1);
    }

    while(1) {

//        cerr << "DEF" << endl;
//        write(1, "KKKKKKKKKKKKKKKK\n", 18);
        fprintf(stdout, "JJJJJJJJJJJJJJ\n");
        fpirntf(stdout, strlen(cout.c_str()));
        cout.flush();
//        char buffer[] = "ABC";
//        send(fd, buffer, strlen(buffer), 0);
        sleep(1);
    }

    return 0;
}

