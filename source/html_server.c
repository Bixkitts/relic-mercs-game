#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "file_handling.h"
#include "html_server.h"
#include "error_handling.h"

static const char contentTypeStrings[MAX_HEADERS][STATUS_LENGTH] = {
    "text/html\n",
    "image/jpg\n",
    "image/png\n",
    "text/javascript\n"
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
    char          header   [HEADER_PACKET_LENGTH] = { 0 };
    unsigned long headerLen                       = 0;
    const char    status          [HEADER_LENGTH] = "HTTP/1.1 200 OK\n";
    const char    contentType     [HEADER_LENGTH] = "Content-Type: ";
    const char    contentLenString[HEADER_LENGTH] = "Content-Length: ";

    char *content               = NULL;
    int   contentLen            = 0;
    char  lenStr[STATUS_LENGTH] = {0};
    char *packet                = NULL;
    int   packetLen             = 0;

    contentLen = getFileData(dir, &content);
    if (contentLen < 0) {
        printError(BB_ERR_FILE_NOT_FOUND);
        if (content != NULL) {
            free(content);
        }
        return;
    }

    sprintf(lenStr, "%d\n\n", contentLen);
    // Status:
    strncpy(header, status, HEADER_LENGTH);
    // Content-Type:
    strncat(header, contentType, HEADER_LENGTH);
    strncat(header, getContentTypeString(type), STATUS_LENGTH);
    // Content-Length:
    strncat(header, contentLenString, HEADER_LENGTH);
    strncat(header, lenStr, STATUS_LENGTH);

    headerLen = strlen (header);

    // ------ Header Over -----------//

    packetLen = headerLen + contentLen;
    packet    = (char*)malloc( (headerLen * sizeof(char)) + (contentLen * sizeof(char)));
    if (packet == NULL) {
        printError(BB_ERR_MALLOC);
        exit(1);
    }

    memcpy(packet, header, headerLen);
    memcpy(&packet[headerLen], content, contentLen);

    sendDataTCP(packet, packetLen, remotehost);
    
    free (packet);
    free (content);
}
