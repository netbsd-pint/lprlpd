#ifndef SERVER_H
#define SERVER_H

int listen_on(unsigned short port, int queue);
void serverLoop(int fd);

#endif
