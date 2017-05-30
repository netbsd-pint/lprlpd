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

  //CF413 Printer
  //char *host = "140.160.139.120";

  //CF405 Printer
  //char *host = "140.160.30.68";

  //CF162 Printer
  char *host = "140.160.30.69";
  
  char *port = "631";
  /* get_address_port("port@host","999", &host, &port); */
  /* printf("Host: %s\nPort: %s\n", host, port); */

  srand((unsigned int)time(NULL) * (unsigned int)getpid());

  ipp_get_attributes(host, port);
  
  return 0;
}
