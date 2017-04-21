#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

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
    /* Currently only IPP 1.1 is supported */
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

char* ipp_mk_http_request(const char *address, const char *port, const char * ipp_path,
                          const struct ipp_header *header, const size_t file_len, size_t *http_req_len) {
  char *http_request = 0;
  char *pos = 0;

  int used_chars = 0;
  
  size_t http_size = 0;

  int16_t *tmp16;
  int32_t *tmp32;

  if (!address || !port || !ipp_path || !header) {
    return 0;
  }

  /* 104 = 19 (%zu max length) + 16 (POST...) + 9 (Host...) + 31
     (Content-Type...) + 20 (Content-Length...) + 8 (IPP Header w/o
     tags) + 1 (NULL) */
  http_size = strlen(address) + strlen(port) + strlen(ipp_path) + header->tags_used + 104;

  http_request = (char *) malloc(http_size);

  if (!http_request) {
    return 0;
  }

  pos = http_request;

  /* TODO: Make helper function for this */
  
  used_chars = snprintf(pos, http_size, "POST %s HTTP/1.1\r\n", ipp_path);
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  used_chars = snprintf(pos, http_size, "Host: %s:%s\r\n", address, port);
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  
  used_chars = snprintf(pos, http_size, "Content-Type: application/ipp\r\n");
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  used_chars = snprintf(pos, http_size, "Content-Length: %zu\r\n\r\n", 8 + header->tags_used + file_len);
  pos += used_chars;
  http_size -= (unsigned long) used_chars;

  printf("HTTP Size remaining: %zu\n", http_size);

  *pos++ = header->major;
  *pos++ = header->minor;

  /* Silence the alignment warning */
  tmp16 = (int16_t *) (void *) pos;
  *tmp16 = (int16_t) htons(header->op_stat);
  pos += sizeof(int16_t);

  /* Silence the alignment warning */
  tmp32 = (int32_t *) (void *) pos;
  *tmp32 = (int32_t) htonl(header->request_id);
  pos += sizeof(int32_t);

  memcpy(pos, header->tags, header->tags_used);

  *(pos + header->tags_used) = 0;

  *http_req_len = (size_t)((pos + header->tags_used) - http_request);

  return http_request;
}

bool ipp_parse_headers(const char *headers, const size_t headers_len, struct ipp_header *ipp_msg, size_t *file_len) {
  char *end = 0;
  char *ipp_start = 0;

  (void)&headers_len;

  end = strchr(headers, ' ');
  *end = 0;

  if (strncasecmp(headers, "HTTP/", 5)) {
    return false;
  }

  

  file_len = 0;
  
  /* Try to find the start of the IPP header */
  ipp_start = strstr(headers, "\r\n\r\n");

  if (!ipp_start)
    ipp_start = strstr(headers, "\n\r\n\r");

  /* Still failed? Not a valid HTTP request */
  if (!ipp_start)
    return false;

  ipp_msg = 0;
  
  return true;
}

void ipp_test_print(int sockfd, const char *text_file) {
  char *http_request = 0;
  size_t http_req_len = 0;
  char tmp_buf[4096] = {0};

  ssize_t rc = 0;

  int filefd = open(text_file, O_RDONLY);
  size_t file_len = 0;
  struct stat file_stat;

  if (!filefd)
    return;

  if (fstat(filefd, &file_stat) < 0) {
    close(filefd);
    return;
  }

  file_len = (size_t) file_stat.st_size;

  struct ipp_header *ipp_header = ipp_mk_header(IPP_OP_PRINT_JOB, (int32_t) random());

  if (!ipp_header)
    return;

  ipp_header_add_tag(ipp_header, (char)IPP_TAG_OPERATION_ATTR, NULL, NULL);

  ipp_header_add_tag(ipp_header, (char)IPP_TAG_CHARSET, "attributes-charset", "utf-8");
  ipp_header_add_tag(ipp_header, (char)IPP_TAG_NATURAL_LANGUAGE, "attributes-natural-language", "en-us");

  /* TODO: This address:port/path shouldn't be hard coded */
  snprintf(tmp_buf, sizeof(tmp_buf), "http://%s/%s", "140.160.139.120:631", "ipp");
  ipp_header_add_tag(ipp_header, (char)IPP_TAG_URI, "printer-uri", tmp_buf);

  /* TODO: Insert real username here */
  ipp_header_add_tag(ipp_header, (char)IPP_TAG_NAME_WITHOUT_LANG, "requesting-user-name", "mcgrewz");

  /* TODO: Insert real job name here */
  ipp_header_add_tag(ipp_header, (char)IPP_TAG_NAME_WITHOUT_LANG, "job-name", "test");

  /* TODO: Insert actual mime-type here */
  ipp_header_add_tag(ipp_header, (char)IPP_TAG_MIME_TYPE, "document-format", "application/octet-stream");

  ipp_header_add_tag(ipp_header, (char)IPP_TAG_END_ATTR, NULL, NULL);

  /* TODO: This address:port/path shouldn't be hard coded */
  http_request = ipp_mk_http_request("140.160.139.120", "631", "/ipp", ipp_header, file_len, &http_req_len);

  printf("HTTP REQ LEN: %zu\n", http_req_len);

  if (http_request) {
    printf("Generated HTTP Request:\n--------------------------------------------------------------------------------\n%s\n", http_request);

    rc = write(sockfd, http_request, http_req_len);
    printf("Write (HTTP+IPP) RC: %zd\n", rc);

    while ((rc = read(filefd, tmp_buf, sizeof(tmp_buf))) > 0) {
      rc = write(sockfd, tmp_buf, (size_t)rc);
      printf("Write (FILE) RC: %zd\n", rc);
    }

    close(filefd);

    filefd = open("response.bin", O_WRONLY);

    rc = 1;

    while (rc > 0) {
      rc = read(sockfd, tmp_buf, sizeof(tmp_buf));
      if (rc > 0) {
        write(filefd, tmp_buf, (size_t)rc);
        tmp_buf[rc - 1] = 0;
        printf("Read (RC: %zd): %s\n", rc, tmp_buf);
      }
    }

    close(filefd);
    
    free(http_request);
  }

  ipp_free_header(ipp_header);

}

void ipp_get_attributes(int sockfd) {
  sockfd = 0;
  /* TODO: Write me please */
}
