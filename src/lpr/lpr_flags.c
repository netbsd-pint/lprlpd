/* File imeplements new/delete functions for the lpr_flags_st */

#include <string.h>

#include "lpr_flags.h"

struct job_file_ll *
new_job_file_ll (char *filename, char *filemime)
{
  struct job_file_ll *jf = (struct job_file_ll*) malloc (sizeof (struct job_file_ll));
  size_t fname_len = strlen (filename) + 1;

  if (jf) {
    jf->filename = (char*) malloc (fname_len);
    (void) strlcpy (jf->filename, filename, fname_len);
    jf->filemime = filemime;
    jf->next = NULL;
  } else {
    printf ("Failed to malloc in new_job_file_ll.\n");
    exit (1);
  }

  return jf;
}

void
free_job_file_ll (struct job_file_ll *jf)
{
  struct job_file_ll *tmp = NULL;

  while (jf) {
    tmp = jf->next;
    free (jf->filename);
    free (jf->filemime);
    free (jf);
    jf = tmp;
  }
}

/* Appends the n to l */
void
job_file_ll_append (struct job_file_ll *l, struct job_file_ll *n)
{
  struct job_file_ll *tmp = NULL;

  while (l) {
    tmp = l;
    l = l->next;
  }
  tmp->next = n;
}

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
  j->files = NULL;
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
  j->Pflag = NULL;
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
  free_job_file_ll (j->files);
  free (j->font);
  free (j->Cflag);
  free (j->Jflag);
  free (j->Tflag);
  free (j->Uflag);
  free (j);
}
