/*
 * ===========================
 * websocket_handlers.h
 * ===========================
 * The client will send websocket packets to indicate
 * what it is doing in the game.
 * This set of functions handles the incoming packets
 * and coordinates with the game logic, and other components,
 * to respond.
 */

#ifndef WEBSOCKET_HANDLERS
#define WEBSOCKET_HANDLERS

#include <stdint.h>
#include "game_logic.h"
#include "bbnetlib.h"

/*
 * Data structures coupled with websocket
 * handlers
 */
// REQUESTS //
struct player_move_req {
    double x_coord;
    double y_coord;
} __attribute__((packed));

struct player_conn_req {
    // We don't actually need anything from
    // the client on connection.
    // But one day we might!
    char placeholder;
} __attribute__((packed));

// RESPONSES //
struct player_move_res {
    player_id_t player_id;
    struct player_move_req coords;
} __attribute__((packed));

struct player_conn_res {
    player_id_t players[MAX_PLAYERS_IN_GAME];
    char player_names[MAX_CREDENTIAL_LEN][MAX_PLAYERS_IN_GAME];
    struct coordinates player_coords[MAX_PLAYERS_IN_GAME];
    player_id_t current_turn;         // ID of the player who's turn it is
    player_id_t connecting_player_id; // ID of the player who connected
    bool game_ongoing;                // Has the game we've joined started yet?
} __attribute__((packed));

/*
 * Primary entry point for interpreting
 * incoming websocket messages
 */
void handle_game_message(char *data,
                         ssize_t data_size,
                         struct host *remotehost);

#endif
