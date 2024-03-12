#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#include "file_handling.h"
#include "html_server.h"
#include "error_handling.h"
#include "helpers.h"

#define FILE_EXTENSION_LEN 16


static char allowedFileTable[MAX_FILENAME_LEN * MAX_FILE_COUNT] = { 0 };
static int  allowedFileTableLen = 0;

static const char contentTypeStrings[HTTP_FLAG_COUNT][STATUS_LENGTH] = {
    "text/html\n",
    "image/jpg\n",
    "image/png\n",
    "text/javascript\n"
};
static const char contentTypeMapping[HTTP_FLAG_COUNT][FILE_EXTENSION_LEN] = {
    "html",
    "jpg",
    "png",
    "js"
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

/*
 * Returns if the file is allowed to be served
 * by looking it up in the allowed list.
 * Also returns the entire directory name
 * in an out parameter.
 */
bool isFileAllowed(const char* inName, char** outDir)
{
    for (int i = 0; i < allowedFileTableLen; i++) {
        char *fileTableEntry = &allowedFileTable[i * MAX_FILENAME_LEN];
        if(stringSearch(fileTableEntry, inName, allowedFileTableLen * MAX_FILENAME_LEN) >= 0) {
            *outDir = fileTableEntry;
            return 1;
        }
    }
    return 0;
}

/*
 * Assumes NUL terminated string,
 * parses file extension
 */
HTTPContentType getContentTypeEnumFromFilename(char* name)
{
    int extensionIndex =
    charSearch(&name[1], '.', MAX_FILENAME_LEN) + 2;
    // When a malformed filename comes, MIME type doesn't matter
    // just return default
    if (extensionIndex < 1) {
        return 0;
    }
    char *extension = &name[extensionIndex];
    int i = 0;
    while( i < HTTP_FLAG_COUNT ) {
        if (stringSearch(contentTypeMapping[i], extension, FILE_EXTENSION_LEN) >= 0) {
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
void createAllowedFileTable(void)
{
    // Currently just whitelists everything
    allowedFileTableLen = listFiles(allowedFileTable);
#ifdef DEBUG
    for (int i = 0; i < allowedFileTableLen; i++) {
        printf("%s\n", &allowedFileTable[i * MAX_FILENAME_LEN]);
    }
#endif
}

void sendContent(char* dir, HTTPContentType type, Host remotehost, const char *customHeaders)
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

    sprintf(lenStr, "%d\n", contentLen);
    // Status:
    strncpy(header, status, HEADER_LENGTH);
    // Content-Type:
    strncat(header, contentType, HEADER_LENGTH);
    strncat(header, getContentTypeString(type), STATUS_LENGTH);
    // Content-Length:
    strncat(header, contentLenString, HEADER_LENGTH);
    strncat(header, lenStr, STATUS_LENGTH);

    headerLen = strnlen (header, HEADER_PACKET_LENGTH);
    // Custom Header:
    if (customHeaders != NULL) {
        strncat(header, customHeaders, HEADER_PACKET_LENGTH - headerLen);
    }
    strncat(header, "\n", 1);

    headerLen = strnlen (header, HEADER_PACKET_LENGTH);

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
}
