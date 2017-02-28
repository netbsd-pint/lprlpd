#ifndef server_t
#define server_t

int listen_on(unsigned short port, int queue);
void serverLoop(int fd);

#endif
