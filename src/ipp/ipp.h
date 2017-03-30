#ifndef IPP_H_
#define IPP_H_

#include <stdbool.h>
#include <stdint.h>

/* IPP Operation types */
enum ipp_op {
  IPP_OP_PRINT_JOB        = 0x02,
  IPP_OP_PRINT_URI        = 0x03,
  IPP_OP_VALIDATE_JOB     = 0x04,
  IPP_OP_CREATE_JOB       = 0x05,
  IPP_OP_SEND_DOCUMENT    = 0x06,
  IPP_OP_SEND_URI         = 0x07,
  IPP_OP_CANCEL_JOB       = 0x08,
  IPP_OP_GET_JOB_ATTR     = 0x09,
  IPP_OP_GET_JOBS         = 0x0a,
  IPP_OP_GET_PRINTER_ATTR = 0x0b,
  IPP_OP_HOLD_JOB         = 0x0c,
  IPP_OP_RELEASE_JOB      = 0x0d,
  IPP_OP_RESTART_JOB      = 0x0e,
  IPP_OP_PAUSE_PRINTER    = 0x10,
  IPP_OP_RESUME_PRINTER   = 0x11,
  IPP_OP_PURGE_JOBS       = 0x12
};

/* IPP Tags from RFC 2910 */
enum ipp_tag {
  /* Attribute Tags */
  IPP_TAG_OPERATION_ATTR    = 0x01,
  IPP_TAG_JOB_ATTR          = 0x02,
  IPP_TAG_END_ATTR          = 0x03,
  IPP_TAG_PRINT_ATTR        = 0x04,
  IPP_TAG_USUPPORTED_ATTR   = 0x05,

  /* Out-out-band Value Tags */
  IPP_TAG_UNSUPPORTED       = 0x10,
  IPP_TAG_UNKNOWN           = 0x12,
  IPP_TAG_NO_VALUE          = 0x13,

  /* Value Tags */
  IPP_TAG_INTEGER           = 0x21,
  IPP_TAG_BOOLEAN           = 0x22,
  IPP_TAG_ENUM              = 0x23,

  /* Octet String Value Tags */
  IPP_TAG_OCTET_STRING      = 0x30,
  IPP_TAG_DATETIME          = 0x31,
  IPP_TAG_RESOLUTION        = 0x32,
  IPP_TAG_INT_RANGE         = 0x33,
  IPP_TAG_TEXT_WITH_LANG    = 0x35,
  IPP_TAG_NAME_WITH_LANG    = 0x36,

  /* Character String Value Tags */
  IPP_TAG_TEXT_WITHOUT_LANG = 0x41,
  IPP_TAG_NAME_WITHOUT_LANG = 0x42,
  IPP_TAG_KEYWORD           = 0x44,
  IPP_TAG_URI               = 0x45,
  IPP_TAG_URI_SCHEME        = 0x46,
  IPP_TAG_CHARSET           = 0x47,
  IPP_TAG_NATURAL_LANGUAGE  = 0x48,
  IPP_TAG_MIME_TYPE         = 0x49
};

/* IPP Status Codes */
enum ipp_stat {
  /* Status OK(-ish) */
  IPP_STAT_OK                          = 0x0000,
  IPP_STAT_OK_ATTR_IGNORED             = 0x0001,
  IPP_STAT_OK_ATTR_CONFLICT            = 0x0002,

  /* Client Status */
  IPP_STAT_CLI_BAD_REQUEST             = 0x0400,
  IPP_STAT_CLI_ERR_FORBIDDEN           = 0x0401,
  IPP_STAT_CLI_ERR_NOT_AUTHENTICATED   = 0x0402,
  IPP_STAT_CLI_ERR_NOT_AUTHERIZED      = 0x0403,
  IPP_STAT_CLI_ERR_NOT_POSSIBLE        = 0x0404,
  IPP_STAT_CLI_ERR_TIMEOUT             = 0x0405,
  IPP_STAT_CLI_ERR_NOT_FOUND           = 0x0406,
  IPP_STAT_CLI_ERR_GONE                = 0x0407,
  IPP_STAT_CLI_ERR_REQ_ENTITY_TOO_BIG  = 0x0408,
  IPP_STAT_CLI_ERR_REQ_VALUE_TOO_LONG  = 0x0409,
  IPP_STAT_CLI_ERR_FORMAT_NO_SUPPORT   = 0x040A,
  IPP_STAT_CLI_ERR_ATTR_NO_SUPPORT     = 0x040B,
  IPP_STAT_CLI_ERR_URI_NO_SUPPORT      = 0x040C,
  IPP_STAT_CLI_ERR_CHAR_NO_SUPPORT     = 0x040D,
  IPP_STAT_CLI_ERR_ATTR_CONFLICT       = 0x040E,
  IPP_STAT_CLI_ERR_COMPRESS_NO_SUPPORT = 0x040F,
  IPP_STAT_CLI_ERR_COMPRESS_ERR        = 0x0410,
  IPP_STAT_CLI_ERR_FORMAT_ERR          = 0x0411,
  IPP_STAT_CLI_ERR_ACCESS_ERR          = 0x0412,

  /* Server Status */
  IPP_STAT_SRV_ERR_INTERNAL_ERR        = 0x0500,
  IPP_STAT_SRV_ERR_OP_NO_SUPPORT       = 0x0501,
  IPP_STAT_SRV_ERR_SRV_UNAVAILABLE     = 0x0502,
  IPP_STAT_SRV_ERR_VERSION_NO_SUPPORT  = 0x0503,
  IPP_STAT_SRV_ERR_DEVICE_ERR          = 0x0504,
  IPP_STAT_SRV_ERR_TEMPORARY_ERR       = 0x0505,
  IPP_STAT_SRV_ERR_NOT_ACCEPTING_JOBS  = 0x0506,
  IPP_STAT_SRV_ERR_BUSY                = 0x0507,
  IPP_STAT_SRV_ERR_JOB_CANCELED        = 0x0508,
  IPP_STAT_SRV_ERR_NO_MULTI_DOC        = 0x0509,
};

/* Status Code Checks */
#define IPP_STATUS_SUCCESS(x)  ((x) >= 0x0000 && (x) <= 0x00ff)
#define IPP_STATUS_INFO(x)     ((x) >= 0x0100 && (x) <= 0x01ff)
#define IPP_STATUS_REDIRECT(x) ((x) >= 0x0300 && (x) <= 0x03ff)
#define IPP_STATUS_CLI_ERR(x)  ((x) >= 0x0400 && (x) <= 0x04ff)
#define IPP_STATUS_SRV_ERR(x)  ((x) >= 0x0500 && (x) <= 0x05ff)

struct ipp_header {
  /* 2 Bytes: Version 1.1 */
  int8_t major; 
  int8_t minor;

  /* 2 Bytes: Operation or Status */
  int16_t op_stat;

  /* 4 Bytes: Request ID */
  int32_t request_id;


  /* This field is not transmitted across the wire */
  size_t tags_used;
  size_t tags_size;

  char *tags;
};

int ipp_connect(const char *address, const char *port);

struct ipp_header *ipp_mk_header(int16_t op_stat, int32_t request_id);
void ipp_free_header(struct ipp_header *header);
bool ipp_header_add_tag(struct ipp_header *header, const enum ipp_tag tag,
                        const char *tag_name, const char *tag_val);

void ipp_getPrinterInfo(int fd);

#endif /* !IPP_H_ */
