#ifndef BB_GAME_LOGIC
#define BB_GAME_LOGIC

#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>

#include "bbnetlib.h"
#include "helpers.h"
#include "session_token.h"

/*
 * Maximum amount of networked
 * objects/states
 */
#define MAX_NETOBJS \
    MAX_GAMES       \
    +MAX_PLAYERS

extern const char test_game_name[];

#define MAX_CREDENTIAL_LEN  32
#define MAX_PLAYERS_IN_GAME 8
#define MAX_GAMES           16
#define MAX_PLAYERS         MAX_PLAYERS_IN_GAME *MAX_GAMES
#define INVALID_PLAYER_ID   -1

typedef uint16_t opcode_t;

// All injuries are followed immediately by their
// healed counterparts.
enum injury_type {
    INJURY_NOTHING,
    INJURY_DEEP_CUT,
    INJURY_BIG_SCAR,
    INJURY_BROKEN_LEFT_LEG,
    INJURY_BROKEN_RIGHT_LEG,
    INJURY_BROKEN_RIGHT_ARM,
    INJURY_BROKEN_LEFT_ARM,
    INJURY_MISSING_LEFT_LEG,
    INJURY_MISSING_RIGHT_LEG,
    INJURY_MISSING_LEFT_ARM,
    INJURY_MISSING_RIGHT_ARM,
    INJURY_COUNT
};

enum factions {
    GAME_FACTION_SLAVERS,
    GAME_FACTION_CULTISTS,
    GAME_FACTION_ELDERS,
    GAME_FACTION_REBELS,
    GAME_FACTION_MERCHANTS,
    GAME_FACTION_BEETLES,
    GAME_FACTION_AFTERLIFE,
    GAME_FACTION_COUNT
};

// This is coupled with playerGenderStrings
enum gender {
    GENDER_MALE,
    GENDER_FEMALE,
    GENDER_COUNT
};
// These ID's will be direct
// callbacks to handle these encounters
// serverside
enum encounter_id {
    ENCOUNTER_NONE,
    ENCOUNTER_BANDITS,
    ENCOUNTER_TROLLS,
    ENCOUNTER_WOLVES,
    ENCOUNTER_RATS,
    ENCOUNTER_BEGGAR,
    ENCOUNTER_DRAGONS,
    ENCOUNTER_TREASURE_SMALL,
    ENCOUNTER_TREASURE_LARGE,
    ENCOUNTER_TREASURE_MAGICAL,
    ENCOUNTER_RUINS_OLD,
    ENCOUNTER_SPELL_TOME,
    ENCOUNTER_CULTISTS_CANNIBAL,
    ENCOUNTER_CULTISTS_PEACEFUL,
    ENCOUNTER_COUNT
};

enum encounter_type_id {
    ENCOUNTER_TYPE_NONE,
    ENCOUNTER_TYPE_ANY,
    ENCOUNTER_TYPE_BANDIT,
    ENCOUNTER_TYPE_BEASTS,
    ENCOUNTER_TYPE_MONSTROSITIES,
    ENCOUNTER_TYPE_DRAGON,
    ENCOUNTER_TYPE_MYSTICAL,
    ENCOUNTER_TYPE_CULTISTS,
    ENCOUNTER_TYPE_SLAVERS,
    ENCOUNTER_TYPE_REFUGEES,
    ENCOUNTER_TYPE_EXILES,
    ENCOUNTER_TYPE_PLAGUE,
    ENCOUNTER_TYPE_COUNT
};

enum resource_id {
    RESOURCE_KNIFE,
    RESOURCE_SWORD,
    RESOURCE_AXE,
    RESOURCE_POTION_HEAL,
    RESOURCE_COUNT
};

/*
 * Encounter Categories, and their possible encounters
 */
// NOTE: Every encounter category needs at least one specific
// encounter in it.
static int encounter_categories[ENCOUNTER_TYPE_COUNT][ENCOUNTER_COUNT] = {
    // ENCOUNTER_TYPE_BANDIT
    {ENCOUNTER_BANDITS},
    // ENCOUNTER_TYPE_BEASTS
    {ENCOUNTER_WOLVES, ENCOUNTER_RATS},
    // ENCOUNTER_TYPE_MONSTROSITIES
    {ENCOUNTER_TROLLS},
    // ENCOUNTER_TYPE_DRAGON
    {ENCOUNTER_DRAGONS},
    // ENCOUNTER_TYPE_MYSTICAL
    {ENCOUNTER_RUINS_OLD, ENCOUNTER_TREASURE_MAGICAL, ENCOUNTER_SPELL_TOME},
    // ENOUNTER_TYPE_CULTISTS
    {ENCOUNTER_CULTISTS_CANNIBAL, ENCOUNTER_CULTISTS_PEACEFUL}};
// This is coupled with playerBackgroundStrings
enum player_background {
    PLAYER_BACKGROUND_TRADER,
    PLAYER_BACKGROUND_FARMER,
    PLAYER_BACKGROUND_WARRIOR,
    PLAYER_BACKGROUND_PRIEST,
    PLAYER_BACKGROUND_CULTIST,
    PLAYER_BACKGROUND_DIPLOMAT,
    PLAYER_BACKGROUND_SLAVER,
    PLAYER_BACKGROUND_MONSTERHUNTER,
    PLAYER_BACKGROUND_CLOWN,
    PLAYER_BACKGROUND_COUNT
};

typedef unsigned int player_attr_t;
struct character_sheet {
    bool is_valid; // Is this Charsheet valid at all?
    enum gender gender;
    player_attr_t vigour;
    player_attr_t violence;
    player_attr_t cunning;
    enum player_background background;
};

struct coordinates {
    double x;
    double y;
    double z;
};

struct player_credentials {
    char name[MAX_CREDENTIAL_LEN];
    char password[MAX_CREDENTIAL_LEN];
};

struct game_config {
    char name[MAX_CREDENTIAL_LEN];
    char password[MAX_CREDENTIAL_LEN];
    int max_player_count;
    int min_player_count;
};

enum game_state {
    GAME_STATE_NOT_EXISTING,
    GAME_STATE_NOT_STARTED,
    GAME_STATE_STARTED,
    GAME_STATE_COUNT 
};

/*
 * Form Interpreting
 */
enum charsheet_form_fields {
    FORM_CHARSHEET_BACKGROUND,
    FORM_CHARSHEET_GENDER,
    FORM_CHARSHEET_VIGOUR,
    FORM_CHARSHEET_VIOLENCE,
    FORM_CHARSHEET_CUNNING,
    FORM_CHARSHEET_FIELD_COUNT
};
/*
 * Networked Data structures.
 * Make sure to lock these properly with
 * respect to concurrent access from
 * multiple clients.
 * They each have their own threadlock mutex
 * set on creation.
 * TODO: Come up with a good system for concurrent access.
 */
typedef int16_t player_id_t;

struct player {
    player_id_t id;
    pthread_mutex_t threadlock;
    struct host *associated_host;
    struct game *game;
    struct player_credentials credentials;
    session_token_t session_token;
    struct character_sheet char_sheet;
    struct coordinates coords;
    // How many of each ResourceID the player has
    int resources[RESOURCE_COUNT];
    // Which encounter is the player currently
    // encountering, if any.
    enum encounter_id current_enc;
};
/*
 * Lock the threadlock before modifying
 * game data.
 * The struct player has it's own threadlock
 * you should lock before you modify any of their
 * data.
 */
struct game {
    pthread_mutex_t threadlock;
    char name[MAX_CREDENTIAL_LEN];
    char password[MAX_CREDENTIAL_LEN];
    // Which player is currently
    // taking their turn.
    // When this is 0, the game hasn't started yet
    // and the first turn needs to be assigned.
    struct player *current_turn;
    enum game_state state;
    int max_player_count;
    int min_player_count;
    struct player players[MAX_PLAYERS_IN_GAME];
    atomic_int player_count;
};


void           set_game_password         (struct game *restrict game,
                                          const char password[static MAX_CREDENTIAL_LEN]);
struct player *try_get_player_from_token (session_token_t token,
                                          struct game *restrict game);
struct game   *get_game_from_name        (const char name[static MAX_CREDENTIAL_LEN]);
// Character sheet setup stuff
int            init_charsheet_from_form  (struct player *player,
                                          const struct html_form *form);
bool           is_charsheet_valid        (const struct player *restrict player);
// Note: use initCharsheetFromForm.
void           set_player_char_sheet     (struct player *player,
                                          const struct character_sheet *charsheet);
/* --------------------------------------------- */

struct game *create_game      (struct game_config *config);
void         try_start_game   (struct game *game);
int          get_player_count (const struct game *game);

struct player *create_player (struct game *game,
                              const struct player_credentials *credentials);
void           delete_player (struct player *restrict player);
#endif
