#ifndef PRINT_JOB_H
#define PRINT_JOB_H

#include "common.h"

struct print_extra {
  enum font font_type;
  bool print_control_chars;
};

struct job {
  char **file_names;
  char **mime_types;

  char *email;
  char *username;
  char *hostname;
  char *job_id;
  char *job_name;

  struct printer* p;

  void *extra;
  
  size_t copies;

  bool burst_page;
  bool no_start;

  char pad[7];	/*offering to the clang gods*/
};

struct job_stat{
	char *status;
};

#endif
