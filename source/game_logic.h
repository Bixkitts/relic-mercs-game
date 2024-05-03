#ifndef BB_GAME_LOGIC
#define BB_GAME_LOGIC

#include <unistd.h>

#include "bbnetlib.h"
#include "helpers.h"
#include "session_token.h"
#include "net_ids.h"

/*
 * Maximum amount of networked
 * objects/states
 */
#define MAX_NETOBJS          MAX_GAMES       \
                             + MAX_PLAYERS

extern const char testGameName[];

#define MAX_CREDENTIAL_LEN   32
#define MAX_PLAYERS_IN_GAME  16
#define MAX_GAMES            16
#define MAX_PLAYERS          MAX_PLAYERS_IN_GAME * MAX_GAMES


typedef uint16_t Opcode;

// All injuries are followed immediately by their 
// healed counterparts.
enum InjuryType{
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

enum Factions{
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
enum Gender {
    GENDER_MALE,
    GENDER_FEMALE,
    GENDER_COUNT
};
// These ID's will be direct
// callbacks to handle these encounters
// serverside
enum EncounterID{
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

enum EncounterTypeID{
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

enum ResourceID{
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
// This is coupled with playerBackgroundStrings
enum PlayerBackground {
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


typedef unsigned int PlayerAttr;
struct CharacterSheet {
    bool                  isValid; // Is this Charsheet valid at all?
    enum Gender           gender;
    PlayerAttr            vigour;
    PlayerAttr            violence;
    PlayerAttr            cunning;
    enum PlayerBackground background;
};

struct Coordinates {
    int x;
    int y;
    int z;
};

struct PlayerCredentials {
    char name     [MAX_CREDENTIAL_LEN];
    char password [MAX_CREDENTIAL_LEN];
};


struct GameConfig {
    char name    [MAX_CREDENTIAL_LEN];
    char password[MAX_CREDENTIAL_LEN];
    int  maxPlayerCount;
};

/*
 * Form Interpreting
 */
enum CharsheetFormFields {
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
 */
struct Player {
    NetID                    netID;
    pthread_mutex_t         *threadlock;
    Host                     associatedHost;
    struct PlayerCredentials credentials;
    SessionToken             sessionToken;
    struct CharacterSheet    charSheet;
    struct Coordinates       coords;
    // How many of each ResourceID the player has
    int                      resources[RESOURCE_COUNT];
};
struct Game {
    NetID              netID;
    pthread_mutex_t   *threadlock;
    char               name    [MAX_CREDENTIAL_LEN];
    char               password[MAX_CREDENTIAL_LEN];
    int                playerTurn;
    int                maxPlayerCount;
    struct Player      players [MAX_PLAYERS_IN_GAME];
    int                playerCount;
};

/*
 * Data structures coupled with websocket
 * handlers
 */
struct MovePlayerData {
    double    xCoord;
    double    yCoord;
};
struct FetchPlayerDataData {
    int16_t playerCount;
    NetID   players[MAX_PLAYERS_IN_GAME];
};

/*
 * Primary entry point for interpreting
 * incoming websocket messages
 */
void handleGameMessage(char *data, 
                       ssize_t dataSize, 
                       Host remotehost);


void         
setGamePassword         (struct Game *restrict game, 
                         const char password[static MAX_CREDENTIAL_LEN]);
struct Player 
*tryGetPlayerFromToken  (SessionToken token,
                         struct Game *restrict game);
struct Game 
*getGameFromName        (const char name[static MAX_CREDENTIAL_LEN]);
// Character sheet setup stuff
int  
initCharsheetFromForm   (struct Player *charsheet, 
                         const struct HTMLForm *form);
bool 
isCharsheetValid        (const struct Player *restrict player);
// Note: use initCharsheetFromForm.
void 
setPlayerCharSheet      (struct Player *player,
                         struct CharacterSheet *charsheet);
/* --------------------------------------------- */

struct Game   *createGame    (struct GameConfig *config);
struct Player *createPlayer  (struct Game *game,
                              struct PlayerCredentials *credentials);
void           deletePlayer  (struct Player *restrict player);
// returns the ID the player got
// Returns the pointer to global Game
// variable.
// Not thread safe, set this exactly once
// in the main thread.
int   getPlayerCount     (const struct Game *game);
#endif
