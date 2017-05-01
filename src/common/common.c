
#include <stdio.h>
#include <string.h>

#include "common.h"

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

<<<<<<< HEAD
// These defines need a real value and to be moved.




int
getprintcap (struct printer *printer) {
    const char *printcapdb[2] = {_PATH_PRINTCAP, 0};
    char *line;
	const char *dp;
	int i;
    char* printcap_buffer;
    // clear any already malloc'd printer settings.
    //puts("here");

        //TODO replace printf with fatal for logging
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
  puts("here");
  //free_pr(printer);
  puts("here");

  /* TODO: If printer name is not found, indicate a useful error instead of segfault */
	printer->local_printer = cgetstr(printcap_buffer, DEFAULT_PRINTER, &line) == -1 ? _PATH_DEFDEVLP : line;
	printer->remote_printer = cgetstr(printcap_buffer, "rp", &line) == -1 ? DEFAULT_PRINTER : line;
	printer->spooling_dir = cgetstr(printcap_buffer, "sd", &line) == -1 ? _PATH_DEFSPOOL : line;
	printer->lock_file = cgetstr(printcap_buffer, "lo", &line) == -1 ? DEFLOCK : line;
	printer->status_file = cgetstr(printcap_buffer, "st", &line) == -1 ? DEFSTAT : line;
	printer->remote_printer = cgetstr(printcap_buffer, "rm", &line) == -1 ? NULL : line;
    printer->log_file = cgetstr(printcap_buffer, "lf", &line) == -1 ? _PATH_CONSOLE : line;
    //puts("here");

  printer->restr_group = cgetstr(printcap_buffer, "rg", &line) == -1 ? NULL : line;    //printcap file(ASCII) is in /src/etc/printcap (in printcap file delete the following...)
=======

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

  free_printer (printer);

  /* TODO: strdup default string values. */
  printer->local_printer = cgetstr(printcap_buffer, DEFAULT_PRINTER, &line) == -1 ? PATH_DEFDEVLP : line;
  printer->remote_printer = cgetstr(printcap_buffer, "rp", &line) == -1 ? DEFAULT_PRINTER : line;
  printer->spooling_dir = cgetstr(printcap_buffer, "sd", &line) == -1 ? PATH_DEFSPOOL : line;
  printer->lock_file = cgetstr(printcap_buffer, "lo", &line) == -1 ? DEFLOCK : line;
  printer->status_file = cgetstr(printcap_buffer, "st", &line) == -1 ? DEFSTAT : line;
  printer->remote_printer = cgetstr(printcap_buffer, "rm", &line) == -1 ? NULL : line;
  printer->log_file = cgetstr(printcap_buffer, "lf", &line) == -1 ? PATH_CONSOLE : line;
  printer->restr_group = cgetstr(printcap_buffer, "rg", &line) == -1 ? NULL : line;
  /* TODO add in the check for lpr/ipp */
>>>>>>> mainlpr/master
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
  struct printer *p = (struct printer *) malloc (sizeof (struct printer));

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
  if (printer->local_printer && (strcmp(printer->local_printer, PATH_DEFDEVLP) == 0)){
    free(printer->local_printer);
  }
  if (printer->remote_printer && (strcmp(printer->remote_printer, DEFAULT_PRINTER) == 0)){
    free(printer->remote_printer);
  }
  if (printer->spooling_dir && (strcmp(printer->spooling_dir, PATH_DEFSPOOL) == 0)){
    free(printer->spooling_dir);
  }
  if (printer->lock_file && (strcmp(printer->lock_file, DEFLOCK) == 0)){
    free(printer->lock_file);
  }
  if (printer->status_file && (strcmp(printer->status_file, DEFSTAT) == 0)){
    free(printer->status_file);
  }
  if (printer->log_file && (strcmp(printer->log_file, PATH_CONSOLE) == 0)){
    free(printer->log_file);
  }
  if (printer->remote_printer != NULL){
    free(printer->remote_printer);
  }
}

const char *
gethost (const char *hname)
{
  const char *p = strchr(hname, '@');
  return p ? ++p : hname;
}
