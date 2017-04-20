#ifndef PROTO_LP_H
#define PROTO_LP_H

int connect_lpr		(const char* address, unsigned short port, const char* username, const char* password);
int print_file_lpr	(const int fd, char* queuename, lpr_flags* job);

#endif /*PROTO_LP_H*/
