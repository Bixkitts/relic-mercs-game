/*
 * ===========================
 * websocket_handlers.c
 * ===========================
 * The client will send websocket packets to indicate
 * what it is doing in the game.
 * This set of functions handles the incoming packets
 * and coordinates with the game logic, and other components,
 * to respond.
 */

#include <string.h>

#include "host_custom_attributes.h"
#include "websocket_handlers.h"
#include "websockets.h"
#include "validators.h"

#define MAX_RESPONSE_HEADER_SIZE WEBSOCKET_HEADER_SIZE_MAX + sizeof(opcode_t)
#define MESSAGE_HANDLER_COUNT    3
#define EMPTY_OPCODE             0 // Some opcodes just give the server no data

/*
 * Primary interpreter for incoming websocket messages
 * carrying valid gameplay opcodes.
 * Player disconnects are handled in packet_handlers.c,
 * and are not seen by the websocket layer.
 */

/*
 * Handler type definitions
 */
typedef void (*game_message_handler_t)(char *data,
                                       ssize_t data_size,
                                       struct host *remotehost);
typedef void (*use_resource_handler_t)(enum resource_id resource,
                                       struct player *user,
                                       struct player *target);
typedef void (*give_resource_handler_t)(enum resource_id resource,
                                        struct player *target,
                                        int count);
typedef void (*take_resource_handler_t)(enum resource_id resource,
                                        struct player *target,
                                        int count);


static int request_sizes[MESSAGE_HANDLER_COUNT] = {
    EMPTY_OPCODE,
    sizeof(struct player_move_req),
    sizeof(struct player_conn_req),
};
static int response_sizes[MESSAGE_HANDLER_COUNT] = {
    EMPTY_OPCODE,
    sizeof(struct player_move_res),
    sizeof(struct player_conn_res),
};

enum response_opcodes {
    OPCODE_PING,
    OPCODE_PLAYER_MOVE,
    OPCODE_PLAYER_CONNECT
};

static inline int is_game_message_valid_length(opcode_t opcode,
                                               ssize_t message_size);
/*
 * These functions handle incoming websocket packets.
 */
static void ping_handler(char *data,
                         ssize_t data_size,
                         struct host *remotehost);
static void move_player_handler(char *data,
                                ssize_t data_size,
                                struct host *remotehost);
static void player_connect_handler(char *data,
                                   ssize_t data_size,
                                   struct host *remotehost);

static game_message_handler_t game_message_handlers[MESSAGE_HANDLER_COUNT] = {
    ping_handler,
    move_player_handler,
    player_connect_handler,
};

/*
 * There's an opcode, and then
 * serialised state data.
 * This state data should match a
 * corresponding data structure perfectly,
 * or be rejected.
 */
static inline int is_game_message_valid_length(opcode_t opcode,
                                               ssize_t message_size)
{
    return message_size == request_sizes[opcode];
}

/*
 * This will run at the start of most
 * websocket handlers to prepare a buffer for writing
 * data to, and takes care of writing the websocket
 * header and opcode data.
 *
 * Response payload should ALWAYS be
 * a single type corresponding to the opcode and what's
 * tracked in gameDataSizes[].
 *
 * Returns the amount of bytes it wrote to the buffer.
 */
static int init_handler_response_buffer(char *response_buffer, opcode_t code)
{
    int header_size            = 0;
    ssize_t response_data_size = response_sizes[code];

    header_size = write_websocket_header(response_buffer,
                                         sizeof(code) + response_data_size);
    memcpy(response_buffer + header_size, &code, sizeof(code));
    return header_size + sizeof(code);
}

/*
 * ========================================================
 * ======== MAIN ENTRY POINT FOR WEBSOCKET MESSAGES =======
 * ========================================================
 * This function expects
 * Decoded websocket data and it's length
 * without the websocket headers.
 */
void handle_game_message(char *data, ssize_t data_size, struct host *remotehost)
{
    opcode_t opcode = 0;
    // memcpy because of pointer aliasing
    memcpy(&opcode, data, sizeof(opcode));
    if (opcode >= MESSAGE_HANDLER_COUNT) {
#ifdef DEBUG
        fprintf(stderr, "\nBad websocket opcode.\n");
#endif
        return;
    }
#ifdef DEBUG
    print_buffer_in_hex(data, data_size);
#endif

    if (is_game_message_valid_length(opcode, data_size - sizeof(opcode_t))) {
        game_message_handlers[opcode](&data[sizeof(opcode_t)],
                                      data_size,
                                      remotehost);
    };
}

static void ping_handler(char *data, ssize_t data_size, struct host *remotehost)
{
#ifdef DEBUG
    printf("Ping incoming!");
#endif
    const opcode_t response_opcode                 = OPCODE_PING;
    char response_buffer[MAX_RESPONSE_HEADER_SIZE] = {0};
    int packet_size =
        init_handler_response_buffer(response_buffer, response_opcode);
    send_data_tcp(response_buffer, (ssize_t)packet_size, remotehost);
}

static void move_player_handler(char *data,
                                ssize_t data_size,
                                struct host *remotehost)
{
    struct player_move_res *response_data   = NULL;
    const struct player_move_req *move_data = (struct player_move_req *)data;
    const opcode_t response_opcode          = OPCODE_PLAYER_MOVE;
    int header_size                         = 0;
    struct player *host_player              = get_player_from_host(remotehost);

    char response_buffer[MAX_RESPONSE_HEADER_SIZE + sizeof(*response_data)] = {
        0};
    header_size =
        init_handler_response_buffer(response_buffer, response_opcode);
    response_data = (struct player_move_res *)&response_buffer[header_size];

    validate_player_move_coords(move_data, &response_data->coords);
    response_data->player_id = host_player->id;

    host_player->coords.x = response_data->coords.x_coord;
    host_player->coords.y = response_data->coords.y_coord;

    int packet_size = header_size + sizeof(*response_data);
    multicast_tcp(response_buffer, packet_size, 0);
}

static void construct_player_connect_response(struct player_conn_res *response_data,
                                              const struct game *game,
                                              const struct player *player_connecting)
{
    const ssize_t namelen = sizeof(game->players[0].credentials.name);
    for (int i = 0; i < MAX_PLAYERS_IN_GAME; i++) {
        if (!game->players[i].game) {
            response_data->players[i] = INVALID_PLAYER_ID;
            continue;
        }
        const player_id_t id      = game->players[i].id;
        response_data->players[i] = id;
        memcpy(&response_data->player_names[i],
               game->players[i].credentials.name,
               namelen);
        memcpy(&response_data->player_coords[i],
               &game->players[i].coords,
               sizeof(game->players[i].coords));
    }
    // On the client side we build an id->name map
    // from the previous data.
    // This entry allows the client to know
    // which player in the map they are.
    response_data->connecting_player_id = player_connecting->id;
}

/*
 * The client is attempting to fetch the player net_ids
 * so it can interpret messages about player state
 * changes
 */
static void player_connect_handler(char *data,
                                   ssize_t data_size,
                                   struct host *remotehost)
{
    // TODO:
    // When somebody connects, they could be connecting to
    // an ongoing game when someone, or themselves, are
    // in the middle of an encounter or other dialog.
    // This will need to be communicated.
    const opcode_t response_opcode         = OPCODE_PLAYER_CONNECT;
    const struct player *player_connecting = get_player_from_host(remotehost);
    struct game *game                      = player_connecting->game;
    if (!game) {
        return;
    }

    char response_buffer[MAX_RESPONSE_HEADER_SIZE
                         + sizeof(struct player_conn_res)] = {0};
    int header_size =
        init_handler_response_buffer(response_buffer, response_opcode);
    struct player_conn_res *const response_data =
        (struct player_conn_res *)&response_buffer[header_size];

    construct_player_connect_response(response_data, game, player_connecting);
    // It's *nobody's* turn?
    // This means the game hasn't started yet.
    if (!game->current_turn) {
        try_start_game(game);
    }
    if (game->state == GAME_STATE_STARTED) {
        response_data->current_turn = game->current_turn->id;
    }
    else {
        response_data->current_turn = INVALID_PLAYER_ID;
    }

    const ssize_t packet_size = header_size + sizeof(*response_data);
    multicast_tcp(response_buffer, packet_size, 0);
}
