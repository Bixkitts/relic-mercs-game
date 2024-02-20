#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "packet_handlers.h"
#include "host_custom_attributes.h"
#include "error_handling.h"
#include "helpers.h"
#include "websockets.h"
#include "html_server.h"


typedef void (*PacketHandler)(char *data, ssize_t packetSize, Host remotehost);

static void htmlHandler        (char *data, ssize_t packetSize, Host remotehost);
static void websockHandler     (char *data, ssize_t packetSize, Host remotehost);
static void establishedHandler (char *data, ssize_t packetSize, Host remotehost);


static PacketHandler handlers[HANDLER_COUNT] = {
    htmlHandler,
    websockHandler,
    establishedHandler
};

void masterHandler(char *data, ssize_t packetSize, Host remotehost)
{
    HostCustomAttributes *customAttr = NULL;
    if (getHostCustomAttr(remotehost) == NULL) {
        customAttr = (HostCustomAttributes*)calloc(1, sizeof(HostCustomAttributes));
        if (customAttr == NULL) {
            printError(BB_ERR_CALLOC);
            exit(1);
        }
        setHostCustomAttr(remotehost, (void*)customAttr);
    }
    else {
        customAttr = (HostCustomAttributes*)getHostCustomAttr(remotehost);
    }
#ifdef DEBUG
    printf("\nReceived data:");
    for (int i = 0; i < packetSize; i++) {
        printf("%c", data[i]);
    }
    printf("\n");
#endif

    handlers[customAttr->handler](data, packetSize, remotehost);

    return;
}
static void htmlHandler(char *data, ssize_t packetSize, Host remotehost)
{
    if (stringSearch(data, "/game") < 0) {
        sendForbiddenPacket(remotehost);
    }
    char header[2048]            = { 0 };
    const char status       [64] =    "HTTP/1.1 200 OK\n";
    const char contentType  [64] =    "Content-Type: text/html\n";
    const char contentLength[64] =    "Content-Length: ";

    char *content   = NULL;
    int  len        = getFileData("./index.html", &content);
    char lenStr[32] = {0};
    sprintf(lenStr, "%d\n\n", len);

    strncpy(header, status, 64);
    strncat(header, contentType, 64);
    strncat(header, contentLength, 64);
    strncat(header, lenStr, 32);
    strncat(header, content, len);

    sendDataTCP(header, strlen(header), remotehost);

    free (content);
}

static void websockHandler(char *data, ssize_t packetSize, Host remotehost)
{
    HostCustomAttributes *customAttr = (HostCustomAttributes*)getHostCustomAttr(remotehost);
#ifdef DEBUG
    printf("\nwebsockHandler() called...\n");
#endif

    getWebSocketResponse(data);
    // We've succesfully opened a websocket channel
    customAttr->handler = HANDLER_ESTABLISHED;
}

static void establishedHandler(char *data, ssize_t packetSize, Host remotehost)
{
#ifdef DEBUG
    printf("\nestablishedHandler() called...\n");
#endif

}
