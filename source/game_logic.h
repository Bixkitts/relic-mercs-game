#ifndef BB_GAME_LOGIC
#define BB_GAME_LOGIC

#include <unistd.h>

#include "bbnetlib.h"

#define MAX_CREDENTIAL_LEN   64
#define MAX_RESOURCES_IN_INV 128

typedef struct Resource Resource;
typedef struct Player Player;
typedef struct Gamestate Gamestate;
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
    ENCOUNTER_TROLL,
    ENCOUNTER_BEGGAR,
    ENCOUNTER_COUNT
}EncounterID;

typedef enum {
    RESOURCE_KNIFE,
    RESOURCE_SWORD,
    RESOURCE_AXE,
    RESOURCE_POTION_HEAL,
    RESOURCE_COUNT
} ResourceID;

struct Gamestate {
    // Who's turn is it
    int   playerTurn;
    // Map weights for different
    // encounter types in which regions

};

struct Player {
    int        playerID;
    char       playerName [MAX_CREDENTIAL_LEN];
    char       password   [MAX_CREDENTIAL_LEN];
    Host       associatedHost;
    int        xCoord;
    int        yCoord;
    // How many of each ResourceID the player has
    int        resources  [RESOURCE_COUNT];
};

void handleGameMessage(char *data, ssize_t dataSize, Host remotehost);

#endif
