#include <stdio.h>
#include <time.h>
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

  //Home Printer
  char *host = "DCP7065DN.local";
  
  char *port = "631";
  /* get_address_port("port@host","999", &host, &port); */
  /* printf("Host: %s\nPort: %s\n", host, port); */

  srand((unsigned int)time(NULL) * (unsigned int)getpid());

  ipp_get_attributes(host, port);
  
  return 0;
}
