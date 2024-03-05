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

typedef void (*GameMessageHandler)(char* data, ssize_t dataSize, Host remotehost);

static void pingHandler              (char *data, ssize_t dataSize, Host remotehost);
static void characterMoveHandler     (char *data, ssize_t dataSize, Host remotehost);

static GameMessageHandler handlers[GAME_MSG_COUNT] = {
    pingHandler,
    characterMoveHandler
};

/*
 * This function expects
 * Decoded websocket data and it's length
 * without the websocket headers
 */
void handleGameMessage(char *data, ssize_t dataSize, Host remotehost)
{
    // Execute the opcode, this function
    // assumes it won't segfault.
    handlers[(uint16_t)data[0]](data, dataSize, remotehost);
}

static void pingHandler(char *data, ssize_t dataSize, Host remotehost)
{
    // We got a ping to keep the TCP connection open, do nothing.
}

static void characterMoveHandler(char *data, ssize_t dataSize, Host remotehost)
{
 
}
