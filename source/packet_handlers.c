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

enum CredentialFormFields {
    FORM_CREDENTIAL_PLAYERNAME,
    FORM_CREDENTIAL_PLAYERPASSWORD,
    FORM_CREDENTIAL_GAMEPASSWORD,
    FORM_CREDENTIAL_FIELD_COUNT
};
typedef void (*PacketHandler)(char *data, ssize_t packetSize, Host remotehost);

static void httpHandler      (char *data, ssize_t packetSize, Host remotehost);
static void websockHandler   (char *data, ssize_t packetSize, Host remotehost);

static void loginHandler     (char *restrict data, ssize_t packetSize, Host remotehost);
static void charsheetHandler (char *restrict data, ssize_t packetSize, Host remotehost);
static void POSTHandler      (char *restrict data, ssize_t packetSize, Host remotehost);
static void GETHandler       (char *restrict data, ssize_t packetSize, Host remotehost);


static PacketHandler handlers[HANDLER_COUNT] = {
    httpHandler,
    websockHandler
};

void masterHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    HostCustomAttributes *customAttr = NULL;
    if (getHostCustomAttr(remotehost) == NULL) {
        customAttr = calloc(1, sizeof(*customAttr));
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

    Game          *game      = getTestGame();
    long long int  token     = getTokenFromHTTP     (data, packetSize);
    const Player  *player    = tryGetPlayerFromToken(token, game);

#ifdef DEBUG_TEMP // TODO BEFORE PUSH: I just need to test rendering functions
    token = 696969;
#endif
    /*
     * Direct the remotehost to the login, character creation
     * or game depending on their session token.
     */
    if (stringSearch(data, "GET / ", 10) >= 0) {
        if (player == NULL) {
#ifndef DEBUG
            sendContent ("./login.html", 
                         HTTP_FLAG_TEXT_HTML, 
                         remotehost, 
                         NULL);
#endif
#ifdef DEBUG_TEMP // TODO BEFORE PUSH: I just need to test rendering functions
            sendContent ("./game.html", 
                         HTTP_FLAG_TEXT_HTML, 
                         remotehost, 
                         NULL);
#endif

        }
        else if (!isCharsheetValid(player)) {
            sendContent ("./charsheet.html", 
                         HTTP_FLAG_TEXT_HTML, 
                         remotehost, 
                         NULL);
        }
        else {
            sendContent ("./game.html", 
                         HTTP_FLAG_TEXT_HTML, 
                         remotehost, 
                         NULL);
        }
        return;
    }
    // Unauthenticated users are allowed the stylesheet, and login script
    else if (stringSearch(data, "GET /styles.css", 16) >= 0) {
        sendContent("./styles.css", HTTP_FLAG_TEXT_CSS, remotehost, NULL);
        return;
    }
    else if (stringSearch(data, "GET /login.js", 14) >= 0) {
        sendContent("./login.js", HTTP_FLAG_TEXT_JAVASCRIPT, remotehost, NULL);
        return;
    }
    /*
     * For any other url than a blank one,
     * the remotehost needs to be authenticated
     * otherwise we're not sending anything at all.
     */
    if (player == NULL
#ifdef DEBUG_TEMP
        && token != 696969
#endif
            ) {
        sendForbiddenPacket(remotehost);
        return;
    }
    else if (isFileAllowed(requestedResource, &fileTableEntry)) {
        sendContent(fileTableEntry, getContentTypeEnumFromFilename(fileTableEntry), remotehost, NULL);
        return;
    }
    else if (stringSearch(data, "GET /index.js", 12) >= 0) {
        sendContent("./index.js", HTTP_FLAG_TEXT_JAVASCRIPT, remotehost, NULL);
        return;
    }
    else if (stringSearch(data, "Sec-WebSocket-Key", packetSize) >= 0) {
        sendWebSocketResponse (data, packetSize, remotehost);
        HostCustomAttributes *hostAttr  = (HostCustomAttributes*)getHostCustomAttr(remotehost);
        hostAttr->player    = player;
        customAttr->handler = HANDLER_WEBSOCK;
        cacheHost             (remotehost, 0);
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
    int               credentialIndex                    = 0;
    const char        firstFormField[MAX_CREDENTIAL_LEN] = "playerName=";
    HTMLForm          form                               = { 0 };

    credentialIndex = 

    stringSearch  (data, 
                   firstFormField, 
                   packetSize);
    parseHTMLForm (&data[credentialIndex], 
                   &form, 
                   packetSize - credentialIndex);
    if (form.fieldCount < FORM_CREDENTIAL_FIELD_COUNT) {
        // TODO: something went wrong while parsing, abort and 
        // tell the user something about their malformed data
        sendForbiddenPacket(remotehost); //placeholder
        return;
    }
    if (tryGameLogin(getTestGame(), 
                     form.fields[FORM_CREDENTIAL_GAMEPASSWORD])
            != 0) {
        sendBadRequestPacket(remotehost);
        return;
    };
    strncpy        (credentials.name, 
                    form.fields[FORM_CREDENTIAL_PLAYERNAME], 
                    MAX_CREDENTIAL_LEN);
    strncpy        (credentials.password, 
                    form.fields[FORM_CREDENTIAL_PLAYERPASSWORD], 
                    MAX_CREDENTIAL_LEN);
    if (tryPlayerLogin (getTestGame(), 
                        &credentials, 
                        remotehost) < 0) {
        sendBadRequestPacket(remotehost);
    }
}

static void charsheetHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    long long int   token         = getTokenFromHTTP(data, packetSize); 
    Player         *player        = tryGetPlayerFromToken(token, getTestGame());
    HTMLForm        form          = {0};

    const char firstFormField[HTMLFORM_FIELD_MAX_LEN] = "playerBackground=";

    if (player == NULL) {
        // token was invalid, handle that
        sendForbiddenPacket(remotehost);
        return;
    }
    int htmlFormIndex =
    stringSearch                (data, 
                                 firstFormField, 
                                 packetSize);
    if (htmlFormIndex < 0) {
        // TODO: Malformed form data, let the client know
        sendForbiddenPacket(remotehost); //placeholder
        return;
    }
    parseHTMLForm               (&data[htmlFormIndex], 
                                 &form, 
                                 packetSize - htmlFormIndex);
    if (initCharsheetFromForm (player, &form) != 0) {
        // The client needs to know about malformed data
        sendForbiddenPacket(remotehost); //placeholder
        return;
    }
    sendContent                 ("./game.html", 
                                 HTTP_FLAG_TEXT_HTML, 
                                 remotehost,
                                 NULL);
}

static void POSTHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    if (stringSearch(data, "login", 12) >= 0) {
        loginHandler(data, packetSize, remotehost);
    }
    else if (stringSearch(data, "charsheet", 16) >= 0) {
        charsheetHandler(data, packetSize, remotehost);
    }
}

static void httpHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    // TODO: I _think_ packetSize is capped at 1024....
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
    else if (packetSize < 8) {
        fprintf(stderr, "\nToo short websocket packet received.\n");
        return;
    }
    // After this part we can freely dereference the 
    // first 8 bytes for opcodes and such.
    char  decodedData[MAX_PACKET_SIZE] = { 0 };
    int   decodedDataLength            = 0;

    decodedDataLength =
    decodeWebsocketMessage (decodedData, data, packetSize);
    handleGameMessage      (decodedData, decodedDataLength, remotehost);
}
