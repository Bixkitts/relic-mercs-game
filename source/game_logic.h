#ifndef BB_GAME_LOGIC
#define BB_GAME_LOGIC

#include <unistd.h>

#include "bbnetlib.h"

typedef enum {
    GAME_MSG_PING,
    GAME_MSG_CHARACTER_MOVE,
    GAME_MSG_COUNT
}GameMessage;

void handleGameMessage(char *data, ssize_t dataSize, Host remotehost);

#endif
