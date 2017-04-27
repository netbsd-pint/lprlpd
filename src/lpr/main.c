/*
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "lpr_job.h"
#include "../common/common.h"

static int usage (void);
struct lpr_flags * parse_commandline (int argc, char **argv);
void print_lpr_flags (struct lpr_flags *f);

static int
usage(void)
{
  fprintf(stderr,
          "Usage: %s [-Pprinter] [-#num] [-C class] [-J job] [-T title] "
          "[-U user]\n"
          "%s [-i[numcols]] [-1234 font] [-wnum] [-cdfghlmnopqRrstv] "
          "[name ...]\n", getprogname(), getprogname());
  return 1;
}

struct lpr_flags *
parse_commandline (int argc, char **argv)
{
  struct lpr_flags *j = new_lpr_flags ("user1", "localhost");
  extern char *optarg;
  extern int optind;
  char ch[2] = {0};

  if (!j) {
    printf ("Failed to malloc in parse_commandline.\n");
    exit (1);
  }

  while ((ch[0] = (char) getopt (argc, argv, "#:1:2:3:4:J:T:U:C:i:cdfghlmnopPqrRstv")) != -1) {
    switch (ch[0]) {
    case '#':
      j->copies = atoi (optarg);
      break;
    case '1':
    case '2':
    case '3':
    case '4':
      j->fontnum = atoi (ch);
      j->font = optarg;

      break;
    case 'J':
      j->Jflag = optarg;
      break;
    case 'T':
      j->Tflag = optarg;
      break;
    case 'U':
      j->Uflag = optarg;
      break;
    case 'C':
      j->Cflag = optarg;
      break;
    case 'i':
      j->iflag = atoi (optarg);
      break;
    /* the rest of these flags are booleans */
    case 'c': j->cflag = true; break;
    case 'd': j->dflag = true; break;
    case 'f': j->fflag = true; break;
    case 'g': j->gflag = true; break;
    case 'h': j->hflag = true; break;
    case 'l': j->lflag = true; break;
    case 'm': j->mflag = true; break;
    case 'n': j->nflag = true; break;
    case 'o': j->oflag = true; break;
    case 'p': j->pflag = true; break;
    case 'P': j->Pflag = true; break;
    case 'q': j->qflag = true; break;
    case 'r': j->rflag = true; break;
    case 'R': j->Rflag = true; break;
    case 's': j->sflag = true; break;
    case 't': j->tflag = true; break;
    case '?':
    default:
      usage ();
      exit (1);
    } /* end switch */

  } /* end while */

  /* The last argument(s) will be filename(s) */
  // TODO: Make this a while loop that supports multiple files
  // Also, reminder that the `protocolize` function will iterate over files, copies
  // making a separate packet for each copy of each file (copies * files) packets
  if (optind < argc) {
        j->filename = (char *) malloc (strlen (argv[optind]));
    strcpy (j->filename, argv[optind]);
  }

  return j;
}

/* test print of the fields of the flags */
void
print_lpr_flags (struct lpr_flags *f)
{
  printf ("Jflags: %s\nTflag: %s\nUflag: %s\nfont: %s\nCflag: %s\ncopies: %d\n",
          f->Jflag, f->Tflag, f->Uflag, f->font, f->Cflag, f->copies);
}

/* Entry point of the lpr command line utility
*/
int
main (int argc, char **argv)
{
  int userid = -1;
  int printer_status = -1;
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
  userid = (int) getuid ();
  gethostname (hostname, 256);
  flags = parse_commandline (argc, argv);

  print_lpr_flags (flags);

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
    exit (1);
  }

  /* TODO: Use the printcap and flags to build a job for the appropriate proto */


  return 0;
}
