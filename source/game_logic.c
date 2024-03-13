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

#define MESSAGE_HANDLER_COUNT 7

// All injuries are followed immediately by their 
// healed counterparts.
typedef enum {
    INJURY_NOTHING,
    INJURY_DEEP_CUT,
    INJURY_BIG_SCAR,
    INJURY_BROKEN_LEFT_LEG,
    INJURY_BROKEN_RIGHT_LEG,
    INJURY_BROKEN_RIGHT_ARM,
    INJURY_BROKEN_LEFT_ARM,
    INJURY_MISSING_LEFT_LEG,
    INJURY_MISSING_RIGHT_LEG,
    INJURY_MISSING_LEFT_ARM,
    INJURY_MISSING_RIGHT_ARM,
    INJURY_COUNT
}InjuryType;

// These ID's will be direct
// callbacks to handle these encounters
// serverside
typedef enum {
    ENCOUNTER_BANDITS,
    ENCOUNTER_TROLLS,
    ENCOUNTER_WOLVES,
    ENCOUNTER_RATS,
    ENCOUNTER_BEGGAR,
    ENCOUNTER_DRAGONS,
    ENCOUNTER_TREASURE_SMALL,
    ENCOUNTER_TREASURE_LARGE,
    ENCOUNTER_TREASURE_MAGICAL,
    ENCOUNTER_RUINS_OLD,
    ENCOUNTER_SPELL_TOME,
    ENCOUNTER_CULTISTS_CANNIBAL,
    ENCOUNTER_CULTISTS_PEACEFUL,
    ENCOUNTER_COUNT
}EncounterID;

typedef enum {
    ENCOUNTER_TYPE_BANDIT,
    ENCOUNTER_TYPE_BEASTS,
    ENCOUNTER_TYPE_MONSTROSITIES,
    ENCOUNTER_TYPE_DRAGON,
    ENCOUNTER_TYPE_MYSTICAL,
    ENCOUNTER_TYPE_CULTISTS,
    ENCOUNTER_TYPE_SLAVERS,
    ENCOUNTER_TYPE_REFUGEES,
    ENCOUNTER_TYPE_EXILES,
    ENCOUNTER_TYPE_PLAGUE,
    ENCOUNTER_TYPE_COUNT
}EncounterTypeID;

typedef enum {
    RESOURCE_KNIFE,
    RESOURCE_SWORD,
    RESOURCE_AXE,
    RESOURCE_POTION_HEAL,
    RESOURCE_COUNT
} ResourceID;

/*
 * Encounter Categories, and their possible encounters
 */
// NOTE: Every encounter category needs at least one specific
// encounter in it.
static int encounterCategories[ENCOUNTER_TYPE_COUNT][ENCOUNTER_COUNT]= {
    {ENCOUNTER_BANDITS},                 // ENCOUNTER_TYPE_BANDIT
    {ENCOUNTER_WOLVES,                   // ENCOUNTER_TYPE_BEASTS
     ENCOUNTER_RATS},
    {ENCOUNTER_TROLLS},                  // ENCOUNTER_TYPE_MONSTROSITIES
    {ENCOUNTER_DRAGONS},                 // ENCOUNTER_TYPE_DRAGON
    {ENCOUNTER_RUINS_OLD,                // ENCOUNTER_TYPE_MYSTICAL
     ENCOUNTER_TREASURE_MAGICAL,
     ENCOUNTER_SPELL_TOME},
    {ENCOUNTER_CULTISTS_CANNIBAL,        // ENCOUNTER_TYPE_CULTISTS
     ENCOUNTER_CULTISTS_PEACEFUL}
};

typedef void (*GameMessageHandler)  (char* data, ssize_t dataSize, Host remotehost);

typedef void (*UseResourceHandler)  (ResourceID resource, Player *user, Player *target);
typedef void (*GiveResourceHandler) (ResourceID resource, Player *target, int count);
typedef void (*TakeResourceHandler) (ResourceID resource, Player *target, int count);

/*
 * State data structures
 */
struct Player {
    Host              associatedHost;
    PlayerCredentials credentials;
    SessionToken      sessionToken;
    CharacterSheet    charSheet;
    int               xCoord;
    int               yCoord;
    // How many of each ResourceID the player has
    int               resources[RESOURCE_COUNT];
    bool              isBanned;
};
struct Game {
    // Who's turn is it
    int     playerTurn;
    int     maxPlayerCount;
    // This is larger than max players to account
    // for kicked and banned players
    Player  players [MAX_PLAYERS_IN_GAME * 2];
    int     playerCount;
    char    password[MAX_CREDENTIAL_LEN];
};

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

/* H
 * Helpers and Authentication
 */
static int  isGameDataValidLength         (Opcode opcode, 
                                           ssize_t messageSize);
static void buildSessionTokenHeader       (char outHeader[static HEADER_LENGTH], 
                                           SessionToken token);
static int  findPlayerFromCredentials     (const Game *game, 
                                           const PlayerCredentials *credentials);
static int  createPlayer                  (Game *game,
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
// Client calls this each tick
static void getGameStateChangeHandler   (char *data, ssize_t dataSize, Host remotehost);
// Client calls this on connect,
// this should either:
// 1. Set an atomic lock on the entire game state while the client connects (easy)
// 2. Allow the game to continue during the connection and apply
//    any state changes that happened since (harder)
static void getGameStateHandler         (char *data, ssize_t dataSize, Host remotehost);

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

/*
 * Primary interpreter for incoming websocket messages
 * carrying valid gameplay opcodes.
 */
static GameMessageHandler gameMessageHandlers[MESSAGE_HANDLER_COUNT] = {
    pingHandler,
    movePlayerHandler, 
    endTurnHandler,           
    respondToEventHandler, 
    getGameStateChangeHandler,
    getGameStateHandler         
};
static int gameDataSizes[MESSAGE_HANDLER_COUNT] = {
    0,
    sizeof(struct MovePlayerData),
    0,
    0,
    0,
    0,
    0
};

static int isGameDataValidLength(Opcode opcode, ssize_t messageSize)
{
    return messageSize == gameDataSizes[opcode];
}

/*
 * This function expects
 * Decoded websocket data and it's length
 * without the websocket headers
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

    if(isGameDataValidLength(*opcode, dataSize - sizeof(Opcode))) {
        // Execute the opcode, this function
        // assumes it won't segfault.
        gameMessageHandlers[*opcode](&data[sizeof(Opcode)], dataSize, remotehost);
    };
}

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
// Client calls this each tick
static void getGameStateChangeHandler(char *data, ssize_t dataSize, Host remotehost)
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

/*
 * This function assumes that the player was redirected to
 * character creation and creates a character at the next free 
 * index in the game
 */
int createPlayer(Game *game, PlayerCredentials *credentials)
{
    Player *newPlayer = &game->players[game->playerCount];

    int lock = getMutexIndex(newPlayer, sizeof(Player), STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&playerStateLock[lock]);
    memcpy (&newPlayer->credentials, credentials, sizeof(PlayerCredentials));
    pthread_mutex_unlock (&playerStateLock[lock]);

    return game->playerCount;
}

void  setPlayerCharSheet (Player *player,
                          CharacterSheet *charsheet)
{
    int lock = getMutexIndex(player, sizeof(Player), STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&playerStateLock[lock]);
    memcpy (&player->charSheet, charsheet, sizeof(CharacterSheet)); 
    pthread_mutex_unlock (&playerStateLock[lock]);
}

void getGamePassword(Game *restrict game, char outPassword[static MAX_CREDENTIAL_LEN])
{
    int lock = getMutexIndex(game, sizeof(Game), STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&gameStateLock[lock]);
    memcpy               (outPassword, 
                          game->password, 
                          MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&gameStateLock[lock]);
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
    bool match = 0;
    int lock = getMutexIndex(game, 
                             sizeof(Game), 
                             STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&gameStateLock[lock]);
    match = strncmp(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&gameStateLock[lock]);
    match = -1 * (match != 0);
    return match;
}

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
 */
Player *tryGetPlayerFromToken(SessionToken token,
                              Game *game)
{
    int lock = getMutexIndex(game, 
                             sizeof(Game), 
                             STATE_MUTEX_COUNT);
    pthread_mutex_lock   (&gameStateLock[lock]);
    for (int i = 0; i < game->playerCount; i ++) {
        if (token == game->players[i].sessionToken) {
            pthread_mutex_unlock (&gameStateLock[lock]);
            return &game->players[i];
        }
    }
    pthread_mutex_unlock (&gameStateLock[lock]);
    return NULL;
}

void generateSessionToken(Player *player, Game *game)
{
    long long int nonce = getRandomInt();
    int           i     = 0;
    // Make sure the token is unique
    while (tryGetPlayerFromToken(nonce, game) >= 0) {
        nonce = getRandomInt();
        i++;
        if (i > 2) {
            fprintf(stderr, "How the fuck? Kill it with fire.\n");
            exit(1);
        }
    }
    int lock   = (unsigned long)game % STATE_MUTEX_COUNT;
    pthread_mutex_lock   (&gameStateLock[lock]);
    player->sessionToken = nonce;
    pthread_mutex_unlock (&gameStateLock[lock]);
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
static int findPlayerFromCredentials(const Game *game, 
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
                return playerIndex;
            }
        }
    }
    return -1;
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
    int                   playerIndex   = 0;
    SessionToken          sessionToken  = 0;
    char                  cookieHeader[CUSTOM_HEADERS_MAX_LEN] = {0};

    int lock   = (unsigned long)game % STATE_MUTEX_COUNT;
    pthread_mutex_lock   (&gameStateLock[lock]);
    if (findPlayerFromCredentials(game, credentials) >= 0) {
           generateSessionToken   (&game->players[playerIndex], 
                                   game);
           buildSessionTokenHeader(cookieHeader, 
                                   sessionToken);
           sendContent            ("./index.html", 
                                   HTTP_FLAG_TEXT_HTML, 
                                   remotehost,
                                   cookieHeader);
           pthread_mutex_unlock   (&gameStateLock[lock]);
           return 0;
    }
    // Player was not found in game redirect them to character creation
    // And create a player
    createPlayer            (game, credentials);
    generateSessionToken    (&game->players[playerIndex], 
                             game);
    buildSessionTokenHeader (cookieHeader, 
                             sessionToken);
    sendContent             ("./charsheet.html", 
                             HTTP_FLAG_TEXT_HTML, 
                             remotehost, 
                             cookieHeader);

    pthread_mutex_unlock (&gameStateLock[lock]);
    return -1;
}
