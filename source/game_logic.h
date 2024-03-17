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

typedef enum PlayerBackground {
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
} PlayerBackground;

typedef enum Factions{
    GAME_FACTION_SLAVERS,
    GAME_FACTION_CULTISTS,
    GAME_FACTION_ELDERS,
    GAME_FACTION_REBELS,
    GAME_FACTION_MERCHANTS,
    GAME_FACTION_BEETLES,
    GAME_FACTION_AFTERLIFE,
    GAME_FACTION_COUNT
} Factions;

typedef enum Gender {
    GENDER_MALE,
    GENDER_FEMALE,
    GENDER_COUNT
}Gender;

typedef struct CharacterSheet {
    bool             isValid; // Is this Charsheet valid at all?
    Gender           gender;
    int              vigour;
    int              violence;
    int              cunning;
    PlayerBackground background;
} CharacterSheet;

// All injuries are followed immediately by their 
// healed counterparts.
typedef enum {
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
}InjuryType;

// These ID's will be direct
// callbacks to handle these encounters
// serverside
typedef enum {
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
}EncounterID;

typedef enum {
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
}EncounterTypeID;

typedef enum {
    RESOURCE_KNIFE,
    RESOURCE_SWORD,
    RESOURCE_AXE,
    RESOURCE_POTION_HEAL,
    RESOURCE_COUNT
} ResourceID;

/*
 * Encounter Categories, and their possible encounters
 */
// NOTE: Every encounter category needs at least one specific
// encounter in it.
static int encounterCategories[ENCOUNTER_TYPE_COUNT][ENCOUNTER_COUNT]= {
    {ENCOUNTER_BANDITS},                 // ENCOUNTER_TYPE_BANDIT
    {ENCOUNTER_WOLVES,                   // ENCOUNTER_TYPE_BEASTS
     ENCOUNTER_RATS},
    {ENCOUNTER_TROLLS},                  // ENCOUNTER_TYPE_MONSTROSITIES
    {ENCOUNTER_DRAGONS},                 // ENCOUNTER_TYPE_DRAGON
    {ENCOUNTER_RUINS_OLD,                // ENCOUNTER_TYPE_MYSTICAL
     ENCOUNTER_TREASURE_MAGICAL,
     ENCOUNTER_SPELL_TOME},
    {ENCOUNTER_CULTISTS_CANNIBAL,        // ENCOUNTER_TYPE_CULTISTS
     ENCOUNTER_CULTISTS_PEACEFUL}
};

/*
 * State data structures
 */
struct Player {
    Host              associatedHost;
    PlayerCredentials credentials;
    SessionToken      sessionToken;
    CharacterSheet    charSheet;
    int               xCoord;
    int               yCoord;
    // How many of each ResourceID the player has
    int               resources[RESOURCE_COUNT];
    bool              isBanned;
};
struct Game {
    // Who's turn is it
    int     playerTurn;
    int     maxPlayerCount;
    // This is larger than max players to account
    // for kicked and banned players
    Player  players [MAX_PLAYERS_IN_GAME * 2];
    int     playerCount;
    char    password[MAX_CREDENTIAL_LEN];
};
/*
 * Primary entry point for interpreting
 * incoming websocket messages
 */
void handleGameMessage(char *data, ssize_t dataSize, Host remotehost);
typedef uint16_t Opcode;

/*
 * Authentication
 * ----------------------------------------------- */
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

long long int getTokenFromHTTP      (char *http,
                                     int httpLength);
Player *tryGetPlayerFromToken       (SessionToken token,
                                     Game *game);
// Character sheet setup stuff
void          validateNewCharsheet  (CharacterSheet *charsheet);
bool          isCharsheetValid      (const Player *player);
void          setPlayerCharSheet    (Player *player,
                                     CharacterSheet *charsheet);
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
#endif
