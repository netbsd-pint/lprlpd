
int connect_lpr(const char *host, const char *port, const char* user, const char* password);
int print_file_lpr(const int fd, const char* filepath, const int flags);
int job_stats_lpr(const int fd, const int job_id);
int stop_job_lpr(const int fd, const int job_id);
int resume_job_lpr(const int fd, const int job_id);
int printer_status_lpr(const int fd);
