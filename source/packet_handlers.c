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

static void httpHandler        (char *data, ssize_t packetSize, Host remotehost);
static void websockHandler     (char *data, ssize_t packetSize, Host remotehost);
static void establishedHandler (char *data, ssize_t packetSize, Host remotehost);


static PacketHandler handlers[HANDLER_COUNT] = {
    httpHandler,
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

static void httpHandler(char *data, ssize_t packetSize, Host remotehost)
{
    if (stringSearch(data, "GET /game", packetSize) >= 0) {
        sendContent("./index.html", HTTP_FLAG_TEXT_HTML, remotehost);
    }
    else if (stringSearch(data, "GET /CADE.png", packetSize) >= 0) {
        sendContent("./CADE.png", HTTP_FLAG_IMAGE_PNG, remotehost);
    }
    else {
        sendForbiddenPacket(remotehost);
    }
    
}

static void websockHandler(char *data, ssize_t packetSize, Host remotehost)
{
    HostCustomAttributes *customAttr = (HostCustomAttributes*)getHostCustomAttr(remotehost);

    sendWebSocketResponse(data);
    // We've succesfully opened a websocket channel
    customAttr->handler = HANDLER_WEBSOCK;
}
