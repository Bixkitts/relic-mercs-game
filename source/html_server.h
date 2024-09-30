#ifndef BB_RELIC_HTML_SERVER
#define BB_RELIC_HTML_SERVER

#include "bbnetlib.h"

// Max length of individual headers
#define HEADER_LENGTH          64
#define MAX_HEADERS            32
// Max size of all the headers put together
#define HEADER_PACKET_LENGTH   HEADER_LENGTH *MAX_HEADERS
#define CUSTOM_HEADERS_MAX_LEN 256
// The string length for integers and names
// in string form to put into headers
#define STATUS_LENGTH          32

enum http_content_type {
    HTTP_FLAG_TEXT_HTML,
    HTTP_FLAG_IMAGE_JPG,
    HTTP_FLAG_IMAGE_PNG,
    HTTP_FLAG_IMAGE_BMP,
    HTTP_FLAG_TEXT_JAVASCRIPT,
    HTTP_FLAG_TEXT_CSS,
    HTTP_FLAG_COUNT
};

void send_content(char *dir,
                  enum http_content_type type,
                  struct host *remotehost,
                  const char *custom_headers);
void send_forbidden_packet(struct host *remotehost);
void send_bad_request_packet(struct host *remotehost);

// A list of files we're allowed to serve
// with "sendContent()"
void create_allowed_file_table(void);
bool is_file_allowed(const char *in_dir, char **out_dir);

enum http_content_type get_content_type_enum_from_filename(char *name);

#endif
