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
struct Game {
    // Who's turn is it
    int     playerTurn;
    int     maxPlayerCount;
    // Map weights for different
    // encounter types in which regions
    Player *players [MAX_PLAYERS_IN_GAME];
    int     playerCount;
    char    password[MAX_CREDENTIAL_LEN];
};

struct Player {
    int        playerID;
    Host       associatedHost;
    char       password[MAX_CREDENTIAL_LEN];
    char       name    [MAX_CREDENTIAL_LEN];
    int        xCoord;
    int        yCoord;
    // How many of each ResourceID the player has
    int        resources  [RESOURCE_COUNT];
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

/*
 * Handlers for incoming messages from the websocket connection
 */
static void pingHandler                 (char *data, ssize_t dataSize, Host remotehost);
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

/*
 * This function expects
 * Decoded websocket data and it's length
 * without the websocket headers
 */
void handleGameMessage(char *data, ssize_t dataSize, Host remotehost)
{
    uint16_t opcode = (uint16_t) data[0];
    if (opcode >= MESSAGE_HANDLER_COUNT) {
#ifdef DEBUG
        fprintf(stderr, "\nBad websocket Opcode.\n");
#endif
        return;
    }
    // Execute the opcode, this function
    // assumes it won't segfault.
    gameMessageHandlers[(uint16_t)data[0]](data, dataSize, remotehost);
}


static void pingHandler                 (char *data, ssize_t dataSize, Host remotehost)
{

}
static void movePlayerHandler           (char *data, ssize_t dataSize, Host remotehost)
{

}
static void endTurnHandler              (char *data, ssize_t dataSize, Host remotehost)
{

}
// Player chose a response to an encounter
static void respondToEventHandler       (char *data, ssize_t dataSize, Host remotehost)
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

void getGamePassword(Game *restrict game, char outPassword[static MAX_CREDENTIAL_LEN])
{
    int lock = (unsigned long)game % STATE_MUTEX_COUNT;
    pthread_mutex_lock   (&gameStateLock[lock]);
    memcpy               (outPassword, 
                          game->password, 
                          MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&gameStateLock[lock]);
}

void setGamePassword(Game *restrict game, const char password[static MAX_CREDENTIAL_LEN])
{
    int lock = (unsigned long)game % STATE_MUTEX_COUNT;
    pthread_mutex_lock   (&gameStateLock[lock]);
    memset               (game->password, 
                          0, 
                          MAX_CREDENTIAL_LEN);
    strncpy              (game->password, 
                          password, 
                          MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&gameStateLock[lock]);
}

int tryGameLogin(Game *restrict game, const char *password)
{
    bool match = 0;
    int lock   = (unsigned long)game % STATE_MUTEX_COUNT;
    pthread_mutex_lock   (&gameStateLock[lock]);
    match = strncmp(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&gameStateLock[lock]);
    match = -1 * (match != 0);
    return match;
}

/*
 * This function assumes the player had a valid
 * game password, so it'll make them a new
 * character if their credentials don't fit.
 */
// TODO: Not done yet, but this is the general idea.
int   tryPlayerLogin    (Game *restrict game,
                         char playerName[static MAX_CREDENTIAL_LEN], 
                         char password[static MAX_CREDENTIAL_LEN], 
                         Host remotehost)
{
    int                   playerFound   = -1;
    HostCustomAttributes *hostAttr      = (HostCustomAttributes*)getHostCustomAttr(remotehost);
    int                   playerIndex   = 0;
    int                   passwordCheck = -1;

    int lock   = (unsigned long)game % STATE_MUTEX_COUNT;
    pthread_mutex_lock   (&gameStateLock[lock]);

    for (playerIndex = 0; playerIndex < game->playerCount; playerIndex++) {
        playerFound = strncmp(playerName, game->players[playerIndex]->name, MAX_CREDENTIAL_LEN); 
        if (playerFound == 0) {
            goto player_exists;
        }
    }
    // Player was not found in game redirect them to character creation
    sendContent("./charsheet.html", HTTP_FLAG_TEXT_HTML, remotehost);
    pthread_mutex_unlock (&gameStateLock[lock]);
    return 0;
player_exists:
    passwordCheck = strncmp(password, game->players[playerIndex]->password, MAX_CREDENTIAL_LEN); 
    if (passwordCheck == 0) {
        hostAttr->player = game->players[playerIndex];
        game->players[playerIndex]->associatedHost = remotehost;
        sendContent("./index.html", HTTP_FLAG_TEXT_HTML, remotehost);
    }
    // login was unsuccessful...
    pthread_mutex_unlock (&gameStateLock[lock]);
    return -1;
}
