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
#include "auth.h"

enum CredentialFormFields {
    FORM_CREDENTIAL_PLAYERNAME,
    FORM_CREDENTIAL_PLAYERPASSWORD,
    FORM_CREDENTIAL_GAMEPASSWORD,
    FORM_CREDENTIAL_FIELD_COUNT
};
typedef void (*PacketHandler)(char *data, ssize_t packetSize, Host remotehost);

static enum Handler
initialHandlerCheck(Host remotehost);

static void disconnectHandler (char *data, ssize_t packetSize, Host remotehost);
static void httpHandler       (char *data, ssize_t packetSize, Host remotehost);
static void websockHandler    (char *data, ssize_t packetSize, Host remotehost);

static void loginHandler     (char *restrict data, ssize_t packetSize, Host remotehost);
static void charsheetHandler (char *restrict data, ssize_t packetSize, Host remotehost);
static void POSTHandler      (char *restrict data, ssize_t packetSize, Host remotehost);
static void GETHandler       (char *restrict data, ssize_t packetSize, Host remotehost);

/* disconnectHandler needs to be at index 0
 * because we use pointer math to handle
 * empty packets (TCP disconnections)
 */
static PacketHandler handlers[HANDLER_COUNT] = {
    disconnectHandler,
    httpHandler,
    websockHandler
};

/*
 * Called from masterHandler,
 * checks all incoming connections
 * and figures out which handler to use
 */
static enum Handler
initialHandlerCheck(Host remotehost)
{
    struct HostCustomAttributes *customAttr = NULL;
    if (getHostCustomAttr(remotehost) == NULL) {
        customAttr = calloc(1, sizeof(*customAttr));
        if (customAttr == NULL) {
            printError(BB_ERR_CALLOC);
            exit(1);
        }
        customAttr->handler = HANDLER_DEFAULT;
        setHostCustomAttr(remotehost, (void*)customAttr);
    }
    else {
        customAttr = (struct HostCustomAttributes*)getHostCustomAttr(remotehost);
    }
    return customAttr->handler;
}

void masterHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    const enum Handler
    handler = initialHandlerCheck(remotehost);
#ifdef DEBUG
    if (packetSize > 0) {
        printf("\nReceived data:");
        for (int i = 0; i < packetSize; i++) {
            printf("%c", data[i]);
        }
        printf("\n");
    }
    else if (packetSize == 0) {
        printf("\nClient disconnected\n");
    }
#endif

    // Pointer math handles client disconnects
    // and calls disconnectHandler()
    handlers[handler * (packetSize > 0)](data, packetSize, remotehost);

    return;
}

static void GETHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    char  requestedResource[MAX_FILENAME_LEN] = {0};

    char *restrict startingPoint  = &data[5];
    int   stringLen               = charSearch(startingPoint, ' ', packetSize - 5);
    char *fileTableEntry          = NULL;

    struct HostCustomAttributes *customAttr = (struct HostCustomAttributes*)getHostCustomAttr(remotehost);

    if (stringLen < 0 || stringLen > MAX_FILENAME_LEN){
        return;
    }

    memcpy(requestedResource, startingPoint, stringLen);

    struct Game   *game   = getGameFromName  (testGameName);
    enqueue(game->queue, 0);
    SessionToken   token  = getTokenFromHTTP (data, packetSize);
    struct Player *player = tryGetPlayerFromToken(token, game);
    dequeue(game->queue);

    /* Direct the remotehost to the login, character creation
     * or game depending on their session token.
     */
    if (stringSearch(data, "GET / ", 10) >= 0) {
        if (player == NULL) {
            sendContent ("./login.html", 
                         HTTP_FLAG_TEXT_HTML, 
                         remotehost, 
                         NULL);

        }
        else if (!isCharsheetValid(player)) {
            sendContent ("./charsheet.html", 
                         HTTP_FLAG_TEXT_HTML, 
                         remotehost, 
                         NULL);
        }
        else if (stringSearch(data, "Sec-WebSocket-Key", packetSize) >= 0) {
            sendWebSocketResponse (data, packetSize, remotehost);
            struct HostCustomAttributes *hostAttr  = (struct HostCustomAttributes*)getHostCustomAttr(remotehost);
            hostAttr->player    = player;
            customAttr->handler = HANDLER_WEBSOCK;
            cacheHost             (remotehost, getCurrentHostCache());
            return;
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
     * For any other url than a blank one
     * (or the styles.css and login.js),
     * the remotehost needs to be authenticated
     * otherwise we're not sending anything at all.
     */
    if (player == NULL) {
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
    sendForbiddenPacket(remotehost);
}

static void loginHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    // Read the Submitted Player Name, Player Password and Game Password
    // and link the remotehost to a specific player object based on that.
    struct PlayerCredentials credentials                        = { 0 };
    int                      credentialIndex                    = 0;
    const char               firstFormField[MAX_CREDENTIAL_LEN] = "playerName=";
    struct HTMLForm          form                               = { 0 };

    struct Game *game = getGameFromName(testGameName);
    enqueue(game->queue, 0);

    credentialIndex = 

    stringSearch  (data, 
                   firstFormField, 
                   packetSize);
    parseHTMLForm (&data[credentialIndex], 
                   &form, 
                   packetSize - credentialIndex);
    if (form.fieldCount < FORM_CREDENTIAL_FIELD_COUNT) {
        sendForbiddenPacket(remotehost); //placeholder
        dequeue(game->queue);
        return;
    }
    if (tryGameLogin(game, 
                     form.fields[FORM_CREDENTIAL_GAMEPASSWORD])
            != 0) {
        sendBadRequestPacket(remotehost);
        dequeue(game->queue);
        return;
    };
    strncpy        (credentials.name, 
                    form.fields[FORM_CREDENTIAL_PLAYERNAME], 
                    MAX_CREDENTIAL_LEN);
    strncpy        (credentials.password, 
                    form.fields[FORM_CREDENTIAL_PLAYERPASSWORD], 
                    MAX_CREDENTIAL_LEN);
    if (tryPlayerLogin (game, 
                        &credentials, 
                        remotehost) < 0) {
        sendBadRequestPacket(remotehost);
        dequeue(game->queue);
        return;
    }
    dequeue(game->queue);
    return;
}

static void charsheetHandler(char *restrict data,
                             ssize_t packetSize,
                             Host remotehost)
{
    SessionToken     token  = getTokenFromHTTP(data, packetSize); 
    struct Game     *game   = getGameFromName(testGameName);
    enqueue(game->queue, 0);
    struct Player   *player = tryGetPlayerFromToken(token, game);
    struct HTMLForm  form   = {0};

    const char firstFormField[HTMLFORM_FIELD_MAX_LEN] = "playerBackground=";

    if (player == NULL) {
        // token was invalid, handle that
        sendForbiddenPacket(remotehost);
        goto cleanup;
    }
    int htmlFormIndex =
    stringSearch                (data, 
                                 firstFormField, 
                                 packetSize);
    if (htmlFormIndex < 0) {
        // TODO: Malformed form data, let the client know
        sendForbiddenPacket(remotehost); //placeholder
        goto cleanup;
    }
    parseHTMLForm               (&data[htmlFormIndex], 
                                 &form, 
                                 packetSize - htmlFormIndex);
    if (initCharsheetFromForm (player, &form) != 0) {
        // The client needs to know about malformed data
        sendForbiddenPacket(remotehost); //placeholder
        goto cleanup;
    }
    sendContent                 ("./game.html", 
                                 HTTP_FLAG_TEXT_HTML, 
                                 remotehost,
                                 NULL);
cleanup:
    dequeue(game->queue);
    return;
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

/*
 * Global caching state.
 * Affected by disconnections.
 */
pthread_mutex_t cachingMutex     = PTHREAD_MUTEX_INITIALIZER;
// Netlib gives us numbered caches for
// storing hosts that connect
int8_t          currentHostCache = 0;
int8_t          lastHostCache    = 0;

/* 
 * The locking branching and looping
 * here probably makes this sloooow,
 * but it only runs when somebody disconnects
 * and a cache is invalidated.
 * I tried to optimise the loop
 * out of the cache lock but couldn't.
 */
static void writePlayersToNewCache(Host exclude)
{
    pthread_mutex_lock(&cachingMutex);    
    lastHostCache       = currentHostCache;
    currentHostCache    = !currentHostCache;
    struct HostCustomAttributes *attr = getHostCustomAttr(exclude);
    struct Player               *plyr = attr->player;
    struct Game                 *game = attr->player->game;
    pthread_mutex_lock(game->threadlock);
    for (int i = 0; i < MAX_PLAYERS_IN_GAME; i++) {
        struct Player *p = &game->players[i];
        if (p->netID != NULL_NET_ID
            && p->netID != plyr->netID) {
            cacheHost(p->associatedHost, currentHostCache);
        }
    }
    pthread_mutex_unlock(game->threadlock);
    clearHostCache(lastHostCache);
    pthread_mutex_unlock(&cachingMutex);    
}
static void disconnectHandler (char *data, ssize_t packetSize, Host remotehost)
{
    struct HostCustomAttributes *attr = getHostCustomAttr(remotehost);
    if (attr->handler == HANDLER_WEBSOCK) {
        writePlayersToNewCache(remotehost);
        // TODO: When someone disconnects,
        // the game will need to pause and alert everyone
        // of the disconnect and ask whether to
        // continue or wait.
    }
}
/*
 * When a user disconnects,
 * all OTHER users who are still
 * connected get written to another
 * multicast cache and the former
 * one is cleared.
 * This function returns the up-to-date
 * multicast cache.
 */
int  getCurrentHostCache (void)
{
    pthread_mutex_lock(&cachingMutex);    
    int ret = (int)currentHostCache;
    pthread_mutex_unlock(&cachingMutex);    
    return ret;
}

static void websockHandler(char *restrict data, ssize_t packetSize, Host remotehost)
{
    if (packetSize < 8) {
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

