#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "ipp.h"
#include "print_job.h"

int main(int argc, char **argv) {
  (void)&argc;
  (void)&argv;
  /* if (argc != 2) { */
  /*   printf("testprint <test.ps>\n"); */
  /*   return 1; */
  /* } */

  char *fNames[] = {"/home/mcgrewz/test.txt", 0};

  struct job *j = (struct job*)malloc(sizeof(struct job));

  j->file_names = fNames;
  
  return ipp_print_file(j);
}
