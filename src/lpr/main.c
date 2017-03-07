
#include <stdio.h>
#include <unistd.h>

#include "../common/lpr_job.h"

static void
usage(void)
{
  fprintf(stderr,
          "Usage: %s [-Pprinter] [-#num] [-C class] [-J job] [-T title] "
          "[-U user]\n"
          "%s [-i[numcols]] [-1234 font] [-wnum] [-cdfghlmnopqRrstv] "
          "[name ...]\n", getprogname(), getprogname());
  exit(1);
}

/* Entry point of the lpr command line utility
*/
int
main (int argc, char **argv)
{
  int status = 0;
  lpr_flags *j = new_lpr_flags ("user1", "localhost");
  extern char *optarg;
  extern int optind;
  int bflag, ch, fd;

  while ((ch = getopt (argc, argv, "J:T:U:C:i:cdfghlmnopPqrRstv")) != -1) {
    printf ("got: %s\n", optarg);

    switch (ch) {
    case 'J':
      break;
    case 'T':
      break;
    case 'U':
      break;
    case 'C':
      break;
    case 'i':
      break;
      /* the rest of these flags are booleans */
    case 'c':
      j->cflag = true;
      break;
    case 'd':
      j->dflag = true;
      break;
    case 'f':
      j->fflag = true;
      break;
    case 'g':
      j->gflag = true;
      break;
    case 'h':
      j->hflag = true;
      break;
    case 'l':
      j->lflag = true;
      break;
    case 'm':
      j->mflag = true;
      break;
    case 'n':
      j->nflag = true;
      break;
    case 'o':
      j->oflag = true;
      break;
    case 'p':
      j->pflag = true;
      break;
    case 'P':
      j->Pflag = true;
      break;
    case 'q':
      j->qflag = true;
      break;
    case 'r':
      j->rflag = true;
      break;
    case 'R':
      j->Rflag = true;
      break;
    case 's':
      j->sflag = true;
      break;
    case 't':
      j->tflag = true;
      break;
    case 'v':
      j->vflag = true;
      break;
    case '?':
    default:
      usage ();
      exit (1);
    } /* end switch */

  } /* end while */

  return status;
}
