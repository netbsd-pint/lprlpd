#include "common.h"




void getprintcap(struct printer_st *printer){

    pr = ;


    char *line;
	const char *dp;
	int i;

	if ((i = cgetent(&printcap_buffer, printcapdb, printer->name)) == -2)
		fatal("can't open printer description file");
	else if (i == -1)
		fatal("unknown printer: %s", printer->name);
	else if (i == -3)
		fatal("potential reference loop detected in printcap file");

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
}
