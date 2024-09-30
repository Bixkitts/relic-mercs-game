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
int try_game_login(struct game *restrict game, const char *password);
// Will associate a remotehost with a player,
// or create a new player and redirect
// to character creator when there isn't an
// existing one.
int try_player_login(struct game *restrict game,
                     struct player_credentials *restrict credentials,
                     struct host *remotehost);

session_token_t get_token_from_http(char *http, int http_length);

#endif
