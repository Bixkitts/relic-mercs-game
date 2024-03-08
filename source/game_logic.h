#ifndef BB_GAME_LOGIC
#define BB_GAME_LOGIC

#include <unistd.h>

#include "bbnetlib.h"

#define MAX_CREDENTIAL_LEN   32

#define MAX_RESOURCES_IN_INV 128
#define MAX_PLAYERS_IN_GAME  16

typedef struct Resource Resource;
typedef struct Player Player;
typedef struct Game Game;

typedef struct GameConfig {
    char password[MAX_CREDENTIAL_LEN];
    int  maxPlayerCount;
} GameConfig;
void handleGameMessage(char *data, ssize_t dataSize, Host remotehost);

/*
 * Authentication
 */
void  getGamePassword   (Game *restrict game, 
                         char* outPassword[static MAX_CREDENTIAL_LEN]);
void  setGamePassword   (Game *restrict game, 
                         const char *password);
// This just returns 0 on success
// and -1 on failure
int   tryGameLogin      (Game *restrict game,
                         const char *password);
// Will associate a remotehost with a player,
// or create a new player and redirect
// to character creator when there isn't an
// existing one.
int   tryPlayerLogin    (Game *game,
                         char *playerName, 
                         char *password, 
                         Host remotehost);
/* --------------------------------------------- */
int   createGame      (Game **game, 
                       GameConfig *config);
// Returns the pointer to global Game
// variable.
// Not thread safe, set this exactly once.
Game *getTestGame     ();


#endif
