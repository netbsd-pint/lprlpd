#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common.h"
#include "ipp.h"

#define ISNEWLINE(x) (x == '\n' || x == '\r')
#define ISWS(x) (x == ' ' || x == '\t' || ISNEWLINE(x))

static char * getline_buf(char *buf) {
  if (!buf)
    return 0;

  while (*buf && !ISNEWLINE(*buf))
    ++buf;

  /* Strip the \r\n or just the \n */
  if (*buf && ISNEWLINE(*buf)) {
    *buf++ = 0;
    if (*buf && ISNEWLINE(*buf))
      *buf++ = 0;
  }

  return buf;
}

static char * match_token(char *buf, char *token) {
  size_t token_len = strlen(token);
  size_t buf_len = strlen(buf);

  if (buf_len < token_len)
    return 0;

  if (strncasecmp(buf, token, token_len))
    return 0;

  buf = buf + token_len;

  if (!*buf || !ISWS(*buf))
    return 0;

  while (*buf && ISWS(*buf))
    ++buf;

  return buf;
}

static struct ipp_wire_header *ipp_mk_wire_header(int16_t op_stat, int32_t request_id) {
  struct ipp_wire_header *header = (struct ipp_wire_header *) malloc(sizeof(struct ipp_wire_header));

  if (header) {
    /* Currently only IPP 1.1 is supported */
    header->major = 1;
    header->minor = 1;

    header->op_stat = (int16_t)op_stat;

    header->request_id = (int32_t)request_id;

    header->tags_used = 0;
    header->tags_size = 0;
    header->tags = 0;
  }

  return header;
}

static void ipp_free_wire_header(struct ipp_wire_header *header) {
  if (header && header->tags)
    free(header->tags);
  if (header)
    free(header);
}

static bool ipp_wire_header_add_tag(struct ipp_wire_header *header, const char tag,
                        const char *tag_name, const char *tag_val) {

  size_t tag_name_len = 0;
  size_t tag_val_len = 0;
  size_t tag_size_needed = 0;
  size_t extra = 1; /* Always need at least 1 extra byte */

  int16_t tmp;

  char *tmp_tags = 0;

  if (!header)
    return false;

  if (tag_name)
    tag_name_len = strlen(tag_name);

  if (tag_val)
    tag_val_len = strlen(tag_val);

  if (tag_name_len > 0)
    extra += 2;

  if (tag_val_len > 0)
    extra += 2;

  tag_size_needed = tag_name_len + tag_val_len + extra;

  if (header->tags_size - header->tags_used < (tag_size_needed)) {
    /* Allocate space for the tag (and possible values) */
    tmp_tags = (char *) realloc(header->tags, header->tags_size + tag_size_needed);

    if (!tmp_tags)
      return false;
    else if (tmp_tags != header->tags)
      header->tags = tmp_tags;

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

static char* ipp_mk_http_request(const char *address, const char *port, const char * ipp_path,
                          const struct ipp_wire_header *header, const size_t file_len, size_t *http_req_len) {
  char *http_request = 0;
  char *pos = 0;

  int used_chars = 0;

  size_t http_size = 0;

  int16_t *tmp16;
  int32_t *tmp32;

  if (!address || !port || !ipp_path || !header)
    return 0;

  /* 104 = 19 (%zu max length) + 16 (POST...) + 9 (Host...) + 31
     (Content-Type...) + 20 (Content-Length...) + 8 (IPP Header w/o
     tags) + 1 (NULL) */
  http_size = strlen(address) + strlen(port) + strlen(ipp_path) + header->tags_used + 104;

  http_request = (char *) malloc(http_size);

  if (!http_request)
    return 0;

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

static bool ipp_parse_ipp_header_tags(char *header, const size_t header_len, struct ipp_wire_header *ipp_msg, size_t *file_len) {
  char *next = header;
  char *end = header + header_len;
  char *attr;
  char *value;

  size_t len;
  size_t count;
  size_t total_count;

  int i = 0;
  int inCollection = 0;

  int32_t int32_value;
  int16_t int16_value;
  int8_t int8_value;

  int32_t val1;
  int32_t val2;

  /* TODO: Actually set the header? */
  (void)&ipp_msg;
  (void)&file_len;

  while (next < end) {
    if (*next == IPP_TAG_END_ATTR)
      break;

    switch (*next) {
      case IPP_TAG_OPERATION_ATTR:
        ++next;
        printf("Switching to Operation attributes!\n");
        i = 0;
        while (i < 2) {
          /*
            TAG_CHARSET and TAG_NATURAL_LANGUAGE can be in any order,
            but they must be ONLY these two for TAG_OPERATION_ATTR
          */
          switch(*next) {
            case IPP_TAG_CHARSET:
              ++next;
              len = (size_t) ntohs(*((uint16_t *) (void *) next));
              next += 2;
              attr = (char *) malloc(++len);
              strlcpy(attr, next, len);
              next += --len;
              len = (size_t) ntohs(*((uint16_t *) (void *) next));
              next += 2;
              value = (char *) malloc(++len);
              strlcpy(value, next, len);
              next += --len;
              printf("(TAG_CHARSET) %s: %s\n", attr, value);
              free(attr);
              free(value);
              break;
            case IPP_TAG_NATURAL_LANGUAGE:
              ++next;
              len = (size_t) ntohs(*((uint16_t *) (void *) next));
              next += 2;
              attr = (char *) malloc(++len);
              strlcpy(attr, next, len);
              next += --len;
              len = (size_t) ntohs(*((uint16_t *) (void *) next));
              next += 2;
              value = (char *) malloc(++len);
              strlcpy(value, next, len);
              next += --len;
              printf("(TAG_NATURAL_LANGUAGE) %s: %s\n", attr, value);
              free(attr);
              free(value);
              break;
            default:
              /* ERROR */
              printf("ERROR!\n");
              return false;
          }
          ++i;
        }
        continue;

      case IPP_TAG_JOB_ATTR:
        ++next;
        printf("Switching to Job attributes!\n");
        break;
      case IPP_TAG_PRINT_ATTR:
        ++next;
        printf("Switching to Printer attributes!\n");
        break;
      case IPP_TAG_UNSUPPORTED_ATTR:
        ++next;
        printf("Switching to Unsupported attributes!\n");
        break;
    }

    /* TODO: This switch statement needs to be cleaned up */
    /* TODO: There's still some tags I (haven't seen in a packet
       capture) that I don't parse yet -Zach (05/10/2017)*/

    switch (*next) {
      case IPP_TAG_UNKNOWN:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        printf("(TAG_UNKNOWN) %s\n", attr);
        free(attr);

        if (len != 0) {
          /* TODO: FIXME? */
          printf("unknown tag with a non-zero size value?\n");
          exit(1);
        }
        break;
      case IPP_TAG_INTEGER:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        switch (len) {
          case 1:
            int8_value = (int8_t) *((uint32_t *) (void *) next);
            ++next;
            printf("(TAG_INTEGER [1]) %s: %"PRIi8"\n", attr, int8_value);
          case 2:
            int16_value = (int16_t) ntohs(*((uint16_t *) (void *) next));
            next += 2;
            printf("(TAG_INTEGER [2]) %s: %"PRIi16"\n", attr, int16_value);
          case 4:
            int32_value = (int32_t) ntohl(*((uint32_t *) (void *) next));
            next += 4;
            printf("(TAG_INTEGER [4]) %s: %"PRIi32"\n", attr, int32_value);
            break;
          default:
            dprintf(STDERR_FILENO, "ERROR: Integer length is not 1, 2, or 4!\n");
            break;
        }
        free(attr);
        break;
      case IPP_TAG_BOOLEAN:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_BOOLEAN) %s: %s\n", attr, value ? "true" : "false");
        free(attr);
        free(value);
        break;
      case IPP_TAG_ENUM:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        switch (len) {
          case 1:
            int8_value = (int8_t) *((uint32_t *) (void *) next);
            ++next;
            printf("(TAG_ENUM [1]) %s: %"PRIi8"\n", attr, int8_value);
          case 2:
            int16_value = (int16_t) ntohs(*((uint16_t *) (void *) next));
            next += 2;
            printf("(TAG_ENUM [2]) %s: %"PRIi16"\n", attr, int16_value);
          case 4:
            int32_value = (int32_t) ntohl(*((uint32_t *) (void *) next));
            next += 4;
            printf("(TAG_ENUM [4]) %s: %"PRIi32"\n", attr, int32_value);
            break;
          default:
            dprintf(STDERR_FILENO, "ERROR: Enum Integer length is not 1, 2, or 4!\n");
            break;
        }
        free(attr);
        break;
      case IPP_TAG_RESOLUTION:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        if (len == 9) {
          val1 = (int32_t) ntohl(*((uint32_t *) (void *) next));
          next += 4;
          val2 = (int32_t) ntohl(*((uint32_t *) (void *) next));
          next += 4;
          int8_value = (int8_t) *((uint8_t *) (void *) next);
          ++next;
        }
        else {
          /* TODO: FIXME?!?! */
          printf("Invalid resolution format!\n");
          exit(1);
        }
        free(attr);
        printf("(TAG_RESOLUTION [%zu]) %s: %"PRIi32"x%"PRIi32" (Unit: %"PRIi8")\n", len, attr, val1, val2, int8_value);
        break;
      case IPP_TAG_INT_RANGE:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        if (len == 8) {
          val1 = (int32_t) ntohl(*((uint32_t *) (void *) next));
          next += 4;
          val2 = (int32_t) ntohl(*((uint32_t *) (void *) next));
          next += 4;
        }
        else {
          /* TODO: FIXME?!?! */
          printf("Invalid int range format!\n");
          exit(1);
        }
        free(attr);
        printf("(TAG_INT_RANGE [%zu]) %s: %"PRIi32" to %"PRIi32"\n", len, attr, val1, val2);
        break;
      case IPP_TAG_TEXT_WITHOUT_LANG:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_TEXT_WITHOUT_LANG) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_NAME_WITHOUT_LANG:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_NAME_WITHOUT_LANG) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_KEYWORD:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        printf("LEN: %zu:", len);
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        printf("%zu\n", len);
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_KEYWORD) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_URI:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_URI) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_URI_SCHEME:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_URI_SCHEME) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_CHARSET:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_CHARSET) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_NATURAL_LANGUAGE:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_NATURAL_LANGUAGE) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_MIME_TYPE:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_MIME_TYPE) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_BEGIN_COLLECTION:
        ++next;
        ++inCollection;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_BEGIN_COLLECTION) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      case IPP_TAG_TEXT_WITH_LANG:
        ++next;
        count = 0;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        total_count = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        printf("(TAG_TEXT_WITH_LANG [%zu]) %s:", total_count, attr);
        free(attr);
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        count += len + 2;
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        count += len + 2;
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf(" %s:%s\n", attr, value);
        free(attr);
        free(value);
        if (count != total_count) {
          /* TODO: FIXME?!?! */
          printf("text_with_lang count != total_count! (%zu != %zu)\n", count, total_count);
          exit(1);
        }
        break;
      case IPP_TAG_NAME_WITH_LANG:
        ++next;
        count = 0;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        total_count = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        printf("(TAG_NAME_WITH_LANG [%zu]) %s:", total_count, attr);
        free(attr);
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        count += len + 2;
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        count += len + 2;
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf(" %s:%s\n", attr, value);
        free(attr);
        free(value);
        if (count != total_count) {
          /* TODO: FIXME?!?! */
          printf("name_with_lang count != total_count! (%zu != %zu)\n", count, total_count);
          exit(1);
        }
        break;
      case IPP_TAG_END_COLLECTION:
        ++next;
        if (inCollection) {
          --inCollection;
          len = (size_t) ntohs(*((uint16_t *) (void *) next));
          next += 2;
          attr = (char *) malloc(++len);
          strlcpy(attr, next, len);
          next += --len;
          len = (size_t) ntohs(*((uint16_t *) (void *) next));
          next += 2;
          value = (char *) malloc(++len);
          strlcpy(value, next, len);
          next += --len;
          printf("(TAG_END_COLLECTION) %s: %s\n", attr, value);
          free(attr);
          free(value);
        }
        else {
          printf("Got end of collection tag, but not in a collection!\n");
          return false;
        }
        break;
      case IPP_TAG_MEMB_ATTR_NAME:
        ++next;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        attr = (char *) malloc(++len);
        strlcpy(attr, next, len);
        next += --len;
        len = (size_t) ntohs(*((uint16_t *) (void *) next));
        next += 2;
        value = (char *) malloc(++len);
        strlcpy(value, next, len);
        next += --len;
        printf("(TAG_MEMB_ATTR_NAME) %s: %s\n", attr, value);
        free(attr);
        free(value);
        break;
      default:
        printf("In collection? %s\n", (inCollection ? "true" : "false"));
        printf("Unsupported tag type: 0x%02x\n", (int) *next);
        return false;
    }
  }

  if (*next == IPP_TAG_END_ATTR) {
    printf("Done!\n");
  }
  else {
    printf("Tags didn't end with END_ATTR tag!\n");
    return false;
  }

  return true;
}

static bool ipp_parse_ipp_header(char *header, const size_t header_len, struct ipp_wire_header *ipp_msg, size_t *file_len) {
  char *next = header;
  char *end = header + header_len;

  int16_t status;
  int32_t request_id;

  if (!next || !(*next)) {
    printf("ERROR: End of buffer\n");
    return false;
  }

  /* TODO: Put protocol version check back in place */
  /* if (*next != 0x01 && *(next+1) != 0x01) { */
  /*   printf("IPP version mismatch\n"); */
  /*   return false; */
  /* } */

  /* printf("IPP Version: 1.1\n"); */

  next += 2;

  status = (int16_t) ntohs(*((uint16_t *) (void *) next));
  next += 2;


  if (IPP_STATUS_SUCCESS(status))
    printf("Status: SUCCESS");
  else if (IPP_STATUS_INFO(status))
    printf("Status: INFO");
  else if (IPP_STATUS_REDIRECT(status))
    printf("Status: REDIRECT");
  else if (IPP_STATUS_CLI_ERR(status))
    printf("Status: CLIENT ERROR");
  else if (IPP_STATUS_SRV_ERR(status))
    printf("Status: SERVER ERROR");
  else
    printf("Status: UNKNOWN CODE");

  printf(" - 0x%04x\n", status);

  request_id = (int32_t) ntohl(*((uint32_t *) (void *) next));
  next += 4;
  printf("Request ID: 0x%04x\n", request_id);

  return ipp_parse_ipp_header_tags(next, (size_t)(end - next), ipp_msg, file_len);
}

static bool ipp_parse_headers(char *headers, const size_t header_len, struct ipp_wire_header *ipp_msg, size_t *file_len) {
  char *start = headers;
  char *end = headers + header_len;
  char *next;
  char *match;

  unsigned long long content_length = 0;

  /* TODO: Actually set the IPP header? */
  (void)&ipp_msg;

  /* Basic check for HTTP response */
  if (strncasecmp(headers, "HTTP/", 5))
    return false;

  /* Check each line to get the results */
  next = getline_buf(start);

  while (*start) {
    printf("Line: %s\n", start);

    if ((match = match_token(start, "Content-Length:"))) {
      printf("Content-Length was %llu\n", strtoull(match, 0, 10));
      content_length = strtoull(match, 0, 10);
    }
    else if ((match = match_token(start, "Content-Type:"))) {
      printf("Content-Type was %s\n", match);
      if (strncasecmp(match, "application/ipp", 15))
        return false;
    }

    start = next;
    next = getline_buf(next);
  }

  /* next now points to the start of the IPP header */

  *file_len = 0;

  return ipp_parse_ipp_header(next, (size_t)(end - next), ipp_msg, file_len);
}

int ipp_print_file(const struct job *j) {
  char *http_request = 0;
  size_t http_req_len = 0;
  char tmp_buf[4096] = {0};

  char *host = 0;
  char *port = 0;

  char **file_name = j->file_names;

  ssize_t rc = 0;

  int filefd;
  int sockfd;
  size_t file_len = 0;
  struct stat file_stat;

  while (*file_name) {

    filefd = open(*file_name, O_RDONLY);

    if (!filefd)
      return -1;

    if (fstat(filefd, &file_stat) < 0) {
      close(filefd);
      return -1;
    }

    file_len = (size_t) file_stat.st_size;

    struct ipp_wire_header *ipp_header = ipp_mk_wire_header(IPP_OP_PRINT_JOB, (int32_t) rand());

    if (!ipp_header)
      return -1;

    get_address_port(j->p->remote_host, "631", &host, &port);

    sockfd = get_connection(host, port);

    if (sockfd == -1) {
      close(filefd);
      free(ipp_header);
      free(host);
      free(port);
      return -1;
    }

    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_OPERATION_ATTR, NULL, NULL);

    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_CHARSET, "attributes-charset", "utf-8");
    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_NATURAL_LANGUAGE, "attributes-natural-language", "en-us");

    /* TODO: Path shouldn't be hardcoded */
    snprintf(tmp_buf, sizeof(tmp_buf), "http://%s:%s/%s", host, port, "ipp");
    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_URI, "printer-uri", tmp_buf);

    /* TODO: Insert real username here */
    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_NAME_WITHOUT_LANG, "requesting-user-name", "mcgrewz");

    /* TODO: Insert real job name here */
    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_NAME_WITHOUT_LANG, "job-name", "test");

    /* TODO: Insert actual mime-type here, after confirming support */
    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_MIME_TYPE, "document-format", "application/octet-stream");

    ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_END_ATTR, NULL, NULL);

    /* TODO: This path (/ipp) shouldn't be hard coded */
    http_request = ipp_mk_http_request(host, port, "/ipp", ipp_header, file_len, &http_req_len);

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

    ipp_free_wire_header(ipp_header);

    ++file_name;
    close(sockfd);
    free(host);
    free(port);
  }

  return 0;
}

void ipp_get_attributes(const char *host, const char *port) {
  char *http_request = 0;
  char *tmp_buf;
  char *read_buf;
  char *path = "/ipp"; /* TODO: Path should come from somewhere!
                         Printer struct? */
  int sockfd;
  int filefd;

  ssize_t rc = 0;

  size_t tmp_buf_len = 0;
  size_t file_len = 0;
  size_t http_req_len = 0;
  size_t read_len = 0;
  size_t read_size = 0;

  struct ipp_wire_header *ipp_header = ipp_mk_wire_header(IPP_OP_GET_PRINTER_ATTR, (int32_t) rand());

  if (!ipp_header)
    return ;

  sockfd = get_connection(host, port);

  if (sockfd == -1)
    return;

  ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_OPERATION_ATTR, NULL, NULL);

  ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_CHARSET, "attributes-charset", "utf-8");
  ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_NATURAL_LANGUAGE, "attributes-natural-language", "en-us");

  /* TODO: Path shouldn't be hardcoded */
  tmp_buf_len = strlen(host) + strlen(port) + strlen(path) + 10;
  tmp_buf = (char *)malloc(tmp_buf_len);
  rc = snprintf(tmp_buf, tmp_buf_len, "http://%s:%s%s", host, port, path);
  tmp_buf[rc] = 0;
  ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_URI, "printer-uri", tmp_buf);
  free(tmp_buf);

  /* TODO: Insert real username here */
  ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_NAME_WITHOUT_LANG, "requesting-user-name", "mcgrewz");

  //ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_KEYWORD, "requested-attributes", "all");

  /* ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_KEYWORD, "document-format-default", "text/plain"); */

  ipp_wire_header_add_tag(ipp_header, (char)IPP_TAG_END_ATTR, NULL, NULL);

  /* TODO: This path (/ipp) shouldn't be hard coded */
  http_request = ipp_mk_http_request(host, port, "/ipp", ipp_header, file_len, &http_req_len);

  printf("HTTP REQ LEN: %zu\n", http_req_len);

  if (http_request) {
    printf("Generated HTTP Request:\n--------------------------------------------------------------------------------\n%s\n", http_request);

    rc = write(sockfd, http_request, http_req_len);
    free(http_request);

    printf("Write (HTTP+IPP) RC: %zd\n", rc);

    /* Guess a big buffer size, it will increase if needed */
    /* Test printer sends almost 7K of data... */
    read_size = 4096;
    read_buf = (char *)malloc(read_size);

    while ((rc = read(sockfd, read_buf + read_len, read_size - read_len))) {
      read_len += (size_t)rc;

      if (read_size - read_len == 0) {
        read_size += 4096;
        tmp_buf = realloc(read_buf, read_size);
        if (tmp_buf != read_buf)
          read_buf = tmp_buf;
      }

      printf("Read (RC: %zd)\n", rc);
    }

    filefd = open("response.bin", O_WRONLY);
    write(filefd, read_buf, read_len);
    close(filefd);

    printf("Parse RC: %d\n", ipp_parse_headers(read_buf, read_len, 0, &file_len));

    free(read_buf);
    ipp_free_wire_header(ipp_header);

    close(sockfd);
  }

  return;
}
