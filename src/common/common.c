#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

struct print_ops printingAPI[2];

void setupprotocol (void) {
  /*
  printingAPI[0].connect = connect_lpr;
  printingAPI[0].print_file = print_file_lpr;
  printingAPI[0].job_stats = job_stats_lpr;
  printingAPI[0].stop_job = stop_job_lpr;
  printingAPI[0].resume_job = resume_job_lpr;
  printingAPI[0].printeratus = printer_status_lpr;

  printingAPI[1].connect = connect_ipp;
  printingAPI[1].print_file = print_file_ipp;
  printingAPI[1].job_stats = job_stats_ipp;
  printingAPI[1].stop_job = stop_job_ipp;
  printingAPI[1].resume_job = resume_job_ipp;
  printingAPI[1].printeratus = printer_status_ipp;
  */
}

/* Read the port@host format from the printcap and break into two
   strings */
void get_address_port(const char *port_at_host, const char *default_port,
                     char **host, char **port) {

  char *at = strchr(port_at_host, '@');
  size_t len;

  if (at)  {
    len = at - port_at_host + 1;

    if (len > 1) {
      *port = (char *)malloc(len);
      (void)strlcpy(*port, port_at_host, len);
    }
    else { /* @ with nothing in front - use default*/
      *port = strdup(default_port);
    }

    len = strlen(port_at_host) - (at - port_at_host) + 1;
    *host = (char *)malloc(len);
    (void)strlcpy(*host, port_at_host + (at - port_at_host) + 1, len);
  }
  else {
    *host = strdup(port_at_host);
    *port = strdup(default_port);
  }
}

int get_connection(const char *address, const char *port) {
  int fd = -1;
  int error;
  const char *cause = NULL;
  struct addrinfo hints, *res, *res0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  error = getaddrinfo(address, port, &hints, &res0);

  if (error) {
    err(1, "%s", gai_strerror(error));
  }

  for (res = res0; res; res = res->ai_next) {
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (fd < 0) {
      cause = "socket";
      continue;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
      cause = "connect";
      close(fd);
      fd = -1;
      continue;
    }

    /* connected */
    break;
  }

  if (fd < 0) {
    perror(cause);
  }

  return fd;
}

int
getprintcap (struct printer *printer) {
  //const char *dp;
  const char *printcapdb[2] = {PATH_PRINTCAP, 0};
  int i;
  char *line;
  char *printcap_buffer;

  //printf("%s","hey");

  /* clear any already malloc'd printer settings. */
  if ((i = cgetent(&printcap_buffer, printcapdb, printer->name)) == -2){
    printf("can't open printer description file\n");
    return -1;
  }
  else if (i == -1){
    printf("unknown printer: %s\n", printer->name);
    return -1;
  }
  else if (i == -3){
    printf("potential reference loop detected in printcap file\n");
    return -1;
  }

  /*lp|brother:\
        :lp=:sh:sd=/var/spool/lpd/lp:\
        rm=140.160.139.120:\
        lf=/var/log/lpd-errs:mx#0:

  #lp|local line printer:\
  #       :sh:lp=/dev/lp:sd=/var/spool/output/lpd:lf=/var/log/lpd-errs:
  */


  // I am trying to get -P flag working... sooooo (-P brother) should print to the brother printer!
  // currently testing and working on fixing it to work
  //printf("print name : %s \n", printer->name);
  free_printer (printer);


  /* TODO: strdup default string values.
           why are their two remote printers?*/
  printer->local_printer = cgetstr(printcap_buffer, printer->name, &line) == -1 ? strdup (PATH_DEFDEVLP) : line;
  printer->remote_printer = cgetstr(printcap_buffer, "rp", &line) == -1 ? strdup (DEFAULT_PRINTER) : line;
  printer->spooling_dir = cgetstr(printcap_buffer, "sd", &line) == -1 ? strdup (PATH_DEFSPOOL) : line;
  printer->lock_file = cgetstr(printcap_buffer, "lo", &line) == -1 ? strdup (DEFLOCK) : line;
  printer->status_file = cgetstr(printcap_buffer, "st", &line) == -1 ? strdup (DEFSTAT) : line;
  printer->remote_host = cgetstr(printcap_buffer, "rm", &line) == -1 ? NULL : line;
  printer->log_file = cgetstr(printcap_buffer, "lf", &line) == -1 ? strdup (PATH_CONSOLE) : line;
  printer->restr_group = cgetstr(printcap_buffer, "rg", &line) == -1 ? NULL : line;
  /* TODO add in the check for lpr/ipp */
  if (cgetnum(printcap_buffer, "mx", &printer->max_file_size) < 0)
    printer->max_file_size = DEFMX;
  printer->mult_copies = (cgetcap(printcap_buffer, "sc", ':') != NULL);

  /* TODO: add in check remote.
     if ((dp = checkremote()) != NULL) {
     printf("Warning: %s\n", dp);
     printer->log_file = cgetstr(printcap_buffer, "lf", &line) == -1 ? _PATH_CONSOLE : line;
     }
  */
  return 0;
}

struct printer *
new_printer (char *printer_name)
{
  struct printer *p = (struct printer *) calloc (1, sizeof (struct printer));

  if (p == NULL) {
    printf ("Failed to malloc in new_printer.");
    exit (1);
  }

  p->name = printer_name;
  return p;
}

void //maybe free printer_name (memory leak?)
free_printer (struct printer *printer)
{
  if (printer->local_printer){
    free(printer->local_printer);
  }
  if (printer->remote_printer){
    free(printer->remote_printer);
  }
  if (printer->spooling_dir){
    free(printer->spooling_dir);
  }
  if (printer->lock_file){
    free(printer->lock_file);
  }
  if (printer->status_file){
    free(printer->status_file);
  }
  if (printer->log_file){
    free(printer->log_file);
  }
  if (printer->remote_host != NULL){
    free(printer->remote_host);
  }
}

const char *
gethost (const char *hname)
{
  const char *p = strchr(hname, '@');
  return p ? ++p : hname;
}
