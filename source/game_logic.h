#ifndef BB_GAME_LOGIC
#define BB_GAME_LOGIC

#include <unistd.h>

#include "bbnetlib.h"
#include "html_server.h"
#include "helpers.h"

#define MAX_CREDENTIAL_LEN   32
#define MAX_PLAYERS_IN_GAME  16

struct Resouce;
struct Player;
struct Game;
struct CharacterSheet;

typedef long long int SessionToken;

struct GameConfig {
    char password[MAX_CREDENTIAL_LEN];
    int  maxPlayerCount;
};

struct PlayerCredentials {
    char name     [MAX_CREDENTIAL_LEN];
    char password [MAX_CREDENTIAL_LEN];
};

/*
 * Primary entry point for interpreting
 * incoming websocket messages
 */
void handleGameMessage(char *data, 
                       ssize_t dataSize, 
                       Host remotehost);
typedef uint16_t Opcode;

/*
 * Authentication
 * ----------------------------------------------- */
void         
setGamePassword         (struct Game *restrict game, 
                         const char password[static MAX_CREDENTIAL_LEN]);
// This just returns 0 on success
// and -1 on failure
int          
tryGameLogin            (struct Game *restrict game,
                         const char *password);
// Will associate a remotehost with a player,
// or create a new player and redirect
// to character creator when there isn't an
// existing one.
int          
tryPlayerLogin          (struct Game *restrict game,
                         struct PlayerCredentials *restrict credentials,
                         Host remotehost);

long long int 
getTokenFromHTTP        (char *http,
                         int httpLength);
struct Player 
*tryGetPlayerFromToken  (SessionToken token,
                         struct Game *restrict game);
// Character sheet setup stuff
int  
initCharsheetFromForm   (struct Player *charsheet, 
                         const struct HTMLForm *form);
bool 
isCharsheetValid        (struct Player *restrict player);
// Note: use initCharsheetFromForm.
void 
setPlayerCharSheet      (struct Player *player,
                         struct CharacterSheet *charsheet);
/* --------------------------------------------- */

int   createGame         (struct Game **game, 
                          struct GameConfig *config);
int   initializeTestGame (struct GameConfig *config);
// returns the ID the player got
// Returns the pointer to global Game
// variable.
// Not thread safe, set this exactly once
// in the main thread.
struct Game *getTestGame        ();
int          getPlayerCount     (const struct Game *game);
#endif
