#include "lpr_api.h"

int connect_lpr(const char *host, const char *port, const char* user, const char* password){
    return 0;
}
int print_file_lpr(const int fd, const char* filepath, const int flags){
    return 0;
}
int job_stats_lpr(const int fd, const int job_id){
    return 0;
}
int stop_job_lpr(const int fd, const int job_id){
    return 0;
}
int resume_job_lpr(const int fd, const int job_id){
    return 0;
}
int printer_status_lpr(const int fd){
    return 0;
}
