/*
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "lpr_flags.h"
#include "../common/common.h"

static int usage (void);
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

static struct lpr_flags *
parse_commandline (int argc, char **argv)
{
  struct lpr_flags *f = new_lpr_flags ("user1", "localhost");
  extern char *optarg;
  extern int optind;
  char ch[2] = {0};

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

  /* The last argument(s) will be filename(s) */
  while (optind < argc) { /* while there are argeuments remaining */
    if (access (argv[optind], F_OK) != -1) { /* File does exists */
      if (!f->files) { /* Case for the first file encountered */
        f->files = new_job_file_ll (argv[optind]);
      } else {
        job_file_ll_append (f->files, new_job_file_ll (argv[optind]));
      }
    } else {
      printf ("lpr: cannot acces %s: No such file or directory\n", argv[optind]);
    }

    optind++;
  }

  /* TODO: If no files are specified, use stdin */
  /* if (!f->files) {
     mkstemp, fstat from fd to get filename
     (read "asdkjasdlfkj")
     use stdin as source of document
     }

     f->files = new_job_file_ll (tempfilename);
  */

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
    printf ("\t%s\n", tmp->filename);
    tmp = tmp->next;
  }

  printf ("With options:\n");
  if (f) {
    printf ("Jflags: %s\nTflag: %s\nUflag: %s\nfont: %s\nCflag: %s\ncopies: %d\n",
            f->Jflag, f->Tflag, f->Uflag, f->font, f->Cflag, f->copies);
  }
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

  /* TODO: Use the printcap and flags to build a job for the appropriate proto */

  return 0;
}
