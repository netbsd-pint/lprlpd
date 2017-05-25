/*
 */

#include <magic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lpr_flags.h"
#include "../common/printer.h"
#include "../common/common.h"

static int usage (void);
static char * get_mime_type (char *file);
static void file_from_stdin ();
static struct lpr_flags * parse_commandline (int argc, char **argv);
static int verify_lpr_flags (struct lpr_flags *f);
static void print_lpr_flags (struct lpr_flags *f);

static int
usage(void)
{
  fprintf(stderr,
          "Usage: %s [-Pprinter] [-#num] [-C class] [-J job] [-T title] "
          "[-U user]\n"
          "%s [-i[numcols]] [-1234 font] [-wnum] [-cdfghlmnopqRrstv] "
          "[filename ...]\n", getprogname(), getprogname());
  return 1;
}

static char *
get_mime_type (char *file) {
  char *mime = NULL;
  size_t mime_len = 0;
  const char *magic_full = NULL;
  magic_t magic_cookie = magic_open(MAGIC_MIME);

  if (magic_cookie == NULL) {
    printf ("unable to initialize magic library.\n");
    exit (1);
  }

  if (magic_load(magic_cookie, NULL) != 0) {
    printf ("cannot load magic database - %s\n", magic_error(magic_cookie));
    magic_close (magic_cookie);
    exit (1);
  }

  magic_full = magic_file (magic_cookie, file);
  mime_len = strlen (magic_full) + 1;
  mime = (char*) malloc (mime_len);
  if (!mime) {
    printf ("Failed to malloc in get_mime_type.\n");
    exit (1);
  }

  (void) strlcpy (mime, magic_full, mime_len + 1);
  magic_close (magic_cookie);

  return mime;
}

static void
file_from_stdin (char *template)
{
  int fd = -1;
  ssize_t res = -1;
  char buf[4096] = {0};

  fd = mkstemp (template);
  if (fd > -1) { /* file creation successful */
    /* read from stdin */
    printf ("Type data and press enter followed by ctrl-D to signal EOF.\n");
    while ((res = read (0, buf, 4096)) > 0) {
      write (fd, buf, 4096);
    }
    close (fd);
  } else {
    printf ("Failed to make temp file for printing from stdin... exiting.\n");
    exit (1);
  }
}

static struct lpr_flags *
parse_commandline (int argc, char **argv)
{
  /* TODO: Edit the constants "user1" and "localhost" */
  struct lpr_flags *f = new_lpr_flags ("user1", "localhost");
  char template[] = "/tmp/lpr.XXXXXXX";
  extern char *optarg;
  extern int optind;
  char ch[2] = {0};
  int tmp = optind;
  bool read_stdin = false;

  if (!f) {
    printf ("Failed to malloc in parse_commandline.\n");
    exit (1);
  }

  while ((ch[0] = (char) getopt (argc, argv, "#:1:2:3:4:J:T:U:C:i:P:cdfghlmnopqrRstv")) != -1) {
    switch (ch[0]) {
    case '#':
      if (atoi (optarg) < 0) {
        printf ("Cannot specify a negative number of copies to print. Exiting...\n");
        exit (1);
      }
      f->copies = strtoul (optarg, NULL, 10);
      break;
    case '1':
    case '2':
    case '3':
    case '4':
      f->fontnum = atoi (ch);
      f->font = optarg;
      break;
    case 'J':
      f->Jflag = optarg;
      break;
    case 'T':
      f->Tflag = optarg;
      break;
    case 'U':
      f->Uflag = optarg;
      break;
    case 'C':
      f->Cflag = optarg;
      break;
    case 'i':
      f->iflag = atoi (optarg);
      break;
    case 'P':
      f->Pflag = optarg;
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
      exit (1);
    } /* end switch */
  } /* end while */

  /* Determine if printing from stdin OR files but do not allow both
     because it might cause errors when pipping into lpr.
  */
  tmp = optind;
  while (tmp < argc) {
    if (strcmp ("-", argv[tmp]) == 0) {
      read_stdin = true;
    }
    tmp++;
  }

  f->file_names = (char**) calloc (sizeof (char*), 2);
  f->mime_types = (char**) calloc (sizeof (char*), 2);
  if (!f->file_names || !f->mime_types) {
    printf ("Failed to malloc in parse_commandline.\n");
    exit (1);
  }

  if (read_stdin) { /* Just print the input from stdin */
    file_from_stdin (template);
    f->file_names[0] = (char*) calloc (sizeof (char), 17);
    if (!f->file_names[0]) {
      printf ("Failed to malloc for filename in parse_commandline.\n");
      exit (1);
    }

    f->file_names[0] = memcpy (f->file_names[0], template, 17);
    f->mime_types[0] = get_mime_type (template);
  }
  else { /* Print the files supplied by user */
    unsigned long i = 0;
    while (optind < argc) {
      printf ("FILENAME:\t%s\n", argv[optind]);
      if (access (argv[optind], F_OK) != -1) {
        /* File exists */
        f->file_names = (char**) realloc (f->file_names, sizeof (char*) * (i + 2));
        f->mime_types = (char**) realloc (f->mime_types, sizeof (char*) * (i + 2));
        if (!f->file_names || !f->mime_types) {
          printf ("Failed to malloc in file collecting loop in pasre_commandline.\n");
          exit (1);
        }
        f->file_names[i + 2] = NULL;
        f->mime_types[i + 2] = NULL;
        f->file_names[i] = argv[optind];
        f->mime_types[i] = get_mime_type (argv[optind]);
      } else {
        printf ("lpr: cannot acces %s: No such file or directory\n", argv[optind]);
      }
      i++;
      optind++;
    }
  }

  return f;
}

static int
verify_lpr_flags (struct lpr_flags *f)
{
  int status = 0;

  /* DEBUG */
  print_lpr_flags (f);

  if (!f->file_names) {
    printf ("No file(s) specified for print job.\n");
    status++;
  }

  if (f->copies <= 0) {
    printf ("Invalid number of copies specified (%zu).\n", f->copies);
    status++;
  }

  return status;
}

/* test print of the fields of the flags */
static void
print_lpr_flags (struct lpr_flags *f)
{
  int i = 0;

  printf ("Printing file(s):\n");
  while (f->file_names[i]) {
    printf ("\t%s -- MIME: %s\n", f->file_names[i], f->mime_types[i]);
    i++;
  }

  printf ("With options:\n");
  if (f) {
    printf ("Jflags: %s\nTflag: %s\nUflag: %s\nfont: %s\nCflag: %s\ncopies: %zu\nPflag: %s\n",
            f->Jflag, f->Tflag, f->Uflag, f->font, f->Cflag, f->copies, f->Pflag);
  }

}
static void
print_printcap_flags(struct printer *printer)
{
    //test what was set from reading the printcap file

    if(printer->local_printer[0] == '\0'){
      printf ("Local Printer: Null\n");
    } else {
      printf ("Local Printer: %s\n", printer->local_printer);
    }
    printf ("Is it a remote printer?: %s\n", printer->remote_printer);
    printf ("Spooling directory: %s\n", printer->spooling_dir);
    printf ("Lock File?: %s\n",   printer->lock_file);
    printf ("File Status: %s\n", printer->status_file);
    printf ("remote printer host?: %s\n", printer->remote_host);
    printf ("Log file set: %s\n", printer->log_file);
    printf ("Selected protocol: %s\n", (printer->proto == 0) ? "lp" : "ip");
}

/* Entry point of the lpr command line utility
 */
int
main (int argc, char **argv)
{
  int userid = -1;
  int printer_status = -1;
  int flag_errors = 0;
  char hostname[256] = {0};
  char *printername = NULL;
  struct lpr_flags *flags = NULL;
  struct printer *printcap = NULL;
  struct job *print_job = NULL;
  /*
    Therefore, in NetBSD, calling setprogname() explicitly has no effect.
    However, portable programs that wish to use getprogname() should call
    setprogname() from main(). On operating systems where getprogname() and
    setprogname() are implemented via a portability library, this call is
    needed to make the name available.
  */
  setprogname (*argv);
  gethostname (hostname, 256);
  userid = (int) getuid ();

  /* Handle the command line input */
  flags = parse_commandline (argc, argv);
  flag_errors = verify_lpr_flags (flags);
  if (flag_errors > 0) {
    printf ("verify_lpr_flags found %d errors with cli input... exiting.\n", flag_errors);
    usage ();
    exit (1);
  }

  /* try to get a printer or die trying */
  if (flags->Pflag != NULL){
    printername = flags->Pflag;
  } else {
    printername = getenv ("PRINTER");
  }

  if (!printername) {
    printername = strdup ("lp");
    printf ("No printer set in PRINTER environment variable... Defaulting to 'lp'.\n");
  }
  printf("Printer name: %s\n", printername);
  printcap = new_printer (printername);

  /* Attempt to load printer configuration data from printcap */
  printer_status = getprintcap (printcap);
  if (printer_status < 0) {
    /* This message may be redundant to printf in `getprintcap` */
    printf ("Failed to load printcap entry for %s.\n", printcap->name);
    exit (1);
  }
  /* testing purposes */
  print_printcap_flags (printcap);

  print_job = (struct job*) malloc (sizeof (struct job));
  if (!print_job) {
    printf ("Failed to malloc struct job in main.\n");
    exit (1);
  }

  print_job->file_names = flags->file_names;
  print_job->mime_types = flags->mime_types;
  print_job->email = NULL;
  print_job->username = "";
  print_job->hostname = hostname;
  print_job->job_id = 0;
  print_job->job_name = "";
  print_job->p = printcap;
  print_job->extra = NULL;
  print_job->copies = flags->copies;
  print_job->burst_page = flags->Jflag;
  print_job->no_start = flags->qflag;

  return 0;
}
