/* File imeplements new/delete functions for the lpr_flags_st */

#include "lpr_job.h"

struct lpr_flags *
new_lpr_flags (char *username, char *hostname)
{
  struct lpr_flags *j = (struct lpr_flags*) malloc (sizeof (struct lpr_flags));
  if (!j) {
    printf ("Failed to malloc in new_lpr_flags");
    exit (1);
  }

  j->username = username;
  j->hostname = hostname;
  j->filemime = NULL;
  j->filename = NULL;
  j->cflag = false;
  j->dflag = false;
  j->fflag = false;
  j->gflag = false;
  j->lflag = false;
  j->nflag = false;
  j->oflag = false;
  j->pflag = false;
  j->tflag = false;
  j->vflag = false;
  j->hflag = false;
  j->mflag = false;
  j->Pflag = false;
  j->qflag = false;
  j->rflag = false;
  j->sflag = false;
  j->Rflag = false;
  j->copies = -1;
  j->fontnum = -1;
  j->font = NULL;
  j->Cflag = NULL;
  j->iflag = -1;
  j->Jflag = NULL;
  j->Tflag = NULL;
  j->Uflag = NULL;
  j->wflag = -1;

  return j;
}

void
delete_lpr_flags (struct lpr_flags *j)
{
  free (j->username);
  free (j->hostname);
  free (j->filemime);
  free (j->filename);
  free (j->font);
  free (j->Cflag);
  free (j->Jflag);
  free (j->Tflag);
  free (j->Uflag);
  free (j);
}
