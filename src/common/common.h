#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/param.h>

#include "printer.h"
#include "print_job.h"

#define PATH_PRINTCAP "/etc/printcap"
#define PATH_DEFDEVLP "a"
#define DEFAULT_PRINTER "lp"
#define PATH_DEFSPOOL "a"
#define DEFLOCK "a"
#define DEFSTAT "a"
#define PATH_CONSOLE "a"
#define DEFMX 1000

struct print_ops {
  struct job_stat* (*job_stats) (const int, const struct job*);
  int (*print_file) (const int, const struct job*);
  int (*printer_status) (const int, struct printer*);
  int (*resume_job) (const int, const struct job*);
  int (*stop_job) (const int, const struct job*);
};

extern struct print_ops printingAPI[2];

int get_connection(const char *address, const char *port);
void get_address_port(const char *port_at_host, const char *default_port,
                     char **host, char **port);
void free_printer (struct printer *printer);
const char * gethost (const char *hname);
int getprintcap (struct printer *printer);
void setupprotocol (void);
struct printer * new_printer (char *printer_name);

#endif
