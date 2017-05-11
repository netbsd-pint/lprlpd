/*
 */

#include <magic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lpr_flags.h"
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
  char template[] = "/tmp/lpr.XXXXXXXX";
  extern char *optarg;
  extern int optind;
  char ch[2] = {0};
  int tmp = optind;
  bool read_stdin = false;

  if (!f) {
    printf ("Failed to malloc in parse_commandline.\n");
    exit (1);
  }

  while ((ch[0] = (char) getopt (argc, argv, "#:1:2:3:4:J:T:U:C:i:cdfghlmnopPqrRstv")) != -1) {
    switch (ch[0]) {
    case '#':
      f->copies = atoi (optarg);
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
    case 'P': f->Pflag = true; break;
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

  if (read_stdin) { /* Just print the input from stdin */
    file_from_stdin (template);
    f->files = new_job_file_ll (template, get_mime_type (template));
  } else { /* Print the files supplied by user */
    while (optind < argc) {
      printf ("FILENAME:\t%s\n", argv[optind]);
      if (access (argv[optind], F_OK) != -1) {
        /* File exists */
        if (!f->files) { /* Case for the first file encountered */
          f->files = new_job_file_ll (argv[optind], get_mime_type (argv[optind]));
        } else {
          job_file_ll_append (f->files, new_job_file_ll (argv[optind], get_mime_type (argv[optind])));
        }
      } else {
        printf ("lpr: cannot acces %s: No such file or directory\n", argv[optind]);
      }
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

  if (!f->files) {
    printf ("No file(s) specified for print job.\n");
    status++;
  }

  if (f->copies <= 0) {
    printf ("Invalid number of copies specified (%d).\n", f->copies);
    status++;
  }

  return status;
}

/* test print of the fields of the flags */
static void
print_lpr_flags (struct lpr_flags *f)
{
  struct job_file_ll *tmp = f->files;

  printf ("Printing file(s):\n");
  while (tmp) {
    printf ("\t%s -- MIME: %s\n", tmp->filename, tmp->filemime);
    tmp = tmp->next;
  }

  /*
  printf ("With options:\n");
  if (f) {
    printf ("Jflags: %s\nTflag: %s\nUflag: %s\nfont: %s\nCflag: %s\ncopies: %d\n",
            f->Jflag, f->Tflag, f->Uflag, f->font, f->Cflag, f->copies);
  }
  */
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
  printername = getenv ("PRINTER");
  if (!printername) {
    printername = strdup("lp");
    printf ("No printer set in PRINTER environment variable... Defaulting to 'lp'.\n");
  }
  printcap = new_printer (printername);

  /* Attempt to load printer configuration data from printcap */
  printer_status = getprintcap (printcap);
  if (printer_status < 0) {
    /* This message may be redundant to printf in `getprintcap` */
    printf ("Failed to load printcap entry for %s.\n", printcap->name);
    exit (1);
  }

  /* TODO: Use the printcap and flags to build a job for the appropriate proto

        //email & jobname & extra can all be NULL. The rest have to be all be set.
      struct job {
      char **file_names;
      char **mime_types;

      char *email;
      char *username;
      char *hostname;
      char *job_id;   //random number (has to be a number in ascii text)
      char *job_name;

      struct printer* p;

      void *extra;   // Contains extra data depends on mimetype

      size_t copies;

      bool burst_page;
      };
    */

  return 0;
}
