#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#include "bbnetlib.h"
#include "host_custom_attributes.h"
#include "error_handling.h"
#include "helpers.h"
#include "websockets.h"
#include "html_server.h"
#include "game_logic.h"

#define MESSAGE_HANDLER_COUNT 6


/*
 * Handler type definitions
 */
typedef void (*GameMessageHandler)  (char* data, ssize_t dataSize, Host remotehost);
typedef void (*UseResourceHandler)  (ResourceID resource, Player *user, Player *target);
typedef void (*GiveResourceHandler) (ResourceID resource, Player *target, int count);
typedef void (*TakeResourceHandler) (ResourceID resource, Player *target, int count);


/*
 * Mutexes for state management
 */
#define STATE_MUTEX_COUNT 16
static pthread_mutex_t gameStateLock  [STATE_MUTEX_COUNT] = { PTHREAD_MUTEX_INITIALIZER };
static pthread_mutex_t playerStateLock[STATE_MUTEX_COUNT] = { PTHREAD_MUTEX_INITIALIZER };

/*
 * Not thread safe, the test game should be
 * written to once on init
 */
static Game testGame = { 0 };
Game *getTestGame()
{
    return &testGame;
}

/* 
 * Helpers and Authentication
 * ----------------------------
 *  None of these have mutex locks in them,
 *  the functions that call them are expected
 *  to lock the game state they are modifying.
 */
static void          generateSessionToken          (Player *player,
                                                    Game *game);
static int           isGameMessageValidLength         (Opcode opcode, 
                                                    ssize_t messageSize);
static void          buildSessionTokenHeader       (char outHeader[static HEADER_LENGTH], 
                                                    SessionToken token);
static Player       *tryGetPlayerFromCredentials   (Game *game, 
                                                    const PlayerCredentials *credentials);
static Player       *createPlayer                  (Game *game,
                                                    PlayerCredentials *credentials);

/*
 * Handlers for incoming messages from the websocket connection
 */
static void pingHandler                 (char *data, ssize_t dataSize, Host remotehost);
struct MovePlayerData {
    double xCoord;
    double yCoord;
};
static void movePlayerHandler           (char *data, ssize_t dataSize, Host remotehost);
static void endTurnHandler              (char *data, ssize_t dataSize, Host remotehost);
// Player chose a response to an encounter
static void respondToEventHandler       (char *data, ssize_t dataSize, Host remotehost);
// Client calls this on connect,
// this should either:
// 1. Set an atomic lock on the entire game state while the client connects (easy)
// 2. Allow the game to continue during the connection and apply
//    any state changes that happened since (harder)
static void getGameStateHandler         (char *data, ssize_t dataSize, Host remotehost);


/*
 * Primary interpreter for incoming websocket messages
 * carrying valid gameplay opcodes.
 */
static GameMessageHandler gameMessageHandlers[MESSAGE_HANDLER_COUNT] = {
    pingHandler,
    movePlayerHandler, 
    endTurnHandler,           
    respondToEventHandler, 
    getGameStateHandler         
};
static int gameDataSizes[MESSAGE_HANDLER_COUNT] = {
    0,
    sizeof(struct MovePlayerData),
    0,
    0,
    0,
    0
};

/*
 * There's an opcode, and then
 * serialised state data.
 * This state data should match a
 * corresponding data structure perfectly,
 * or be rejected.
 */
static int isGameMessageValidLength(Opcode opcode, ssize_t messageSize)
{
    return messageSize == gameDataSizes[opcode];
}

/*
 * ========================================================
 * ======== MAIN ENTRY POINT FOR WEBSOCKET MESSAGES =======
 * ========================================================
 * This function expects
 * Decoded websocket data and it's length
 * without the websocket headers.
 */
void handleGameMessage(char *data, ssize_t dataSize, Host remotehost)
{
    Opcode *opcode = (Opcode*)data;
    if (*opcode >= MESSAGE_HANDLER_COUNT) {
#ifdef DEBUG
        fprintf(stderr, "\nBad websocket Opcode.\n");
#endif
        return;
    }
#ifdef DEBUG
    printBufferInHex(data, dataSize);
#endif 

    if(isGameMessageValidLength(*opcode, dataSize - sizeof(Opcode))) {
        // Execute the opcode, this function
        // assumes it won't segfault.
        gameMessageHandlers[*opcode](&data[sizeof(Opcode)], dataSize, remotehost);
    };
}

int createGame(Game **game, GameConfig *config)
{
    *game = (Game*)calloc(1, sizeof(Game));
    if ((*game) == NULL) {
        return -1;
    }
    strncpy((*game)->password, config->password, MAX_CREDENTIAL_LEN);
    (*game)->maxPlayerCount = config->maxPlayerCount;
    return 0;
}
int initializeTestGame(GameConfig *config)
{
    Game *game = getTestGame();
    strncpy(game->password, config->password, MAX_CREDENTIAL_LEN);
    game->maxPlayerCount = config->maxPlayerCount;
    return 0;
}

/*
 * This function assumes that the player was redirected to
 * character creation and creates a character at the next free 
 * index in the game
 */
static Player *createPlayer(Game *game, PlayerCredentials *credentials)
{
    Player *newPlayer = &game->players[game->playerCount];

    memcpy (&newPlayer->credentials, credentials, sizeof(PlayerCredentials));

    // Currently, this function is only called
    // from within a critical section within
    // tryPlayerLogin(), so incrementing this
    // is okay.
    game->playerCount++;

    return newPlayer;
}

void  setPlayerCharSheet (Player *player,
                          CharacterSheet *charsheet)
{
    int lock = getMutexIndex(player, sizeof(Player), STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&playerStateLock[lock]);
    memcpy (&player->charSheet, charsheet, sizeof(CharacterSheet)); 
    pthread_mutex_unlock (&playerStateLock[lock]);
}

/*
 * We've just gotten a character sheet
 * parsed out of a html form,
 * but we need to make sure it's
 * not fudged somehow by the client.
 * We then set the "isValid" flag in the 
 * CharacterSheet in question.
 */
void validateNewCharsheet (CharacterSheet *charsheet)
{
    // TODO: literally anything at all
    charsheet->isValid = true;
}

/*
 * This just checks the "isValid" flag
 * in a thread safe manner,
 * see "validateNewCharsheet()"
 * for vibe-checking hackers
 */
bool isCharsheetValid (const Player *player)
{
    bool result = 0;
    int lock = getMutexIndex(player, sizeof(Player), STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&playerStateLock[lock]);
    result = player->charSheet.isValid;
    pthread_mutex_unlock (&playerStateLock[lock]);
    return result;
}

void setGamePassword(Game *restrict game, const char password[static MAX_CREDENTIAL_LEN])
{
    int lock = getMutexIndex(game, sizeof(Game), STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&gameStateLock[lock]);
    memset               (game->password, 
                          0, 
                          MAX_CREDENTIAL_LEN);
    strncpy              (game->password, 
                          password, 
                          MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&gameStateLock[lock]);
}

/*
 * Returns 0 on success and -1 on failure
 */
int tryGameLogin(Game *restrict game, const char *password)
{
    int match = 0;
    int lock = getMutexIndex(game, 
                             sizeof(Game), 
                             STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&gameStateLock[lock]);
    match = strncmp(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&gameStateLock[lock]);
    match = -1 * (match != 0);
    return match;
}

/*
 * Parses an int64 token out of a HTTP
 * message and returns it, or 0 on failure.
 */
long long int getTokenFromHTTP(char *http,
                               int httpLength)
{
    const char    cookieName[HEADER_LENGTH] = "sessionToken=";
    int           startIndex                = stringSearch(http, cookieName, httpLength);
    long long int token                     = 0;

    if (startIndex >= 0) {
        startIndex += strnlen(cookieName, HEADER_LENGTH);
        // TODO: This might overflow with specifically 
        // malformed packets
        token       = strtoll(&http[startIndex], NULL, 10);
    }
    return token;
}

/*
 * Returns NULL when none is found
 * It's all readonly, so it _should_ be
 * thread safe in this specific case...
 */
Player *tryGetPlayerFromToken(SessionToken token,
                              Game *game)
{
    for (int i = 0; i < game->playerCount; i ++) {
        if (token == game->players[i].sessionToken) {
            return &game->players[i];
        }
    }
    return NULL;
}

/*
 * We pass in the game because the
 * session token needs to be unique
 * on a per game basis
 */
static void generateSessionToken(Player *player, Game *game)
{
    long long int nonce = getRandomInt();
    int           i     = 0;
    // Make sure the token is unique, and not 0.
    while ((tryGetPlayerFromToken(nonce, game) != NULL) || (nonce == 0)) {
        nonce = getRandomInt();
        i++;
        if (i > 2) {
            fprintf(stderr, "How the fuck? Kill it with fire.\n");
            exit(1);
        }
    }
    player->sessionToken = nonce;
}

/* 
 * Builds the custom Cookie header that
 * sends the session token to the client.
 */
static void buildSessionTokenHeader(char outHeader[static HEADER_LENGTH], 
                                    SessionToken token)
{
    char headerBase [HEADER_LENGTH] = "Set-Cookie: sessionToken=";
    char tokenString[HEADER_LENGTH] = {0};

    sprintf(tokenString, "%lld\n", token);
    strncat(headerBase, tokenString, HEADER_LENGTH - strlen(headerBase));
    memcpy (outHeader, headerBase, HEADER_LENGTH);
}

/*
 * returns the index of the player in the game
 */
static Player *tryGetPlayerFromCredentials(Game *game, 
                                           const PlayerCredentials *credentials)
{
    int playerIndex   = 0;
    int playerFound   = -1;
    int passwordCheck = -1;

    for (playerIndex = 0; playerIndex < game->playerCount; playerIndex++) {
        playerFound = strncmp(credentials->name, 
                              game->players[playerIndex].credentials.name, 
                              MAX_CREDENTIAL_LEN); 
        if (playerFound == 0) {
            passwordCheck = strncmp(credentials->password, 
                                    game->players[playerIndex].credentials.password, 
                                    MAX_CREDENTIAL_LEN); 
            if (passwordCheck == 0) {
                return &game->players[playerIndex];
            }
        }
    }
    return NULL;
}
/*
 * This function assumes the player had a valid
 * game password, so it'll make them a new
 * character if their credentials don't fit.
 */
int   tryPlayerLogin    (Game *restrict game,
                         PlayerCredentials *credentials,
                         Host remotehost)
{
    Player               *player        = NULL;
    char                  sessionTokenHeader[CUSTOM_HEADERS_MAX_LEN] = {0};

    int lock   = getMutexIndex(game, sizeof(Game), STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&gameStateLock[lock]);
    player = tryGetPlayerFromCredentials(game, credentials);
    if ( player != NULL ) {
           generateSessionToken   (player, 
                                   game);
           buildSessionTokenHeader(sessionTokenHeader, 
                                   player->sessionToken);
           sendContent            ("./game.html", 
                                   HTTP_FLAG_TEXT_HTML, 
                                   remotehost,
                                   sessionTokenHeader);
           pthread_mutex_unlock   (&gameStateLock[lock]);
           return 0;
    }
    // Player was not found in game redirect them to character creation
    // And create a player
    player = 
    createPlayer            (game, 
                             credentials);
    generateSessionToken    (player, 
                             game);
    buildSessionTokenHeader (sessionTokenHeader, 
                             player->sessionToken);
    sendContent             ("./charsheet.html", 
                             HTTP_FLAG_TEXT_HTML, 
                             remotehost, 
                             sessionTokenHeader);

    pthread_mutex_unlock (&gameStateLock[lock]);
    return -1;
}

/*
 * ==============================================================
 * ======= Message handling functions ===========================
 * ==============================================================
 */
static void pingHandler(char *data, ssize_t dataSize, Host remotehost)
{
    printf("Ping incoming!");
}

static void movePlayerHandler(char *data, ssize_t dataSize, Host remotehost)
{
    struct MovePlayerData *moveData = (struct MovePlayerData*)data; 
#ifdef DEBUG
    printBufferInHex(data, dataSize);
#endif 
    printf("\nxCoord: %f\n", moveData->xCoord);
    printf("\nyCoord: %f\n", moveData->yCoord);

    // Here we respond to the clients,
    // telling them all who moved and where.
    Opcode responseOpcode = 0x0001;
    char   gameResponseData[(sizeof(struct MovePlayerData) 
                            + sizeof(Opcode))] = { 0 };
    memcpy (&gameResponseData, &responseOpcode, sizeof(Opcode));
    memcpy (&gameResponseData[sizeof(Opcode)], 
            moveData, 
            sizeof(struct MovePlayerData));


    char multicastBuffer   [(sizeof(struct MovePlayerData) 
                            + sizeof(Opcode)) 
                            + WEBSOCKET_HEADER_SIZE_MAX] = { 0 };
    encodeWebsocketMessage (multicastBuffer, 
                            gameResponseData, 
                            sizeof(struct MovePlayerData) 
                            + sizeof(Opcode));
    multicastTCP           (multicastBuffer, 
                            (sizeof(struct MovePlayerData) 
                            + sizeof(Opcode)) 
                            + WEBSOCKET_HEADER_SIZE_MAX, 
                            0);
}
static void endTurnHandler(char *data, ssize_t dataSize, Host remotehost)
{

}
// Player chose a response to an encounter
static void respondToEventHandler(char *data, ssize_t dataSize, Host remotehost)
{

}
// Player manually used one of their resources
static void useResourceHandler(char *data, ssize_t dataSize, Host remotehost)
{

}

// Client calls this on connect,
// this should either:
// 1. Set an atomic lock on the entire game state while the client connects (easy)
// 2. Allow the game to continue during the connection and apply
//    any state changes that happened since (harder)
static void getGameStateHandler(char *data, ssize_t dataSize, Host remotehost)
{

}

/* ===================================================================
 * =========== Player resources (inventory) management ===============
 * ===================================================================
 */

/*
 * Handlers for interacting with Player Resources.
 * Copy Paste these for different resources a player could 
 * receive, lose, or use.
 */
void baseUseResourceHandler (ResourceID resource, Player *user, Player *target)
{
    // Implement code for using specific resources here
}
void baseGiveResourceHandler (ResourceID resource, Player *target, int count)
{
    // Implement code for receiving resources here, such
    // as adding it to the player's inventory or adding 
    // a passive effect
}
void baseTakeResourceHandler (ResourceID resource, Player *target, int count)
{
   // Implement code for removing an item from a player's
   // inventory here, such as decreasing the count
   // or removing a passive effect.
}

/* 
 *
 * Handlers for when a resource is used
 *      Make sure to list a handler FOR EACH existing resource ID
 *
 */
static UseResourceHandler useResourceHandlers[RESOURCE_COUNT] = {
    baseUseResourceHandler
};
// Handlers for when a resource is given to a player
static GiveResourceHandler giveResourceHandlers[RESOURCE_COUNT] = {
    baseGiveResourceHandler
};
// Handlers for when a resource is taken from a player
static TakeResourceHandler takeResourceHandlers[RESOURCE_COUNT] = {
    baseTakeResourceHandler
};
