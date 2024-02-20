#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "error_handling.h"
#include "html_server.h"

static const char contentTypeStrings[MAX_HEADERS][STATUS_LENGTH] = {
    "text/html\n",
    "image/jpg\n",
    "image/png\n"
};

void sendForbiddenPacket(Host remotehost)
{
    char *forbidden =
        "HTTP/1.1 301 Moved Permanently\n"
        "Location: https://http.cat/403\n"
        "Content-Type: text/html\n"
        "Content-Length: 0\n\n";
    sendDataTCP(forbidden, strlen(forbidden), remotehost);
    return;
}
static const char *getContentTypeString(HTTPContentType type)
{
    return contentTypeStrings[type];
}
void sendContent(char* dir, HTTPContentType type, Host remotehost)
{
    char       header[HEADER_PACKET_LENGTH] = { 0 };
    const char status       [HEADER_LENGTH] = "HTTP/1.1 200 OK\n";
    const char contentType  [HEADER_LENGTH] = "Content-Type: ";
    const char contentLength[HEADER_LENGTH] = "Content-Length: ";

    char *content               = NULL;
    int   contentLen            = getFileData(dir, &content);
    char  lenStr[STATUS_LENGTH] = {0};

    sprintf(lenStr, "%d\n\n", contentLen);

    // Status:
    strncpy(header, status, HEADER_LENGTH);
    // Content-Type:
    strncat(header, contentType, HEADER_LENGTH);
    strncat(header, getContentTypeString(type), STATUS_LENGTH);
    // Content-Length:
    strncat(header, contentLength, HEADER_LENGTH);
    strncat(header, lenStr, STATUS_LENGTH);

    // ------ Header Over -----------//
    //strncat(header, content, len);

    sendDataTCP(header, strlen(header), remotehost);
    sendDataTCP(content, contentLen, remotehost);
    
    free (content);
}
int getFileData(const char *dir, char **buffer)
{
    // Open the HTML file
    int fd = open(dir, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Get the size of the file
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the buffer to hold the file content
    *buffer = (char *)calloc(1, file_stat.st_size + 1); // +1 for null terminator
    if (*buffer == NULL) {
        printError(BB_ERR_MALLOC);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Read the file content into the buffer
    ssize_t bytes_read = read(fd, *buffer, file_stat.st_size);
    if (bytes_read == -1) {
        perror("Error reading file");
        close(fd);
        free(*buffer);
        exit(EXIT_FAILURE);
    }

    // Close the file
    close(fd);
    return bytes_read;
}
