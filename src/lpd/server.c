#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include "server.h"
#include "threadpool.h"


void sighandler(int num) __attribute__ ((noreturn));
static unsigned short globalport = 0;
int main(){

    unsigned short port = 25525;
    int max_half_connections = 10;


    struct sigaction act;
    act.sa_handler = sighandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGINT, &act, NULL);



    // setting up network stuff.
    int fd = listen_on(port, max_half_connections);
    if (fd == -1){
        perror("Error in listen_on");
    }
    globalport = port;
    printf("Listening on port %d\n", port);



    thread_pool_init();
    serverLoop(fd);


}



// TODO: Needs to be rewritten. This is clausons function from 367
int listen_on(unsigned short port, int queue){

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        fprintf(stderr, "Call to socket failed because (%d) \"%s\"\n",
        errno, strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,&optval,4);
    // become root
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        fprintf(stderr, "Call to bind failed because (%d) \"%s\"\n",
        errno, strerror(errno));
    close (fd);
    return -1;
    }
    //become not root
    if(listen(fd, queue)) {
        fprintf(stderr, "Call to listen failed because (%d) \"%s\"\n",
            errno, strerror(errno));
            close (fd);
        return -1;
    }

    return fd;
}

void serverLoop(int FD){
    int clientFD;


	//struct timeval timeout = {2,0};

	int working = 1;
	while(working){
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        bzero(&addr, len);

        clientFD = accept(FD, (struct sockaddr*) &addr, &len);
        if(clientFD == -1){
            perror("accept");
            _exit(0);
        }

        requestJob(clientFD);

    }


}

void sighandler(int num){
  
  close(globalport);
  _exit(num);
}
