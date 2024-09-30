#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "auth.h"
#include "bbnetlib.h"
#include "error_handling.h"
#include "game_logic.h"
#include "helpers.h"
#include "host_custom_attributes.h"
#include "net_ids.h"
#include "validators.h"
#include "websockets.h"

#define MAX_RESPONSE_HEADER_SIZE WEBSOCKET_HEADER_SIZE_MAX + sizeof(opcode_t)

// This is coupled with enum PlayerBackground
// and also coupled on the clientside
static const char player_background_strings[PLAYER_BACKGROUND_COUNT]
                                           [HTMLFORM_FIELD_MAX_LEN] =
                                               {"Trader",
                                                "Farmer",
                                                "Warrior",
                                                "Priest",
                                                "Cultist",
                                                "Diplomat",
                                                "Slaver",
                                                "Monster+Hunter",
                                                "Clown"};

// This is coupled with enum Gender
static const char player_gender_strings[GENDER_COUNT][HTMLFORM_FIELD_MAX_LEN] =
    {"Male", "Female"};

pthread_mutex_t net_obj_mutexes[MAX_NETOBJS] = {0};

const char test_game_name[MAX_CREDENTIAL_LEN] = "test game";

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

/*
 * Central global list of games
 */
struct game_slot {
    atomic_int in_use;
    struct game game;
    pthread_mutex_t game_mutex;
};
static struct game_slot game_list[MAX_GAMES] = {0};
atomic_int game_count                        = 0;

/*
 * Returns a pointer to the corresponding
 * game from the global list, or NULL
 * if no name matched
 */
struct game *get_game_from_name(const char name[static MAX_CREDENTIAL_LEN])
{
    int cmp = -1;
    for (int i = 0; i < MAX_GAMES; i++) {
        cmp = strncmp(game_list[i].game.name, name, MAX_CREDENTIAL_LEN);
        if (cmp == 0) {
            return &game_list[i].game;
        }
    }
    return NULL;
}

/*
 * Helpers and Authentication
 * ----------------------------
 *  None of these have mutex locks in them,
 *  the functions that call them are expected
 *  to lock the game state they are modifying.
 */
// TODO: move these to validator.h
static inline int is_game_message_valid_length(opcode_t opcode,
                                               ssize_t message_size);
static void gen_player_start_pos(struct coordinates *out_coords);
/*
 * Handlers for incoming messages from the websocket connection
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

/*
 * Primary interpreter for incoming websocket messages
 * carrying valid gameplay opcodes.
 * Player disconnects are handled in packet_handlers.c,
 * and are not seen by the websocket layer.
 */
#define MESSAGE_HANDLER_COUNT 3
static game_message_handler_t game_message_handlers[MESSAGE_HANDLER_COUNT] = {
    ping_handler,
    move_player_handler,
    player_connect_handler,
};
#define EMPTY_OPCODE 0 // Some opcodes just give the server no data
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
        fprintf(stderr, "\nBad websocket opcode_t.\n");
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

static inline int get_free_game()
{
    int expected = 0;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (atomic_compare_exchange_strong(&game_list[i].in_use,
                                           &expected,
                                           1)) {
            return i;
        }
    }
    return -1;
}

struct game *create_game(struct game_config *config)
{
    int game_index = get_free_game();
    if (game_index == -1) {
        return NULL;
    }
    struct game *game = &game_list[game_index].game;

    game->threadlock = &game_list[game_index].game_mutex;

    strncpy(game->password, config->password, MAX_CREDENTIAL_LEN);
    strncpy(game->name, config->name, MAX_CREDENTIAL_LEN);
    game->max_player_count = config->max_player_count;
    game->min_player_count = config->min_player_count;

    atomic_store(&game->player_count, 0);
    atomic_fetch_add(&game_count, 1);
    return game;
}

void delete_game(struct game *game)
{
    pthread_mutex_t *lock = game->threadlock;
    pthread_mutex_lock(lock);
    // TODO:
    // Before we nuke the game,
    // we need to lock and tell all the clients
    // that the game is deleted and make
    // sure they shutdown
    atomic_store(&game->player_count, 0);
    memset(game, 0, sizeof(*game));
    pthread_mutex_unlock(lock);
    atomic_fetch_sub(&game_count, 1);
}

static void gen_player_start_pos(struct coordinates *out_coords)
{
    out_coords->x = 0.0f;
    out_coords->y = 0.0f;
    out_coords->z = 0.0f;
}
/*
 * This function assumes that the player was redirected to
 * character creation and creates a character at the next free
 * index in the game.
 * Caller handles concurrency.
 */
struct player *create_player(struct game *game,
                             struct player_credentials *credentials)
{
    struct player *new_player = &game->players[game->player_count];
    new_player->net_id =
        create_net_id(NET_TYPE_PLAYER, game, (void *)new_player);
    new_player->threadlock = get_mutex_from_net_id(game, new_player->net_id);
    new_player->game       = game;
    memcpy(&new_player->credentials, credentials, sizeof(*credentials));
    gen_player_start_pos(&new_player->coords);
    atomic_fetch_add(&game->player_count, 1);
    return new_player;
}

void delete_player(struct player *restrict player)
{
    pthread_mutex_t *lock = player->threadlock;
    pthread_mutex_lock(lock);
    clear_net_id(player->game, player->net_id);
    atomic_fetch_sub(&player->game->player_count, 1);
    player->game->player_count--;
    memset(player, 0, sizeof(*player));
    pthread_mutex_unlock(lock);
}

void set_player_char_sheet(struct player *player,
                           const struct character_sheet *charsheet)
{
    pthread_mutex_lock(player->threadlock);
    memcpy(&player->char_sheet, charsheet, sizeof(*charsheet));
    pthread_mutex_unlock(player->threadlock);
}

/*
 * Returns -1 on failure,
 * you should tell the client about malformed data.
 */
int init_charsheet_from_form(struct player *player,
                             const struct html_form *form)
{
    pthread_mutex_lock(player->threadlock);
    struct character_sheet *sheet = &player->char_sheet;
    if (form->field_count < FORM_CHARSHEET_FIELD_COUNT) {
        goto exit_error;
    }
    while (sheet->background < PLAYER_BACKGROUND_COUNT) {
        if (strncmp(player_background_strings[sheet->background],
                    form->fields[FORM_CHARSHEET_BACKGROUND],
                    HTMLFORM_FIELD_MAX_LEN) == 0) {
            break;
        }
        sheet->background++;
    }
    while (sheet->gender < GENDER_COUNT) {
        if (strncmp(player_gender_strings[sheet->gender],
                    form->fields[FORM_CHARSHEET_GENDER],
                    HTMLFORM_FIELD_MAX_LEN) == 0) {
            break;
        }
        sheet->gender++;
    }
    // TODO: This math only works as long as chargen stat limits
    // are EXACTLY 10. Just do string to int.
    sheet->vigour = (form->fields[FORM_CHARSHEET_VIGOUR][0] - ASCII_TO_INT) +
                    (9 * (form->fields[FORM_CHARSHEET_VIGOUR][1] != 0));
    sheet->violence =
        (form->fields[FORM_CHARSHEET_VIOLENCE][0] - ASCII_TO_INT) +
        (9 * (form->fields[FORM_CHARSHEET_VIOLENCE][1] != 0));
    sheet->cunning = (form->fields[FORM_CHARSHEET_CUNNING][0] - ASCII_TO_INT) +
                     (9 * (form->fields[FORM_CHARSHEET_CUNNING][1] != 0));

    if (validate_new_charsheet(sheet) != 0) {
        memset(sheet, 0, sizeof(*sheet));
        goto exit_error;
    }
    sheet->is_valid = true;
    pthread_mutex_unlock(player->threadlock);
    return 0;
exit_error:
    pthread_mutex_unlock(player->threadlock);
    return -1;
}

/*
 * This just checks the "is_valid" flag
 * in a thread safe manner,
 * see "validate_new_charsheet()"
 * for vibe-checking hackers
 */
bool is_charsheet_valid(const struct player *restrict player)
{
    bool result = 0;
    pthread_mutex_lock(player->threadlock);
    result = player->char_sheet.is_valid;
    pthread_mutex_unlock(player->threadlock);
    return result;
}

void set_game_password(struct game *restrict game,
                       const char password[static MAX_CREDENTIAL_LEN])
{
    pthread_mutex_lock(game->threadlock);
    memset(game->password, 0, MAX_CREDENTIAL_LEN);
    strncpy(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock(game->threadlock);
}

/*
 * Returns NULL when none is found
 * It's all readonly, so it _should_ be
 * thread safe in this specific case...
 */
struct player *try_get_player_from_token(session_token_t token,
                                         struct game *restrict game)
{
    if (token == INVALID_SESSION_TOKEN) {
        return NULL;
    }
    for (int i = 0; i < game->player_count; i++) {
        if (token == game->players[i].session_token) {
            return &game->players[i];
        }
    }
    return NULL;
}

/*
 * ==============================================================
 * ======= Message handling functions ===========================
 * ==============================================================
 */
enum response_opcodes {
    OPCODE_PING,
    OPCODE_PLAYER_MOVE,
    OPCODE_PLAYER_CONNECT
};
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
static int init_handler_response_buffer(void *response_buffer, opcode_t code)
{
    int header_size            = 0;
    ssize_t response_data_size = response_sizes[code];

    header_size = write_websocket_header(response_buffer,
                                         sizeof(code) + response_data_size);
    memcpy(&response_buffer[header_size], &code, sizeof(code));
    return header_size + sizeof(code);
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
    response_data->player_net_id = host_player->net_id;

    host_player->coords.x = response_data->coords.x_coord;
    host_player->coords.y = response_data->coords.y_coord;

    int packet_size = header_size + sizeof(*response_data);
    multicast_tcp(response_buffer, packet_size, 0);
}

/*
 * Somebody has sent the connection opcode,
 * but the current player taking a turn is *nobody*.
 * Returns 0 on success, -1 if we still
 * can't start the game because there aren't enough
 * players.
 * Caller handles game threadlock.
 */
static int try_start_game(struct game *game)
{
    if (atomic_load(&game->player_count) >= game->min_player_count) {
        // TODO: handle turn order more
        // gracefully than first come first serve.
        game->current_turn = game->players[0].net_id;
        return 0;
    }
    return -1;
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
    struct player_conn_res *response_data = NULL;
    // Currently unused
    // ---------------------
    // const struct
    // PlayerConnectReq  *playerData     = (struct PlayerConnectReq*)data;
    const opcode_t response_opcode = OPCODE_PLAYER_CONNECT;
    struct player *host_player     = get_player_from_host(remotehost);
    struct game *game              = host_player->game;
    if (game == NULL) {
        return;
    }
    const ssize_t namelen = sizeof(game->players[0].credentials.name);

    char response_buffer[MAX_RESPONSE_HEADER_SIZE +
                         sizeof(struct player_conn_res)] = {0};

    int header_size =
        init_handler_response_buffer(response_buffer, response_opcode);
    response_data = (struct player_conn_res *)&response_buffer[header_size];

    for (int i = 0; i < MAX_PLAYERS_IN_GAME; i++) {
        if (game->players[i].net_id == NULL_NET_ID) {
            continue;
        }
        pthread_mutex_lock(game->players[i].threadlock);
        net_id_t id               = game->players[i].net_id;
        response_data->players[i] = id;
        memcpy(&response_data->player_names[i],
               game->players[i].credentials.name,
               namelen);
        memcpy(&response_data->player_coords[i],
               &game->players[i].coords,
               sizeof(game->players[i].coords));
        pthread_mutex_unlock(game->players[i].threadlock);
        if (host_player->net_id == id) {
            response_data->player_index = (int8_t)i;
        }
    }
    // It's *nobody's* turn?
    // This means the game hasn't started yet.
    if (game->current_turn == NULL_NET_ID) {
        try_start_game(game);
    }
    response_data->current_turn = game->current_turn;

    int packet_size = header_size + sizeof(*response_data);
    multicast_tcp(response_buffer, packet_size, 0);
}

/* ===================================================================
 * =========== Player resources (inventory) management ===============
 * ===================================================================
 */

/*
 * Handlers for interacting with Player Resources.
 * Copy Paste these for different resources a player could
 * receive, lose, or use.
 */
void base_use_resource_handler(enum resource_id resource,
                               struct player *user,
                               struct player *target)
{
    // Implement code for using specific resources here
}
void base_give_resource_handler(enum resource_id resource,
                                struct player *target,
                                int count)
{
    // Implement code for receiving resources here, such
    // as adding it to the player's inventory or adding
    // a passive effect
}
void base_take_resource_handler(enum resource_id resource,
                                struct player *target,
                                int count)
{
    // Implement code for removing an item from a player's
    // inventory here, such as decreasing the count
    // or removing a passive effect.
}

/*
 *
 * Handlers for when a resource is used
 *      Make sure to list a handler FOR EACH existing resource ID
 *
 */
static use_resource_handler_t use_resource_handlers[RESOURCE_COUNT] = {
    base_use_resource_handler};
// Handlers for when a resource is given to a player
static give_resource_handler_t give_resource_handlers[RESOURCE_COUNT] = {
    base_give_resource_handler};
// Handlers for when a resource is taken from a player
static take_resource_handler_t take_resource_handlers[RESOURCE_COUNT] = {
    base_take_resource_handler};
