#include <stdio.h>
#include <unistd.h>

#include "ipp.h"

int main(void) {
  int fd;

  if ((fd = ipp_connect("140.160.139.120", "631")) > -1){
    ipp_getPrinterInfo(fd);
    close(fd);
  }

  return 0;
}
