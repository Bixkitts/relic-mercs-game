#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "packet_handlers.h"
#include "bbnetlib.h"
#include "host_custom_attributes.h"
#include "error_handling.h"
#include "helpers.h"
#include "websockets.h"
#include "file_handling.h"
#include "html_server.h"
#include "game_logic.h"

typedef void (*PacketHandler)(char *data, ssize_t packetSize, Host remotehost);

static void httpHandler        (char *data, ssize_t packetSize, Host remotehost);
static void websockHandler     (char *data, ssize_t packetSize, Host remotehost);

static void loginHandler (char *restrict data, ssize_t packetSize, Host remotehost);
static void POSTHandler  (char *restrict data, ssize_t packetSize, Host remotehost);
static void GETHandler   (char *restrict data, ssize_t packetSize, Host remotehost);


static PacketHandler handlers[HANDLER_COUNT] = {
    httpHandler,
    websockHandler
};

void masterHandler(char *restrict data, ssize_t packetSize, Host remotehost)
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

static void GETHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    char  requestedResource[MAX_FILENAME_LEN] = {0};

    char *restrict startingPoint  = &data[5];
    int   stringLen               = charSearch(startingPoint, ' ', packetSize - 5);
    char *fileTableEntry          = NULL;

    HostCustomAttributes *customAttr = (HostCustomAttributes*)getHostCustomAttr(remotehost);

    if (stringLen < 0 || stringLen > MAX_FILENAME_LEN){
        return;
    }

    memcpy(requestedResource, startingPoint, stringLen);

    if (stringSearch(data, "GET /login", 10) >= 0) {
        sendContent("./login.html", HTTP_FLAG_TEXT_HTML, remotehost, NULL);
        return;
    }
    else if (stringSearch(data, "GET /index.js", 12) >= 0) {
        sendContent("./index.js", HTTP_FLAG_TEXT_JAVASCRIPT, remotehost, NULL);
        return;
    }
    else if (stringSearch(data, "Sec-WebSocket-Key", packetSize) >= 0) {
        sendWebSocketResponse (data, packetSize, remotehost);
        cacheHost             (remotehost, 0);
        customAttr->handler = HANDLER_WEBSOCK;
        return;
    }
    else if (isFileAllowed(requestedResource, &fileTableEntry)) {
        sendContent(fileTableEntry, getContentTypeEnumFromFilename(fileTableEntry), remotehost, NULL);
        return;
    }
    // TODO: Have an ignore list of files the client should not be able
    // to download.
    // Or just put the whole site in a subdirectory and count on file extensions.
    sendForbiddenPacket(remotehost);

}

static void loginHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    // Read the Submitted Player Name, Player Password and Game Password
    // and link the remotehost to a specific player object based on that.
    PlayerCredentials credentials                        = { 0 };
    char              gamePassword  [MAX_CREDENTIAL_LEN] = { 0 };
    int               credentialIndex                    = 0;
    int               stopIndex                          = 0;
    const char        searchKey     [MAX_CREDENTIAL_LEN] = "playerName=";

    credentialIndex = stringSearch(data, searchKey, packetSize);
    // the player name is here:
    credentialIndex += strlen(searchKey);
    stopIndex       = charSearch(&data[credentialIndex], '&', packetSize - credentialIndex);
    capInt(&stopIndex, MAX_CREDENTIAL_LEN);
    memcpy(credentials.name, &data[credentialIndex], stopIndex);
    credentialIndex += charSearch(&data[credentialIndex], '=', packetSize - credentialIndex);
    credentialIndex += 1;
    stopIndex       = charSearch(&data[credentialIndex], '&', packetSize - credentialIndex);
    capInt(&stopIndex, MAX_CREDENTIAL_LEN);
    memcpy(credentials.password, &data[credentialIndex], stopIndex);
    credentialIndex += charSearch(&data[credentialIndex], '=', packetSize - credentialIndex);
    credentialIndex += 1;
    stopIndex       = strnlen(&data[credentialIndex], packetSize - credentialIndex);
    capInt(&stopIndex, MAX_CREDENTIAL_LEN);
    memcpy(gamePassword, &data[credentialIndex], stopIndex);

    if (!tryGameLogin(getTestGame(), gamePassword)) {
        return;
    };
    tryPlayerLogin(getTestGame(), &credentials, remotehost);
}

static void POSTHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    if (stringSearch(data, "login", 12) >= 0) {
        loginHandler(data, packetSize, remotehost);
    }
}

static void httpHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    if (packetSize < 10) {
        return;
    }
    if (stringSearch(data, "GET /", 8) >= 0) {
        GETHandler(data, packetSize, remotehost);
    }
    else if (stringSearch(data, "POST /", 8) >= 0) {
        POSTHandler(data, packetSize, remotehost);
    }
    else {
        sendForbiddenPacket(remotehost);
    }
    
}

static void websockHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    if (packetSize > 120) {
        fprintf(stderr, "\nToo long websocket packet received.\n");
        return;
    }
    if (packetSize < 8) {
        fprintf(stderr, "\nToo short websocket packet received.\n");
        return;
    }
    // After this part we can freely dereference the 
    // first 8 bytes for opcodes and such.
    char *decodedData       = (char*)calloc(packetSize, sizeof(char));
    int   decodedDataLength = 0;

    if (decodedData == NULL) {
        printError(BB_ERR_CALLOC);
        exit(1);
    }
    decodedDataLength =
    decodeWebsocketMessage(decodedData, data, packetSize);
    handleGameMessage (decodedData, decodedDataLength, remotehost);
    free              (decodedData);
}
