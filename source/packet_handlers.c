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
#include "html_server.h"
#include "game_logic.h"

static char fileTable[MAX_FILENAME_LEN * MAX_FILE_COUNT] = { 0 };
static int  fileTableLen = 0;

typedef void (*PacketHandler)(char *data, ssize_t packetSize, Host remotehost);

static void httpHandler        (char *data, ssize_t packetSize, Host remotehost);
static void websockHandler     (char *data, ssize_t packetSize, Host remotehost);


static PacketHandler handlers[HANDLER_COUNT] = {
    httpHandler,
    websockHandler
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
void buildFileTable(void)
{
    fileTableLen = listFiles(fileTable);
    for (int i = 0; i < fileTableLen; i++) {
        printf("%s\n", &fileTable[i * MAX_FILENAME_LEN]);
    }
}

static void GETHandler(char *data, ssize_t packetSize, Host remotehost)
{
    char  requestedResource[MAX_FILENAME_LEN] = {0};

    char *startingPoint  = &data[5];
    int   stringLen      = charSearch(startingPoint, ' ', packetSize - 5);

    if (stringLen < 0 || stringLen > MAX_FILENAME_LEN){
        return;
    }

    memcpy(requestedResource, startingPoint, stringLen);

    // TODO: Find a better way to send files with different names
    if (stringSearch(data, "GET /game", MAX_FILENAME_LEN * MAX_FILE_COUNT) >= 0) {
        sendContent("./index.html", HTTP_FLAG_TEXT_HTML, remotehost);
        return;
    }
    // TODO: Have an ignore list of files the client should not be able
    // to download.
    // Or just put the whole site in a subdirectory and count on file extensions.
    for (int i = 0; i < fileTableLen; i++) {
        char *fileTableEntry = &fileTable[i * MAX_FILENAME_LEN];
        if(stringSearch(fileTableEntry, requestedResource, fileTableLen * MAX_FILENAME_LEN) >= 0) {
            sendContent(fileTableEntry, getContentTypeEnumFromFilename(fileTableEntry), remotehost);
            return;
        }
    }
    HostCustomAttributes *customAttr = (HostCustomAttributes*)getHostCustomAttr(remotehost);
    if (stringSearch(data, "Sec-WebSocket-Key", packetSize) >= 0) {
        sendWebSocketResponse(data, packetSize, remotehost);
        cacheHost(remotehost, 0);
        customAttr->handler = HANDLER_WEBSOCK;
        return;
    }
    sendForbiddenPacket(remotehost);

}

static void httpHandler(char *data, ssize_t packetSize, Host remotehost)
{
    if (packetSize < 10) {
        return;
    }
    if (stringSearch(data, "GET /", 8) >= 0) {
        GETHandler(data, packetSize, remotehost);
    }
    else {
        sendForbiddenPacket(remotehost);
    }
    
}

static void websockHandler(char *data, ssize_t packetSize, Host remotehost)
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
    if ((uint16_t)decodedData[0] >= GAME_MSG_COUNT) {
    #ifdef DEBUG
        fprintf(stderr, "\nBad websocket opcode received\n");
    #endif
        free(decodedData);
        return;
    }
    handleGameMessage (decodedData, decodedDataLength, remotehost);
    free              (decodedData);
}
