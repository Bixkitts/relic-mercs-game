#ifndef BB_GAME_LOGIC
#define BB_GAME_LOGIC

#include <unistd.h>

#include "bbnetlib.h"
#include "html_server.h"

#define MAX_CREDENTIAL_LEN   32

#define MAX_RESOURCES_IN_INV 128
#define MAX_PLAYERS_IN_GAME  16

typedef struct Resource Resource;
typedef struct Player Player;
typedef struct Game Game;

typedef long long int SessionToken;

typedef struct GameConfig {
    char password[MAX_CREDENTIAL_LEN];
    int  maxPlayerCount;
} GameConfig;

typedef struct PlayerCredentials {
    char name     [MAX_CREDENTIAL_LEN];
    char password [MAX_CREDENTIAL_LEN];
} PlayerCredentials;

typedef struct CharacterSheet {
    int vigour;
    int violence;
    int guile;
} CharacterSheet;

/*
 * Primary entry point for interpreting
 * incoming websocket messages
 */
void handleGameMessage(char *data, ssize_t dataSize, Host remotehost);
typedef uint16_t Opcode;

/*
 * Authentication
 * ----------------------------------------------- */
void         getGamePassword        (Game *restrict game, 
                                     char outPassword[static MAX_CREDENTIAL_LEN]);
void         setGamePassword        (Game *restrict game, 
                                     const char password[static MAX_CREDENTIAL_LEN]);
// This just returns 0 on success
// and -1 on failure
int          tryGameLogin           (Game *restrict game,
                                     const char *password);
// Will associate a remotehost with a player,
// or create a new player and redirect
// to character creator when there isn't an
// existing one.
int          tryPlayerLogin         (Game *restrict game,
                                     PlayerCredentials *credentials,
                                     Host remotehost);
// returns the associated player ID
// if the session token is valid
Player       *tryGetPlayerFromToken  (SessionToken token,
                                      Game *game);
void          generateSessionToken   (Player *player,
                                      Game *game);
long long int getTokenFromHTTP       (char *http,
                                      int httpLength);
/* --------------------------------------------- */

int   createGame         (Game **game, 
                          GameConfig *config);
// returns the ID the player got
// Returns the pointer to global Game
// variable.
// Not thread safe, set this exactly once
// in the main thread.
Game *getTestGame        ();
int   getPlayerCount     (const Game *game);
void  setPlayerCharSheet (Player *player,
                          CharacterSheet *charsheet);


#endif
