#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>

using namespace std;

typedef struct {
    int fd;
    struct sockaddr_un sockaddr;
} connection;

void * receiving_routine(void * arg) {

    connection * conn = (connection *) arg;

    char buffer[1024];

    while (1) {
        recv(conn->fd, buffer, 3, MSG_WAITALL);
        cout << "[RECV]: " << buffer << endl;
    }

    pthread_exit(NULL);
}

int main(int argc, char ** argv) {

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        cerr << "Cannot create AF_UNIX socket" << endl;
        exit(-1);
    }

    sockaddr_un sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr_un));
    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, "master_addr");
    int ret = bind(fd, (struct sockaddr *) &sockaddr, sizeof(sockaddr_un));
    listen(fd, 2000);

    pthread_t recv_th;
    pthread_attr_t recv_th_attr;
    pthread_attr_init(&recv_th_attr);
    pthread_attr_setdetachstate(&recv_th_attr, PTHREAD_CREATE_JOINABLE);

    while (1) {
        connection * conn = new connection();
        socklen_t    socklen = sizeof(struct sockaddr_un);
        int new_fd = accept(fd, (struct sockaddr *) &conn->sockaddr, &socklen);
        conn->fd = new_fd;
        cout << "[MAIN]: Connection received." << endl;
        pthread_create(&recv_th, &recv_th_attr, receiving_routine, conn);
    }

    return 0;
}

