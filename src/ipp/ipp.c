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

bool ipp_header_add_tag(struct ipp_header *header, const char tag,
                        const char *tag_name, const char *tag_val) {

  size_t tag_name_len = 0;
  size_t tag_val_len = 0;
  size_t tag_size_needed = 0;
  size_t extra = 1; /* Always need at least 1 extra byte */
  
  int16_t tmp;

  char *tmp_tags = 0;

  if (!header) {
    return false;
  }

  if (tag_name) {
    tag_name_len = strlen(tag_name);
  }

  if (tag_val) {
    tag_val_len = strlen(tag_val);
  }

  if (tag_name_len > 0) {
    extra += 2;
  }
  
  if (tag_val_len > 0) {
    extra += 2;
  }

  tag_size_needed = tag_name_len + tag_val_len + extra;

  if (header->tags_size - header->tags_used < (tag_size_needed)) {
    /* Allocate space for the tag (and possible values) */
    tmp_tags = (char *) realloc(header->tags, header->tags_size + tag_size_needed);

    if (!tmp_tags) {
      return false;
    }
    else if (tmp_tags != header->tags) {
      free(header->tags);
      header->tags = tmp_tags;
    }
    
    header->tags_size += tag_size_needed;
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

char* ipp_generate_http_request(const char *address, const char *port, const char * ipp_path,
                                const struct ipp_header *header) {
  char *http_request = 0;

  char *pos = 0;

  int used_chars = 0;
  
  size_t http_size = 0;
  /* size_t full_length = 0; */
  size_t FIXME = 512;

  int16_t *tmp16;
  int32_t *tmp32;

  if (!address || !port || !ipp_path || !header) {
    return 0;
  }

  /* TODO: Fix the FIXME size calculation here! */
  /* Need more space for IPP header data + HTTP stuff */
  http_size = strlen(address) + strlen(port) + strlen(ipp_path) + header->tags_used + FIXME;

  http_request = (char *) malloc(http_size);

  if (!http_request) {
    return 0;
  }

  pos = http_request;

  /* TODO: Make helper function for this */
  /* %zu = 19 chars max */
  /* 19 + strlen(address) + strlen(port) + strlen(ipp_path) + 1 for ':' +
     All of HTTP header text including \r\n's*/
  
  used_chars = snprintf(pos, http_size, "POST %s HTTP/1.1\r\n", ipp_path);
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  used_chars = snprintf(pos, http_size, "Host: %s:%s\r\n", address, port);
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  used_chars = snprintf(pos, http_size, "Content-Type: application/ipp\r\n");
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  used_chars = snprintf(pos, http_size, "Content-Length: %zu\r\n\r\n", header->tags_used);
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  *pos++ = header->major;
  *pos++ = header->minor;

  tmp16 = (int16_t *) pos;
  *tmp16 = (int16_t) htons(header->op_stat);
  pos += sizeof(int16_t);

  tmp32 = (int32_t *) pos;
  *tmp32 = (int32_t) htonl(header->request_id);
  pos += sizeof(int32_t);

  memcpy(pos, header->tags, header->tags_used);

  *(pos + header->tags_used) = 0;

  return http_request;
}

void ipp_test_print(int fd) {
  printf("%d\n", fd);

  char *http_request = 0;
  char tmpBuf[4096] = {0};

  ssize_t rc = 0;

  struct ipp_header *ipp_header = ipp_mk_header(IPP_OP_GET_PRINTER_ATTR, 1);

  if (!ipp_header)
    return;

  ipp_header_add_tag(ipp_header, (char)IPP_OP_PRINT_JOB, NULL, NULL);
  
  ipp_header_add_tag(ipp_header, (char)IPP_TAG_END_ATTR, NULL, NULL);

  http_request = ipp_generate_http_request("140.160.139.120", "631", "/ipp", ipp_header);

  if (http_request) {
    printf("Generated HTTP Request:\n--------------------------------------------------------------------------------\n%s\n", http_request);

    rc = write(fd, http_request, strlen(http_request));
    printf("Write RC: %zd\n", rc);

    while (rc > 0) {
      rc = read(fd, tmpBuf, sizeof(tmpBuf));
      printf("Read (RC: %zd): %s\n", rc, tmpBuf);
    }
    
    free(http_request);
  }

  ipp_free_header(ipp_header);

}
