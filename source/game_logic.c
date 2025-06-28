#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "auth.h"
#include "game_logic.h"
#include "helpers.h"
#include "validators.h"


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
static void gen_player_start_pos(struct coordinates *out_coords);

static inline int get_free_game(void)
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

    pthread_mutex_init(&game->threadlock, NULL);
    strncpy(game->password, config->password, MAX_CREDENTIAL_LEN);
    strncpy(game->name, config->name, MAX_CREDENTIAL_LEN);
    game->max_player_count = config->max_player_count;
    game->min_player_count = config->min_player_count;
    game->state = GAME_STATE_NOT_STARTED;
    atomic_store(&game->player_count, 0);
    atomic_fetch_add(&game_count, 1);
    return game;
}

void delete_game(struct game *game)
{
    pthread_mutex_lock(&game->threadlock);
    // TODO:
    // Before we nuke the game,
    // we need to lock and tell all the clients
    // that the game is deleted and make
    // sure they shutdown
    atomic_store(&game->player_count, 0);
    memset(game, 0, sizeof(*game));
    pthread_mutex_unlock(&game->threadlock);
    atomic_fetch_sub(&game_count, 1);
}

static void gen_player_start_pos(struct coordinates *out_coords)
{
    // TODO: generate an actual start position
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
                             const struct player_credentials *credentials)
{
    const player_id_t new_player_id = atomic_fetch_add(&game->player_count, 1);

    /* TODO: make this find a free slot instead */
    struct player *new_player = &game->players[new_player_id];

    new_player->id = new_player_id;
    pthread_mutex_init(&new_player->threadlock, NULL);
    new_player->game = game;
    memcpy(&new_player->credentials, credentials, sizeof(*credentials));
    gen_player_start_pos(&new_player->coords);
    return new_player;
}

void delete_player(struct player *restrict player)
{
    // TODO: if we memset the entire player struct this probably
    //       messes up the player threadlock
    pthread_mutex_lock(&player->threadlock);
    atomic_fetch_sub(&player->game->player_count, 1);
    memset(player, 0, sizeof(*player));
    pthread_mutex_unlock(&player->threadlock);
}

void set_player_char_sheet(struct player *player,
                           const struct character_sheet *charsheet)
{
    pthread_mutex_lock(&player->threadlock);
    memcpy(&player->char_sheet, charsheet, sizeof(*charsheet));
    pthread_mutex_unlock(&player->threadlock);
}

/*
 * Returns -1 on failure,
 * you should tell the client about malformed data.
 */
int init_charsheet_from_form(struct player *player,
                             const struct html_form *form)
{
    pthread_mutex_lock(&player->threadlock);
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
    sheet->vigour   = (form->fields[FORM_CHARSHEET_VIGOUR][0] - ASCII_TO_INT)
                      + (9 * (form->fields[FORM_CHARSHEET_VIGOUR][1] != 0));
    sheet->violence = (form->fields[FORM_CHARSHEET_VIOLENCE][0] - ASCII_TO_INT)
                      + (9 * (form->fields[FORM_CHARSHEET_VIOLENCE][1] != 0));
    sheet->cunning  = (form->fields[FORM_CHARSHEET_CUNNING][0] - ASCII_TO_INT)
                      + (9 * (form->fields[FORM_CHARSHEET_CUNNING][1] != 0));

    if (validate_new_charsheet(sheet) != 0) {
        memset(sheet, 0, sizeof(*sheet));
        goto exit_error;
    }
    sheet->is_valid = true;
    pthread_mutex_unlock(&player->threadlock);
    return 0;
exit_error:
    pthread_mutex_unlock(&player->threadlock);
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
    result = player->char_sheet.is_valid;
    return result;
}

void set_game_password(struct game *restrict game,
                       const char password[static MAX_CREDENTIAL_LEN])
{
    pthread_mutex_lock(&game->threadlock);
    memset(game->password, 0, MAX_CREDENTIAL_LEN);
    strncpy(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock(&game->threadlock);
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

static bool is_player_turn(const struct player *player)
{
    return player->game->current_turn == player;
}


/*
 * Somebody has sent the connection opcode,
 * but the current player taking a turn is *nobody*.
 * Returns 0 on success, -1 if we still
 * can't start the game because there aren't enough
 * players.
 * Caller handles game threadlock.
 */
void try_start_game(struct game *game)
{
    if (atomic_load(&game->player_count) >= game->min_player_count) {
        // TODO: handle turn order more
        // gracefully than first come first serve.
        game->current_turn = &game->players[0];
        game->state = GAME_STATE_STARTED;
    }
}
