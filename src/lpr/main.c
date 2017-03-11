
#include <stdio.h>
#include <string.h>
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

lpr_flags *
parse_commandline (int argc, char **argv)
{
  lpr_flags *j = new_lpr_flags ("user1", "localhost");
  extern char *optarg;
  extern int optind;
  int bflag, fd;
  char ch[2] = {0};

  while ((ch[0] = getopt (argc, argv, "#:1:2:3:4:J:T:U:C:i:cdfghlmnopPqrRstv")) != -1) {
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

  /* The last argument(s) will be filename(s) */
  // TODO: Make this a while loop that supports multiple files
  // Also, reminder that the `protocolize` functio will iterate over files, copies
  // making a separate packet for each copy of each file (copies * files) packets
  if (optind <= argc) {
    j->filename = (char *) malloc (strlen (argv[optind]));
    strcpy (j->filename, argv[optind]);
  }

  return j;
}

/* Entry point of the lpr command line utility
*/
int
main (int argc, char **argv)
{
  int status = 0;
  lpr_flags *j = parse_commandline (argc, argv);



  return status;
}
