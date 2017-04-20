#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/param.h>

#include "lpr_api.h"

#define _PATH_PRINTCAP "/etc/printcap"
#define _PATH_DEFDEVLP "a"
#define DEFAULT_PRINTER "lp"
#define _PATH_DEFSPOOL "a"
#define DEFLOCK "a"
#define DEFSTAT "a"
#define _PATH_CONSOLE "a"
#define DEFMX 1000

struct printer {
    char* local_printer;
    char* lock_file;
    char* log_file;
    char* name;
    char* remote_printer;
    char* spooling_dir;
    char* status_file;
    char* restr_group;
    bool mult_copies;
    long max_file_size;
    int protocol;
};

struct print_ops {
    int (*connect) (const char*, const char*,const char*,const char*);
    int (*print_file) (const int, const char*, const int);
    int (*job_stats) (const int, const int);
    int (*stop_job) (const int, const int);
    int (*resume_job) (const int, const int);
    int (*printeratus) (const int);
};

struct printer * new_printer ();
void free_printer (struct printer *printer);

//void getprintcap(struct printer *printer);
void setupprotocol();
int getprintcap(struct printer *printer);

#endif
