#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error_handling.h"
#include "file_handling.h"
#include "helpers.h"
#include "html_server.h"

#define FILE_EXTENSION_LEN 16

static char files_to_serve[MAX_FILENAME_LEN * MAX_FILE_COUNT] = {0};
static int files_to_serve_len                                 = 0;

static const char content_type_strings[HTTP_FLAG_COUNT][STATUS_LENGTH] =
    {"text/html\r\n",
     "image/jpg\r\n",
     "image/png\r\n",
     "image/bmp\r\n",
     "text/javascript\r\n",
     "text/css\r\n"};
static const char content_type_mapping[HTTP_FLAG_COUNT][FILE_EXTENSION_LEN] =
    {"html", "jpg", "png", "bmp", "js", "css"};

void send_forbidden_packet(struct host *remotehost)
{
    const char *data = "HTTP/1.1 403 Forbidden\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: 0\r\n\r\n";
    send_data_tcp(data, strlen(data), remotehost);
    return;
}
void send_bad_request_packet(struct host *remotehost)
{
    const char *data = "HTTP/1.1 400 Bad Request\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: 0\r\n\r\n";
    send_data_tcp(data, strlen(data), remotehost);
    return;
}
static const char *get_content_type_string(enum http_content_type type)
{
    return content_type_strings[type];
}

/*
 * Returns if the file is allowed to be served
 * by looking it up in the allowed list.
 * Also returns the entire directory name
 * in an out parameter.
 */
bool is_file_allowed(const char *in_name, char **out_dir)
{
    for (int i = 0; i < files_to_serve_len; i++) {
        char *file_table_entry = &files_to_serve[i * MAX_FILENAME_LEN];
        if (string_search(file_table_entry,
                          in_name,
                          files_to_serve_len * MAX_FILENAME_LEN) >= 0) {
            *out_dir = file_table_entry;
            return 1;
        }
    }
    return 0;
}

/*
 * Assumes NUL terminated string,
 * parses file extension
 */
enum http_content_type get_content_type_enum_from_filename(char *name)
{
    int extension_index = char_search(&name[1], '.', MAX_FILENAME_LEN) + 2;
    // When a malformed filename comes, MIME type doesn't matter
    // just return default
    if (extension_index < 1) {
        return 0;
    }
    char *extension = &name[extension_index];
    int i           = 0;
    while (i < HTTP_FLAG_COUNT) {
        if (string_search(content_type_mapping[i],
                          extension,
                          FILE_EXTENSION_LEN) >= 0) {
            return i;
        }
        i++;
    }
    // default to HTML
    // TODO: This might be an issue? idek?
    // Is there a better MIME type to fall back to?
    return 0;
}

/*
 * This function is supposed to
 * create a list of files that
 * are whitelisted to be served
 */
void create_allowed_file_table(void)
{
    char *allowed_directories[] = {"./src/rendering/",
                                   "./src/ui/",
                                   "./src/",
                                   "./images/" };
    // Currently just whitelists everything
    files_to_serve_len = list_files(allowed_directories,
                                    sizeof(allowed_directories) / sizeof(*allowed_directories),
                                    files_to_serve);
#ifdef DEBUG
    for (int i = 0; i < files_to_serve_len; i++) {
        printf("%s\n", &files_to_serve[i * MAX_FILENAME_LEN]);
    }
#endif
}

void send_content(char *dir,
                  enum http_content_type type,
                  struct host *remotehost,
                  const char *custom_headers)
{
    char header[HEADER_PACKET_LENGTH]            = {0};
    unsigned long header_len                     = 0;
    const char status[HEADER_LENGTH]             = "HTTP/1.1 200 OK\r\n";
    const char content_type[HEADER_LENGTH]       = "Content-Type: ";
    const char content_len_string[HEADER_LENGTH] = "Content-Length: ";
    const char cors_header[HEADER_LENGTH] =
        "Access-Control-Allow-Origin: *\r\n";

    char *content               = NULL;
    int content_len             = 0;
    char len_str[STATUS_LENGTH] = {0};
    char *packet                = NULL;
    int packet_len              = 0;

    content_len = get_file_data(dir, &content);
    if (content_len < 0) {
        print_error(BB_ERR_FILE_NOT_FOUND);
        if (!content) {
            free(content);
        }
        return;
    }
    sprintf(len_str, "%d\r\n", content_len);

    // Status:
    strncpy(header, status, HEADER_LENGTH);
    // Content-Type:
    strncat(header, content_type, HEADER_LENGTH);
    strncat(header, get_content_type_string(type), STATUS_LENGTH);
    // Content-Length:
    strncat(header, content_len_string, HEADER_LENGTH);
    strncat(header, len_str, STATUS_LENGTH);
    // Access-Control:
    strncat(header, cors_header, HEADER_LENGTH);

    header_len = strnlen(header, HEADER_PACKET_LENGTH);
    // Custom Header:
    if (custom_headers) {
        strncat(header, custom_headers, HEADER_PACKET_LENGTH - header_len);
    }
    strncat(header, "\r\n", 3);

    header_len = strnlen(header, HEADER_PACKET_LENGTH);

    // ------ Header Over -----------//

    packet_len = header_len + content_len;
    packet = malloc((header_len * sizeof(char)) + (content_len * sizeof(char)));
    if (!packet) {
        print_error(BB_ERR_MALLOC);
        exit(1);
    }

    memcpy(packet, header, header_len);
    memcpy(&packet[header_len], content, content_len);

    send_data_tcp(packet, packet_len, remotehost);

    free(packet);
}
