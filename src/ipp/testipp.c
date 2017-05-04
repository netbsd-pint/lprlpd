#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "ipp.h"

int main(int argc, char **argv) {
  int fd;

  if (argc != 2) {
    printf("testprint <test.ps>\n");
    return 1;
  }

  if ((fd = get_connection("140.160.139.120", "631")) > -1){
    ipp_test_print(fd, argv[1]);
    close(fd);
  }

  return 0;
}
