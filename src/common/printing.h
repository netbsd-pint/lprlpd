/* Header file for lpr protocol and cli structures */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

typedef
struct lpr_flags_st {
  char *username;
  char *hostname;
  char *filemime;
  char *filename;
  bool cflag; /* The files are assumed to contain data produced by cifplot */
  bool dflag; /* The files are assumed to contain data from tex (DVI) */
  bool fflag; /* Use a filter which interprets the first character of each line as a standard FORTRAN carriages control character. */
  bool gflag; /* The files are assumed to containt standard plot data as produced by the plot routines. */
  bool lflag; /* Use a filter which allows control characters to be printed and suppress page breaks. */
  bool nflag; /* The file are assumed to contain data from ditroff (device independent troff). */
  bool oflag; /* The files are assumed to be in postscript format. */
  bool pflag; /* Use pr(1) to fornat the files (equivalent to print). */
  bool tflag; /* The files are assumed to contain data from troff(1) (cat phototypesetter commands). */
  bool vflag; /* The files are assumed to contain a raster image for devices like the Benson Varian. */
  /* The rest of the flags only apply to handling of the print job */
  bool hflag; /* Suppress the printing of the burst page. */
  bool mflag; /* Send mail upon complettion. */
  bool Pflag; /* Force output to a specific printer. Normally, the default printer is used or the value of the environment variable PRINTER. */
  bool qflag; /* Queue the print job but do not start the spooling daemom. */
  bool rflag; /* Remove the file ipon completion of spooling or upon completion of printing (with -s option). */
  bool sflag; /* Use symlinks. Usually files are copied to the spool directory. */
  bool Rflag; /* Writes a message to stdout containing the unique number which is used to identify this job. */
  uint copies; /* How many copies to print. */
  /* The following two fields have to do with fonts */
  uint fontnum;
  char *font; /* Specifies a font to be mounted on font position i. */
  char *Cflag; /* A c string: Job classification to use on the burst page. */
  uint iflag; /* the output is indented by numcols. */
  char *Jflag; /* Job name to print on the burst page. Normally the file's name is used. */
  char *Tflag; /* Title name for pr(1), instead of file name. */
  char *Uflag; /* user name to print on the burst page, also for accounting purposes. */
  uint wflag; /* page width for pr(1) */
} lpr_flags;
