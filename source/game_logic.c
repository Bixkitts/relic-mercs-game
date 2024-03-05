#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "bbnetlib.h"
#include "host_custom_attributes.h"
#include "error_handling.h"
#include "helpers.h"
#include "websockets.h"
#include "html_server.h"
#include "game_logic.h"

#define MESSAGE_HANDLER_COUNT 7

typedef void (*GameMessageHandler)(char* data, ssize_t dataSize, Host remotehost);

typedef void (*UseResourceHandler)  (ResourceID resource, Player *user, Player *target);
typedef void (*GiveResourceHandler) (ResourceID resource, Player *target, int count);
typedef void (*TakeResourceHandler) (ResourceID resource, Player *target, int count);

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

// Handlers for when a resource is used
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
static void useResourceHandler          (char *data, ssize_t dataSize, Host remotehost)
{

}
// Client calls this each tick
static void getGameStateChangeHandler   (char *data, ssize_t dataSize, Host remotehost)
{

}
// Client calls this on connect,
// this should either:
// 1. Set an atomic lock on the entire game state while the client connects (easy)
// 2. Allow the game to continue during the connection and apply
//    any state changes that happened since (harder)
static void getGameStateHandler         (char *data, ssize_t dataSize, Host remotehost)
{

}
