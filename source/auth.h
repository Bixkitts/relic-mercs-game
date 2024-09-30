#ifndef BB_GAME_AUTH
#define BB_GAME_AUTH

#include "game_logic.h"
#include "session_token.h"

#define INVALID_SESSION_TOKEN 0
/* A randomly generated token
 * will collide with another one
 * this many times before
 * crashing the program
 */
#define TOKEN_GEN_LIMIT       3

// This just returns 0 on success
// and -1 on failure
int tryGameLogin(struct Game *restrict game, const char *password);
// Will associate a remotehost with a player,
// or create a new player and redirect
// to character creator when there isn't an
// existing one.
int tryPlayerLogin(struct Game *restrict game,
                   struct PlayerCredentials *restrict credentials,
                   struct host *remotehost);

SessionToken getTokenFromHTTP(char *http, int httpLength);

#endif
