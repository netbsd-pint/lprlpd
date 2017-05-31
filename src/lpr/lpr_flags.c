/* File imeplements new/delete functions for the lpr_flags_st */

#include <string.h>

#include "lpr_flags.h"

struct lpr_flags *
new_lpr_flags(void)
{
  struct lpr_flags *j = (struct lpr_flags*)malloc(sizeof(struct lpr_flags));
  if (j == NULL) {
    printf("Failed to malloc in new_lpr_flags");
    exit (EXIT_FAILURE);
  }

  j->username = NULL;
  j->hostname = NULL;
  j->file_names = NULL;
  j->mime_types = NULL;
  j->cflag = false;
  j->dflag = false;
  j->fflag = false;
  j->gflag = false;
  j->lflag = false;
  j->nflag = false;
  j->oflag = false;
  j->pflag = false;
  j->tflag = false;
  j->hflag = false;
  j->mflag = false;
  j->qflag = false;
  j->rflag = false;
  j->sflag = false;
  j->Rflag = false;
  j->copies = 1;
  j->fontnum = -1;
  j->font = NULL;
  j->Cflag = NULL;
  j->iflag = -1;
  j->Jflag = NULL;
  j->Mflag = NULL;
  j->Pflag = NULL;
  j->Tflag = NULL;
  j->Uflag = NULL;
  j->wflag = -1;

  return j;
}

void
free_lpr_flags(struct lpr_flags *j)
{
  free(j->username);
  free(j->hostname);
  free(j);
}
