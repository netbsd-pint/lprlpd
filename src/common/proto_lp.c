#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "lpr_job.h"

/*Daemon command codes*/
#define CMD_PRINT		0x01
#define CMD_INITJOB		0x02
#define CMD_GETQ		0x03
#define CMD_GETQVERB	0x04
#define CMD_REMJOB		0x05

/*Job description command codes*/
#define CMD_ABORT		0x01
#define CMD_CTRLFILE	0x02
#define CMD_DATAFILE	0x03

/*Control file command codes*/
/*THESE ARE CHARACTERS. DO NOT DEFINE WITH MORE THAN ONE CHARACTER. The use of 
strings ensures that literals are passable to functions such as send()*/
#define CMD_BNRCLASS	"C"	/*Set banner page class name*/
#define CMD_HOSTNAME	"H"	/*Specify host starting job*/
#define CMD_INDENT		"I"	/*Specify tab-in for plaintext files*/
#define CMD_BNRNAME		"J"	/*Set banner page job name*/
#define CMD_PRINTBNR	"L"	/*Set banner page user name, print banner page*/
#define CMD_MAILDONE	"M"	/*Mail the specified user when job is done*/
#define CMD_SRCNAME		"N"	/*Specify name of the source file*/
#define CMD_USERID		"P"	/*Specify user starting job*/
#define CMD_SYMLINK		"S"	/*Memoize symlink data (ignore dirent changes)*/
#define CMD_TITLE		"T"	/*Specify file title for use in headers*/
#define CMD_UNLINK		"U"	/*Mark data file as no longer required*/
#define CMD_COLWIDTH	"W"	/*Specify number of columns for plaintext printing*/
#define CMD_TROFFR		"1"	/*Use troff R font (default Times Roman)*/
#define CMD_TROFFI		"2"	/*Use troff I font (default Times Italic)*/
#define CMD_TROFFB		"3"	/*Use troff B font (default Times Bold)*/
#define CMD_TROFFS		"4"	/*Use troff S font (default Special Mathematical)*/
#define CMD_CIFPLOT		"5"	/*Treat as Caltech Intermediate Form plot format*/
#define CMD_DVI			"d"	/*Treat as TeX format*/
#define CMD_PLAINTEXT	"f"	/*Treat as plain-text ASCII format*/
#define CMD_PLOT		"g" /*Treat as Berkeley Unix plot format*/
#define CMD_UNFILTER	"l"	/*Treat as ASCII format, printing control chars*/
#define CMD_DITROFF		"n" /*Treat as ditroff format*/
#define CMD_POSTSCR		"o" /*Treat as postscript format*/
#define CMD_PRHEADERS	"p" /*Print formatting data (heading, page numbers)*/
#define CMD_FORTRAN		"r"	/*Use fortran carriage control (remove?)*/
#define CMD_TROFF		"t"	/*Treat as C/A/T phototypesetter format*/
#define CMD_RASTER		"v" /*Treat as Sun raster format*/	

/*Constants related to protocol operation*/
#define BUFSIZE			256
#define CMDMAXLEN		256

/*TODO: handle socket failure conditions*/

struct printcomm
{
	char*	queue;
	char	command;
};

/*create a temporary control file */
int build_ctlfile(lpr_flags* job)
{
	int tmpfile = mkstemp("/temp/lptemp.XXXXXX");
	if(0 > tmpfile)
		return -1;
	/*generating host name*/
	write(tmpfile, CMD_HOSTNAME, 1);
	write(tmpfile, job->hostname, strlen(job->hostname));
	write(tmpfile, "\n", 1);
	/*generating user name*/
	write(tmpfile, CMD_USERID, 1);
	write(tmpfile, job->username, strlen(job->username));
	write(tmpfile, "\n", 1);
	/*suppress banner page*/
	if(!job->hflag)
	{
		write(tmpfile, CMD_PRINTBNR, 1);
		write(tmpfile, job->username, strlen(job->username));
		write(tmpfile, "\n", 1);
	}

	lseek(tmpfile, 0, SEEK_SET);
	return tmpfile;
}

/*Send a single daemon command*/
/*returns -1 if the remote did not respond as expected, else zero*/
int send_dcmd(int serv, char* queue, char cmd, char** args, int argc)
{
	int i;
	char sp = ' ';
	
	send(serv, &cmd, 1, 0);
	send(serv, queue, strlen(queue), 0);
	if(0 < argc)
		for(i = 0; i < argc; ++i)
		{
			send(serv, args[i], strlen(args[i]), 0);
			if(i != (argc - 1))
				send(serv, &sp, 1, 0);
		}
	
	if(1 != send(serv, "\n", 1, 0)) /*failure to send*/
		return -1;
	return 0;
}

/*Send a single line in the control file*/
/*returns -1 if the remote did not respond as expected, else zero*/
int send_ctrlline(int serv, char cmd, char* arg)
{
	send(serv, &cmd, 1, 0);
	send(serv, arg, strlen(arg), 0);
	if(1 != send(serv, "\n", 1, 0)) /*failure to send*/
		return -1;
	return 0;
}

/*returns zero on success, -1 otherwise*/
int start_print(int serv, char* queue)
{
	if(0 != send_dcmd(serv, queue, 0x01, NULL, 0))
		return -1;
	return 0;
}

/*returns:
-1 for communication failure
-2 for bad file
-3 for cannot create temp file
else 0*/
int send_job(int serv, char* queue, lpr_flags* job)
{
	char		ack;			/*Acknowledgement byte*/
	char*		numstr;			/*Space for string render of size values*/
	int			dfile, ctlfile;	/*FDs to data and control files*/
	int			dsize, ctlsize;	/*Sizes of data and control files*/
	char		buf[BUFSIZE];	/*Buffer for data passed to network*/
	int			size;			/*Size of data read*/
	int			retval;			/*Success value for return*/
	struct stat	filestat;		/*Used to get file sizes*/

	/*prepare data for job sequence*/
	dfile = open(job->filename, O_RDONLY);
	ctlfile = build_ctlfile(job);
	if(0 > dfile || 0 > fstat(dfile, &filestat))
	{
		close(dfile);
		return -2;
	}
	dsize = filestat.st_size;
	if(0 > ctlfile || 0 > fstat(ctlfile, &filestat))
	{
		close(dfile);
		close(ctlfile);
		return -3;
	}
	ctlsize = filestat.st_size;
	size = dsize > ctlsize ? dsize : ctlsize;
	numstr = malloc(sizeof(char) * (1 + log10(size))); /*room for base-10*/

	retval = 0;
	for(int i = 0; i < job->copies; ++i)
	{
		/*send the recieve-job notification*/
		send(serv, "\x02", 1, 0);
		send(serv, queue, strlen(queue), 0);
		send(serv, "\n", 1, 0);
		if(1 != recv(serv, &ack, 1, 0) || ack != 0x00)
		{
			retval = -1; /*bad ack*/
			break;
		}
	
		/*send data file*/\
		send(serv, "\x03", 1, 0);
		sprintf(numstr, "%d", dsize);
		send(serv, numstr, strlen(numstr), 0);
		send(serv, " dfA", 4, 0);
		sprintf(numstr, "%03d", job->jobnum);
		send(serv, numstr, 3, 0);
		send(serv, job->hostname, strlen(job->hostname), 0);
		send(serv, "\n", 1, 0);
		do {
			size = read(dfile, buf, BUFSIZE);
			send(serv, buf, size, 0);
		} while (size > 0);
		if(1 != recv(serv, &ack, 1, 0) || ack != 0x00)
		{
			retval = -1; /*bad ack*/
			break;
		}

		/*send control file*/
		send(serv, "\x02", 1, 0);
		sprintf(numstr, "%d", ctlsize);
		send(serv, numstr, strlen(numstr), 0);
		send(serv, " cfA", 4, 0);
		sprintf(numstr, "%03d", job->jobnum);
		send(serv, numstr, 3, 0);
		send(serv, job->hostname, strlen(job->hostname), 0);
		send(serv, "\n", 1, 0);
		do {
			size = read(ctlfile, buf, BUFSIZE);
			send(serv, buf, size, 0);
		} while (size > 0);
		if(1 != recv(serv, &ack, 1, 0) || ack != 0x00)
		{
			retval = -1; /*bad ack*/
			break;
		}	
	}
	free(numstr);
	close(dfile);
	close(ctlfile);
	return retval;
}

int connect_lpr(const char* address, unsigned short port, const char* username, const char* password)
{
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

 if(!inet_aton(address, &addr.sin_addr)) {
    fprintf(stderr, "Could not convert \"%s\" to an address\n", address);
    return -1;
  }


 int fd = socket(PF_INET, SOCK_STREAM, 0);
  if(fd == -1) {
    fprintf(stderr, "Call to socket failed because (%d) \"%s\"\n",
            errno, strerror(errno));
    return -1;
  }

 if(connect(fd, (struct sockaddr*)&addr, sizeof(addr))) {
    fprintf(stderr, "Failed to connect to %s:%d because (%d) \"%s\"\n",
            address, port, errno, strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

int print_file_lpr(const int fd, char* queuename, lpr_flags* job){
	int result = send_job(fd, queuename, job);
	if(0 != result)
		return result;
	if(!job->qflag)
		start_print(fd, queuename);
    return 0;
}
