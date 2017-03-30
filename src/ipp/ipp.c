#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "ipp.h"

int ipp_connect(const char *address, const char *port) {
  int fd = -1;
  int error;
  const char *cause = NULL;
  struct addrinfo hints, *res, *res0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  error = getaddrinfo(address, port, &hints, &res0);

  if (error) {
    errx(1, "%s", gai_strerror(error));
  }

  for (res = res0; res; res = res->ai_next) {
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (fd < 0) {
      cause = "socket";
      continue;
    }
    
    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
      cause = "connect";
      close(fd);
      fd = -1;
      continue;
    }

    /* connected */
    break;
  }

  if (fd < 0) {
    perror(cause);
  }

  return fd;
}

struct ipp_header *ipp_mk_header(int16_t op_stat, int32_t request_id) {
  struct ipp_header *header = (struct ipp_header *) malloc(sizeof(struct ipp_header));

  if (header) {
    /* Current only IPP 1.1 is supported */
    header->major = 1;
    header->minor = 1;

    header->op_stat = op_stat;

    header->request_id = request_id;

    header->tags_used = 0;
    header->tags_size = 0;
    header->tags = 0;
  }

  return header;
}

void ipp_free_header(struct ipp_header *header) {
  if (header && header->tags)
    free(header->tags);
  if (header)
    free(header);
}

bool ipp_header_add_tag(struct ipp_header *header, const enum ipp_tag tag,
                        const char *tag_name, const char *tag_val) {

  struct ipp_header *tmpHdr = 0;
  size_t tag_name_len, tag_val_len;
  size_t extra = 1; /* Always need at least 1 extra byte */
  int16_t tmp;

  if (!header)
    return false;

  tag_name_len = strlen(tag_name);
  tag_val_len = strlen(tag_val);

  if (tag_name_len > 0) {
    extra += 2;
  }
  if (tag_val_len > 0) {
    extra += 2;
  }

  if (header->tags_size - header->tags_used < (tag_name_len + tag_val_len + extra)) {
    /* Allocate space for the tag (and possible values) */
    tmpHdr = (char *) realloc(header->tags, tag_name_len + tag_val_len + extra);

    if (!tmpHdr) {
      return false;
    }
    else if (tmpHdr != header->tags) {
      free(header->tags);
      header->tags = tmpHdr;
    }
    
    header->tags_size = 512;
  }

  *(header->tags + header->tags_used++) = (char) tag;

  if (tag_name_len) {
    /* Length of name + name */
    tmp = (int16_t) htons(tag_name_len);
    memcpy(header->tags + header->tags_used, &tmp, sizeof(tmp));
    header->tags_used += 2;
    memcpy(header->tags + header->tags_used, tag_name, tag_name_len);
    header->tags_used += tag_name_len;
  }

  if (tag_val_len) {
    /* Length of val + val */
    tmp = (int16_t) htons(tag_val_len);
    memcpy(header->tags + header->tags_used, &tmp, sizeof(tmp));
    header->tags_used += 2;
    memcpy(header->tags + header->tags_used, tag_val, tag_val_len);
    header->tags_used += tag_val_len;
  }

  return true;

}

void ipp_getPrinterInfo(int fd) {
  printf("%d\n", fd);
  char httpBuf[512] = {0};
  char tmpBuf[4096] = {0};


  
  
  struct ipp_header *ipp_header = ipp_mk_header(IPP_OP_GET_PRINTER_ATTR, 1);
  ipp_header_add_tag(ipp_header, IPP_TAG_END_ATTR, NULL, NULL);

  
}
