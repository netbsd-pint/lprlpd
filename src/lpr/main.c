/*
 */

#include <sys/stat.h>

#include <fcntl.h>
#include <magic.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "lpr_flags.h"
#include "../common/common.h"
#include "../common/printer.h"
#include "../ipp/ipp.h"

static int create_job_id(void);
static void file_from_stdin();
static char * get_mime_type(char *file);
static struct lpr_flags * parse_commandline(int argc, char **argv);
static void print_lpr_flags(struct lpr_flags *f);
static void print_printcap_flags(struct printer *printer);
static void usage(void) __dead;
static int verify_lpr_flags(struct lpr_flags *f);

/* Entry point of the lpr command line utility. Commandline arguments are
 * parsed into an `struct lpr_flags` for further processing. The printcap
 * is read and the entry requested by the user is parsed into a structure.
 * Finally, main calls out to one of the protocol libraries to print.
 */
int
main(int argc, char **argv)
{
  int flag_errors = -1;
  int jobid = -1;
  int printer_status = -1;
  int userid = -1;
  char *jid = NULL;
  char *printername = NULL;
  double len = 0;
  struct job *print_job = NULL;
  struct lpr_flags *flags = NULL;
  struct printer *printcap = NULL;
  char hostname[256] = {0};

  setprogname(argv[0]);
  gethostname(hostname, 256);
  userid = (int) getuid ();

  /* Handle the command line input */
  flags = parse_commandline(argc, argv);
  flag_errors = verify_lpr_flags(flags);
  if (flag_errors > 0) {
    printf("verify_lpr_flags found %d errors with cli input... exiting.\n", flag_errors);
    usage();
  }

  /* try to get a printer or die trying */
  if (flags->Pflag != NULL){
    printername = flags->Pflag;
  } else {
    printername = getenv("PRINTER");
  }

  /* Default to trying printer called 'lp' */
  if (printername == NULL) {
    printername = strdup("lp");
    printf("No printer set in PRINTER environment variable and -P not specified... Defaulting to printer named 'lp'.\n");
  }
  printcap = new_printer(printername);

  /* Attempt to load printer configuration data from printcap */
  printer_status = getprintcap(printcap);
  if (printer_status < 0) {
    /* This message may be redundant to printf in `getprintcap` */
    printf("Failed to load printcap entry for %s.\n", printcap->name);
    exit(EXIT_FAILURE);
  }

  /* DEBUG: testing purposes */
  print_printcap_flags(printcap);

  print_job = (struct job*)malloc(sizeof(struct job));
  if (print_job == NULL) {
    printf("Failed to malloc struct job in main.\n");
    exit(EXIT_FAILURE);
  }

  /* Munge data into the print_job structure before shipping off to a protocol */
  jobid = create_job_id();
  len = ceil(log10(jobid));
  jid = (char*)calloc(sizeof(char), (unsigned long)len + 1);
  if (jid == NULL) {
    printf("Failed to malloc in lpr:main.\n");
    exit(EXIT_FAILURE);
  }

  print_job->file_names = flags->file_names;
  print_job->mime_types = flags->mime_types;
  print_job->email = NULL;
  print_job->username = getenv("USER");
  print_job->hostname = hostname;
  print_job->job_id = jid;
  if (flags->Jflag) {
    print_job->job_name = flags->Jflag;
  } else {
    print_job->job_name = print_job->file_names[0];
  }
  print_job->p = printcap;
  print_job->extra = NULL;
  print_job->copies = flags->copies;
  print_job->burst_page = flags->Jflag;
  print_job->no_start = flags->qflag;

  /* Ship off print_job to a protocol */
  /* TODO: make this a conditional? */
  //int r = ipp_print_file (print_job);
  //printf ("HOST: %s ipp_print_file result: %d\n", hostname, r);

  return EXIT_SUCCESS;
}

int
create_job_id(void) {
  int fd = open("/var/spool/job", O_RDWR|O_EXLOCK);
  if (fd < 0) {
    // error out here.
    printf("Job id file not found.\n");
    exit(EXIT_FAILURE);
  }
  char input[10];
  int jid = 0;
  read(fd, input, 9);
  input[9] = 0;
  jid = atoi(input);
  printf("ID is %d, setting new jid to %d\n", jid, jid + 1);
  lseek(fd, 0, SEEK_SET);
  dprintf(fd, "%d", jid + 1);
  close(fd);
  return jid;
}

static void
file_from_stdin(char *template)
{
  int fd = -1;
  ssize_t res = -1;
  char buf[4096] = {0};

  fd = mkstemp(template);
  if (fd > -1) { /* file creation successful */
    /* read from stdin */
    printf("Type data and press enter followed by ctrl-D to signal EOF.\n");
    while ((res = read(0, buf, 4096)) > 0) {
      write(fd, buf, 4096);
    }
    close(fd);
  } else {
    printf("Failed to make temp file for printing from stdin... exiting.\n");
    exit(EXIT_FAILURE);
  }
}

static char *
get_mime_type(char *file) {
  size_t mime_len = 0;
  const char *magic_full = NULL;
  char *mime = NULL;
  magic_t magic_cookie = magic_open(MAGIC_MIME);

  /* Attempt to setup the file magic database */
  if (magic_cookie == NULL) {
    printf("unable to initialize magic library.\n");
    exit(EXIT_FAILURE);
  } else if (magic_load(magic_cookie, NULL) != 0) {
    printf("cannot load magic database - %s\n", magic_error(magic_cookie));
    magic_close(magic_cookie);
    exit(EXIT_FAILURE);
  } else {
    magic_full = magic_file(magic_cookie, file);
    mime_len = strlen(magic_full) + 1;
    mime = (char*)malloc(mime_len);
    if (mime == NULL) {
      printf("Failed to malloc in get_mime_type.\n");
      exit(EXIT_FAILURE);
    }
    (void)strlcpy(mime, magic_full, mime_len + 1);
    magic_close(magic_cookie);
  }

  return mime;
}

static struct lpr_flags *
parse_commandline(int argc, char **argv)
{
  char ch[2] = {0};
  bool read_stdin = false;
  extern char *optarg;
  extern int optind;
  int tmp = optind;
  unsigned long i = 0;
  char template[] = "/tmp/lpr.XXXXXXX";
  /* TODO: Edit the constants "user1" and "localhost" */
  struct lpr_flags *f = new_lpr_flags ("user1", "localhost");

  while ((ch[0] = (char)getopt(argc, argv, "#:1:2:3:4:C:i:J:M:P:T:U:cdfghlmnopqrRstv")) != -1) {
    switch (ch[0]) {
    case '#':
      if (atoi(optarg) < 0) {
        printf("Cannot specify a negative number of copies to print. Exiting...\n");
        exit(EXIT_FAILURE);
      }
      f->copies = strtoul(optarg, NULL, 10);
      break;
    case '1':
    case '2':
    case '3':
    case '4':
      f->fontnum = atoi(ch);
      f->font = optarg;
      break;
    case 'C':
      f->Cflag = optarg;
      break;
    case 'i':
      f->iflag = atoi(optarg);
      break;
    case 'J':
      f->Jflag = optarg;
      break;
    case 'M':
      f->Mflag = optarg;
      break;
    case 'P':
      f->Pflag = optarg;
      break;
    case 'T':
      f->Tflag = optarg;
      break;
    case 'U':
      f->Uflag = optarg;
      break;
      /* the rest of these flags are booleans */
    case 'c': f->cflag = true; break;
    case 'd': f->dflag = true; break;
    case 'f': f->fflag = true; break;
    case 'g': f->gflag = true; break;
    case 'h': f->hflag = true; break;
    case 'l': f->lflag = true; break;
    case 'm': f->mflag = true; break;
    case 'n': f->nflag = true; break;
    case 'o': f->oflag = true; break;
    case 'p': f->pflag = true; break;
    case 'q': f->qflag = true; break;
    case 'r': f->rflag = true; break;
    case 'R': f->Rflag = true; break;
    case 's': f->sflag = true; break;
    case 't': f->tflag = true; break;
    case '?':
    default:
      usage ();
      /* NOTREACHED */
    } /* end switch */
  } /* end while */

  /* Determine if printing from stdin OR files but do not allow both
     because it might cause errors when pipping into lpr.
  */
  if (argv[optind] == 0) {
    read_stdin = true;
  }
  else {
    tmp = optind;
    while (tmp < argc) {
      if (strcmp("-", argv[tmp]) == 0) {
        read_stdin = true;
      }
      tmp++;
    }
  }

  f->file_names = (char**)calloc(sizeof (char*), 2);
  f->mime_types = (char**)calloc(sizeof (char*), 2);
  if (!f->file_names || !f->mime_types) {
    printf("Failed to malloc in parse_commandline.\n");
    exit(EXIT_FAILURE);
  }

  /* Just print the input from stdin */
  if (read_stdin) {
    file_from_stdin(template);
    f->file_names[0] = (char*)calloc(sizeof (char), 17);
    if (!f->file_names[0]) {
      printf("Failed to malloc for filename in parse_commandline.\n");
      exit(EXIT_FAILURE);
    }

    f->file_names[0] = memcpy(f->file_names[0], template, 17);
    f->mime_types[0] = get_mime_type(template);
  }
  /* Print the files supplied by user */
  else {
    while (optind < argc) {
      /* DEBUG: printf ("FILENAME:\t%s\n", argv[optind]); */
      if (access(argv[optind], F_OK) != -1) {
        f->file_names = (char**)realloc(f->file_names, sizeof(char*) * (i + 2));
        f->mime_types = (char**)realloc(f->mime_types, sizeof(char*) * (i + 2));
        if (!f->file_names || !f->mime_types) {
          printf("Failed to malloc in file collecting loop in parse_commandline.\n");
          exit(EXIT_FAILURE);
        }
        f->file_names[i + 2] = NULL;
        f->mime_types[i + 2] = NULL;
        f->file_names[i] = argv[optind];
        if (f->Mflag) {
          f->mime_types[i] = f->Mflag;
        } else {
          f->mime_types[i] = get_mime_type(argv[optind]);
        }
      } else {
        printf("lpr: cannot acces %s: No such file or directory\n", argv[optind]);
      }
      i++;
      optind++;
    }
  }

  return f;
}

/* test print of the fields of the flags */
static void
print_lpr_flags(struct lpr_flags *f)
{
  int i = 0;

  printf("Printing file(s):\n");
  while (f->file_names[i]) {
    printf("\t%s -- MIME: %s\n", f->file_names[i], f->mime_types[i]);
    i++;
  }

  printf("With options:\n");
  if (f) {
    printf("Jflags: %s\nTflag: %s\nUflag: %s\nfont: %s\nCflag: %s\ncopies: %zu\nPflag: %s\n",
           f->Jflag, f->Tflag, f->Uflag, f->font, f->Cflag, f->copies, f->Pflag);
  }

}
static void
print_printcap_flags(struct printer *printer)
{
    //test what was set from reading the printcap file

    if (printer->local_printer[0] == '\0') {
      printf("Local Printer: Null\n");
    } else {
      printf("Local Printer: %s\n", printer->local_printer);
    }
    printf("Is it a remote printer?: %s\n", printer->remote_printer);
    printf("Spooling directory: %s\n", printer->spooling_dir);
    printf("Lock File?: %s\n",   printer->lock_file);
    printf("File Status: %s\n", printer->status_file);
    printf("remote printer host?: %s\n", printer->remote_host);
    printf("Log file set: %s\n", printer->log_file);
    printf("Selected protocol: %s\n", (printer->proto == 0) ? "lp" : "ip");
}

static void
usage(void)
{
  fprintf(stderr,
          "Usage: %s [-Pprinter] [-#num] [-C class] [-J job] [-T title] "
          "[-U user]\n"
          "%s [-i[numcols]] [-1234 font] [-wnum] [-cdfghlmnopqRrstv] "
          "[filename ...]\n", getprogname(), getprogname());
  exit(EXIT_FAILURE);
}

static int
verify_lpr_flags(struct lpr_flags *f)
{
  int status = 0;

  /* DEBUG */
  print_lpr_flags(f);

  if (!f->file_names) {
    printf("No file(s) specified for print job.\n");
    status++;
  }

  if (f->copies <= 0) {
    printf("Invalid number of copies specified (%zu).\n", f->copies);
    status++;
  }

  return status;
}
