#ifndef common
#define common

#include <stdbool.h>
#include <stdlib.h>
#include <sys/param.h>

#include "lpr_api.h"
//#include "global.h"

#define _PATH_PRINTCAP "/etc/printcap"
#define _PATH_DEFDEVLP "a"
#define DEFLP "a"
#define _PATH_DEFSPOOL "a"
#define DEFLOCK "a"
#define DEFSTAT "a"
#define _PATH_CONSOLE "a"
#define DEFMX 1000



char host[MAXHOSTNAMELEN+1];	/* host machine name */

struct printer_st{
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

struct function_st{
    int (*connect) (const char*, const char*,const char*,const char*);
    int (*print_file) (const int, const char*, const int);
    int (*job_stats) (const int, const int);
    int (*stop_job) (const int, const int);
    int (*resume_job) (const int, const int);
    int (*printer_status) (const int);
};

struct function_st printingAPI[2];

//void getprintcap(struct printer_st *printer);
void setupprotocol();
int getprintcap(struct printer_st *printer);


#endif
