#ifndef server_t
#define common_T

#include <stdlib.h>


#define _PATH_PRINTCAP /etc/printcap

const char *printcapdb[2] = {_PATH_PRINTCAP, 0};
char* printcap_buffer;

struct printer_st{
    char* local_printer;
    char* lock_file;
    char* log_file;
    char* name;
    char* remote_printer;
    char* spooling_dir;
    char* status_file;
    int lpr;
};


void getprintcap(struct printer_st *printer);




#endif

// TODO move to another thing

#ifndef api
#define api
// TODO check if port should be a char* or int
int connect(const char *host, const char *port, const char* user, const char* password);
int print_file(const int fd, const char* filepath, const int flags);
int job_stats(const int fd, const int job_id);
int stop_job(const int fd, const int job_if);
int resume_job(const int fd, const int job_id);
int printer_status(const int fd)



#endif
