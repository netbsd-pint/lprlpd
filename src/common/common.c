#include "common.h"

void free_pr(struct printer_st *printer);


void getprintcap(struct printer_st *printer){

    char *line;
	const char *dp;
	int i;

    // clear any already malloc'd printer settings.

	if ((i = cgetent(&printcap_buffer, printcapdb, printer->name)) == -2)
		fatal("can't open printer description file");
	else if (i == -1)
		fatal("unknown printer: %s", printer->name);
	else if (i == -3)
		fatal("potential reference loop detected in printcap file");

    free_pr(printer);

	printer->local_printer = cgetstr(bp, DEFLP, &line) == -1 ? _PATH_DEFDEVLP : line;
	printer->remote_printer = cgetstr(bp, "rp", &line) == -1 ? DEFLP : line;
	printer->spooling_dir = cgetstr(bp, "sd", &line) == -1 ? _PATH_DEFSPOOL : line;
	pinter->lock_file = cgetstr(bp, "lo", &line) == -1 ? DEFLOCK : line;
	printer->status_file = cgetstr(bp, "st", &line) == -1 ? DEFSTAT : line;
	printer->remote_printer = cgetstr(bp, "rm", &line) == -1 ? NULL : line;
    // TODO add in check remote
	if ((dp = checkremote()) != NULL)
		printf("Warning: %s\n", dp);
	printer->log_file = cgetstr(bp, "lf", &line) == -1 ? _PATH_CONSOLE : line;

    // TODO add in the check for lpr/ipp
}

void free_pr(struct printer_st *printer){
    if (strcmp(printer->local_printer, _PATH_DEFDEVLP) == 0){
        free(printer->local_printer);
    }
    if (strcmp(printer->remote_printer, DEFLP) == 0){
        free(printer->remote_printer);
    }
    if (strcmp(printer->spooling_dir, _PATH_DEFSPOOL) == 0){
        free(printer->spooling_dir);
    }
    if (strcmp(printer->lock_file, DEFLOCK) == 0){
        free(printer->lock_file);
    }
    if (strcmp(printer->status_file, DEFSTAT) == 0){
        free(printer->status_file);
    }
    if (strcmp(printer->log_file, _PATH_CONSOLE) == 0){
        free(printer->log_file);
    }
    if (printer->remote_printer != NULL){
        free(printer->remote_printer);
    }

}
