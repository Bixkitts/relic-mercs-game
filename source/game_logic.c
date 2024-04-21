#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#include "bbnetlib.h"
#include "host_custom_attributes.h"
#include "error_handling.h"
#include "helpers.h"
#include "websockets.h"
#include "html_server.h"
#include "game_logic.h"

#define MESSAGE_HANDLER_COUNT 6

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
// This is coupled with enum PlayerBackground
static const char playerBackgroundStrings[PLAYER_BACKGROUND_COUNT][HTMLFORM_FIELD_MAX_LEN] = {
    "Trader",
    "Farmer",
    "Warrior",
    "Priest",
    "Cultist",
    "Diplomat",
    "Slaver",
    "Monster+Hunter",
    "Clown"
};



/*
 * These encode the ranges of NetIDs that correspond
 * to those objects
 */
enum NetObjType {
    NET_TYPE_NULL,
    NET_TYPE_PLAYER,
    NET_TYPE_GAME,
    NET_TYPE_COUNT
};
/*
 * NetID ranges corresponding to object types
 */
static int netIDRanges[NET_TYPE_COUNT] = {0, 32, 34};
/*
 * Maybe replace this with a resizing version if
 * we ever need more networked objects than this?
 * Remember to lock netIDs before touching it.
 *
 * NOTE: this needs a logical to physical type mapping
 * if we start working with thousands and thousands of
 * dynamic objects. That won't happen to me though :D
 */
#define NETIDS_MAX 2048
// Lock this before touching netIDs
static pthread_mutex_t   netIDmutex         = PTHREAD_MUTEX_INITIALIZER;
static void             *netIDs[NETIDS_MAX] = { 0 };

typedef long long NetID;



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
}Gender;
// This is coupled with enum Gender
static const char playerGenderStrings[GENDER_COUNT][HTMLFORM_FIELD_MAX_LEN] = {
    "Male",
    "Female"
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

struct Player {
    NetID                    netID;
    pthread_mutex_t          threadlock;
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
    pthread_mutex_t    threadlock;
    int                playerTurn;
    int                maxPlayerCount;
    // This is larger than max players to account
    // for kicked and banned players
    struct Player      players [MAX_PLAYERS_IN_GAME * 2];
    int                playerCount;
    char               password[MAX_CREDENTIAL_LEN];
};

union GameObject {
    struct Player player;
    struct Game   game;
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
 * Handler type definitions
 */
typedef void (*GameMessageHandler)  (char* data, ssize_t dataSize, Host remotehost);
typedef void (*UseResourceHandler)  (enum ResourceID resource, struct Player *user, struct Player *target);
typedef void (*GiveResourceHandler) (enum ResourceID resource, struct Player *target, int count);
typedef void (*TakeResourceHandler) (enum ResourceID resource, struct Player *target, int count);

/*
 * Not thread safe, the test game should be
 * written to once on init
 */
static struct Game testGame = { 0 };
struct Game *getTestGame()
{
    return &testGame;
}

/* 
 * Helpers and Authentication
 * ----------------------------
 *  None of these have mutex locks in them,
 *  the functions that call them are expected
 *  to lock the game state they are modifying.
 */
static void           
generateSessionToken        (struct Player *player,
                             struct Game *game);
static int            
isGameMessageValidLength    (Opcode opcode, 
                             ssize_t messageSize);
static void           
buildSessionTokenHeader     (char outHeader[static HEADER_LENGTH], 
                             SessionToken token);
static struct Player 
*tryGetPlayerFromPlayername (struct Game *game, 
                             const char *credentials);
static int            
isPlayerPasswordValid       (const struct Player *restrict player,
                             const char *password);
static struct Player 
*createPlayer               (struct Game *game,
                             struct PlayerCredentials *credentials);
static int            
validateNewCharsheet        (struct CharacterSheet *sheet);
static void         
*resolveNetIDToObj          (const NetID netID,
                             enum NetObjType type);
static NetID
createNetID                 (enum NetObjType, 
                             void *object);
static void
clearNetID                  (const NetID netID);

/*
 * Handlers for incoming messages from the websocket connection
 */
static void pingHandler                 (char *data, ssize_t dataSize, Host remotehost);
struct MoveObjOnMapData {
    NetID     netID; 
    double    xCoord;
    double    yCoord;
};
static void moveObjOnMapHandler         (char *data, ssize_t dataSize, Host remotehost);
static void endTurnHandler              (char *data, ssize_t dataSize, Host remotehost);
// Player chose a response to an encounter
static void respondToEventHandler       (char *data, ssize_t dataSize, Host remotehost);
// Client calls this on connect,
// this should either:
// 1. Set an atomic lock on the entire game state while the client connects (easy)
// 2. Allow the game to continue during the connection and apply
//    any state changes that happened since (harder)
static void getGameStateHandler         (char *data, ssize_t dataSize, Host remotehost);


/*
 * Primary interpreter for incoming websocket messages
 * carrying valid gameplay opcodes.
 */
static GameMessageHandler gameMessageHandlers[MESSAGE_HANDLER_COUNT] = {
    pingHandler,
    moveObjOnMapHandler, 
    endTurnHandler,           
    respondToEventHandler, 
    getGameStateHandler         
};
static int gameDataSizes[MESSAGE_HANDLER_COUNT] = {
    0,
    sizeof(struct MoveObjOnMapData),
    0,
    0,
    0,
    0
};

/*
 * There's an opcode, and then
 * serialised state data.
 * This state data should match a
 * corresponding data structure perfectly,
 * or be rejected.
 */
static int isGameMessageValidLength(Opcode opcode, ssize_t messageSize)
{
    return messageSize == gameDataSizes[opcode];
}

/*
 * ========================================================
 * ======== MAIN ENTRY POINT FOR WEBSOCKET MESSAGES =======
 * ========================================================
 * This function expects
 * Decoded websocket data and it's length
 * without the websocket headers.
 */
void handleGameMessage(char *data, ssize_t dataSize, Host remotehost)
{
    Opcode *opcode = (Opcode*)data;
    if (*opcode >= MESSAGE_HANDLER_COUNT) {
#ifdef DEBUG
        fprintf(stderr, "\nBad websocket Opcode.\n");
#endif
        return;
    }
#ifdef DEBUG
    printBufferInHex(data, dataSize);
#endif 

    if(isGameMessageValidLength(*opcode, dataSize - sizeof(Opcode))) {
        // Execute the opcode, this function
        // assumes it won't segfault.
        gameMessageHandlers[*opcode](&data[sizeof(Opcode)], dataSize, remotehost);
    };
}

int createGame(struct Game **game, struct GameConfig *config)
{
    *game = calloc(1, sizeof(**game));
    if (!(*game)) {
        return -1;
    }
    strncpy((*game)->password, config->password, MAX_CREDENTIAL_LEN);
    (*game)->maxPlayerCount = config->maxPlayerCount;

    pthread_mutexattr_t threadlock_att;
    pthread_mutexattr_init (&threadlock_att);
    pthread_mutex_init     (&(*game)->threadlock, &threadlock_att);

    return 0;
}
int initializeTestGame(struct GameConfig *config)
{
    struct Game *game = getTestGame();
    strncpy(game->password, config->password, MAX_CREDENTIAL_LEN);
    game->maxPlayerCount = config->maxPlayerCount;
    return 0;
}

/*
 * This function assumes that the player was redirected to
 * character creation and creates a character at the next free 
 * index in the game
 */
static struct Player *createPlayer(struct Game *game, struct PlayerCredentials *credentials)
{
    struct Player *newPlayer = &game->players[game->playerCount];

    // Threadlock initialisation...
    pthread_mutexattr_t threadlock_att;
    pthread_mutexattr_init (&threadlock_att);
    pthread_mutex_init     (&newPlayer->threadlock, &threadlock_att);

    // Credentials...
    memcpy (&newPlayer->credentials, credentials, sizeof(*credentials));

    // Currently, this function is only called
    // from within a critical section within
    // tryPlayerLogin(), so incrementing this
    // is okay.
    game->playerCount++;

    return newPlayer;
}

static void clearPlayer(struct Player *restrict player)
{
    memset(player, 0, sizeof(*player));
}

void  setPlayerCharSheet (struct Player *player,
                          struct CharacterSheet *charsheet)
{
    pthread_mutex_lock(&player->threadlock);                           
    memcpy (&player->charSheet, charsheet, sizeof(*charsheet)); 
    pthread_mutex_unlock (&player->threadlock);
}

/*
 * We've just gotten a character sheet
 * parsed out of a html form,
 * but we need to make sure it's
 * not fudged somehow by the client.
 */
static int validateNewCharsheet (struct CharacterSheet *sheet)
{
    // All this math is unsigned for a reason
    PlayerAttr playerPower = sheet->vigour
                             + sheet->cunning
                             + sheet->violence;
    if (playerPower != 13) {
        return -1;
    }
    if (sheet->background >= PLAYER_BACKGROUND_COUNT
        || sheet->background < 0) {
        return -1;
    }
    if (sheet->gender >= GENDER_COUNT
        || sheet->gender < 0) {
        return -1;
    }
    return 0;
}

/* 
 * Returns -1 on failure,
 * you should tell the client about malformed data.
 */
int initCharsheetFromForm(struct Player *player, 
                          const struct HTMLForm *form)
{
    pthread_mutex_lock(&player->threadlock);                           
    struct CharacterSheet *sheet = &player->charSheet;
    if (form->fieldCount < FORM_CHARSHEET_FIELD_COUNT) {
        pthread_mutex_unlock(&player->threadlock);
        return -1;
    }
    while(sheet->background < PLAYER_BACKGROUND_COUNT) {
        if (strncmp(playerBackgroundStrings[sheet->background], 
                    form->fields[FORM_CHARSHEET_BACKGROUND], 
                    HTMLFORM_FIELD_MAX_LEN) == 0) {
            break;
        }
        sheet->background++;
    }
    while(sheet->gender < GENDER_COUNT) {
        if (strncmp(playerGenderStrings[sheet->gender], 
                    form->fields[FORM_CHARSHEET_GENDER], 
                    HTMLFORM_FIELD_MAX_LEN) == 0) {
            break;
        }
        sheet->gender++;
    }
    // TODO: This math only works as long as chargen stat limits
    // are EXACTLY 10. Just do string to int.
    sheet->vigour   = (form->fields[FORM_CHARSHEET_VIGOUR]  [0] 
                      - ASCII_TO_INT) + (9 * (form->fields[FORM_CHARSHEET_VIGOUR] [1] != 0));
    sheet->violence = (form->fields[FORM_CHARSHEET_VIOLENCE][0] 
                      - ASCII_TO_INT) + (9 * (form->fields[FORM_CHARSHEET_VIOLENCE] [1] != 0));
    sheet->cunning  = (form->fields[FORM_CHARSHEET_CUNNING] [0] 
                      - ASCII_TO_INT) + (9 * (form->fields[FORM_CHARSHEET_CUNNING] [1] != 0));

    if (validateNewCharsheet(sheet) != 0) {
        memset(sheet, 0, sizeof(*sheet));
        pthread_mutex_unlock(&player->threadlock);
        return -1;
    }
    sheet->isValid = true;
    pthread_mutex_unlock(&player->threadlock);
    return 0;
}

/*
 * This just checks the "isValid" flag
 * in a thread safe manner,
 * see "validateNewCharsheet()"
 * for vibe-checking hackers
 */
bool isCharsheetValid (struct Player *restrict player)
{
    bool result = 0;
    pthread_mutex_lock(&player->threadlock);
    result = player->charSheet.isValid;
    pthread_mutex_unlock(&player->threadlock);
    return result;
}

void setGamePassword(struct Game *restrict game, const char password[static MAX_CREDENTIAL_LEN])
{
    pthread_mutex_lock   (&game->threadlock);
    memset               (game->password, 
                          0, 
                          MAX_CREDENTIAL_LEN);
    strncpy              (game->password, 
                          password, 
                          MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&game->threadlock);
}

/*
 * Returns 0 on success and -1 on failure
 */
int tryGameLogin(struct Game *restrict game, const char *password)
{
    int match = 0;
    pthread_mutex_lock   (&game->threadlock);
    match = strncmp(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock (&game->threadlock);
    match = -1 * (match != 0);
    return match;
}

/*
 * Parses an int64 token out of a HTTP
 * message and returns it, or 0 on failure.
 */
long long getTokenFromHTTP(char *http,
                               int httpLength)
{
    const char    cookieName[HEADER_LENGTH] = "sessionToken=";
    int           startIndex                = stringSearch(http, cookieName, httpLength);
    long long     token                     = 0;

    if (startIndex >= 0) {
        startIndex += strnlen(cookieName, HEADER_LENGTH);
        // TODO: This might overflow with specifically 
        // malformed packets
        token       = strtoll(&http[startIndex], NULL, 10);
    }
    return token;
}

/*
 * Returns NULL when none is found
 * It's all readonly, so it _should_ be
 * thread safe in this specific case...
 */
struct Player *tryGetPlayerFromToken(SessionToken token,
                                     struct Game *restrict game)
{
    for (int i = 0; i < game->playerCount; i ++) {
        if (token == game->players[i].sessionToken) {
            return &game->players[i];
        }
    }
    return NULL;
}

/*
 * We pass in the game because the
 * session token needs to be unique
 * on a per game basis
 */
static void generateSessionToken(struct Player *restrict player, 
                                 struct Game *restrict game)
{
    long long int nonce = getRandomInt();
    int           i     = 0;
    // Make sure the token is unique, and not 0.
    while ((tryGetPlayerFromToken(nonce, game) != NULL) || (nonce == 0)) {
        nonce = getRandomInt();
        i++;
        if (i > 2) {
            fprintf(stderr, "How the fuck? Kill it with fire.\n");
            exit(1);
        }
    }
    player->sessionToken = nonce;
}

/* 
 * Builds the custom Cookie header that
 * sends the session token to the client.
 */
static void buildSessionTokenHeader(char outHeader[static HEADER_LENGTH], 
                                    SessionToken token)
{
    char headerBase [HEADER_LENGTH] = "Set-Cookie: sessionToken=";
    char tokenString[HEADER_LENGTH] = {0};

    sprintf(tokenString, "%lld\n", token);
    strncat(headerBase, tokenString, HEADER_LENGTH - strlen(headerBase));
    memcpy (outHeader, headerBase, HEADER_LENGTH);
}

/*
 * returns the index of the player in the game
 */
static struct Player *tryGetPlayerFromPlayername(struct Game *game, 
                                                 const char *playername)
{
    int playerIndex   = 0;
    int playerFound   = -1;

    for (playerIndex = 0; playerIndex < game->playerCount; playerIndex++) {
        playerFound = strncmp(playername, 
                              game->players[playerIndex].credentials.name, 
                              MAX_CREDENTIAL_LEN); 
        if (playerFound == 0) {
            return &game->players[playerIndex];
        }
    }
    return NULL;
}

static int isPlayerPasswordValid(const struct Player *restrict player,
                                 const char *password)
{
    int passwordCheck = 0;
    passwordCheck = strncmp(password, 
                            player->credentials.password, 
                            MAX_CREDENTIAL_LEN); 
    return passwordCheck == 0;
}
/*
 * This function assumes the player had a valid
 * game password, so it'll make them a new
 * character if their credentials don't fit.
 *
 * returns 0 if the client successfully logged in
 * and took control of some player, new or pre-existing
 *
 * returns -1 when the player exists but the
 * password was wrong
 */
int   tryPlayerLogin    (struct Game *restrict game,
                         struct PlayerCredentials *restrict credentials,
                         Host remotehost)
{
    struct Player        *player        = NULL;
    char                  sessionTokenHeader[CUSTOM_HEADERS_MAX_LEN] = {0};

    pthread_mutex_lock(&game->threadlock);
    player = tryGetPlayerFromPlayername(game, credentials->name);
    if ( player != NULL ) {
        if (isPlayerPasswordValid(player, credentials->password)) {
            generateSessionToken   (player, 
                                    game);
            buildSessionTokenHeader(sessionTokenHeader, 
                                    player->sessionToken);
            sendContent            ("./game.html", 
                                    HTTP_FLAG_TEXT_HTML, 
                                    remotehost,
                                    sessionTokenHeader);
            pthread_mutex_unlock   (&game->threadlock);
            return 0;
        }
        else {
            // Player exists, but the password was wrong
            pthread_mutex_unlock   (&game->threadlock);
            return -1;
        }
    }
    // Player was not found in game redirect them to character creation
    // And create a player
    player = 
    createPlayer            (game, 
                             credentials);
    generateSessionToken    (player, 
                             game);
    buildSessionTokenHeader (sessionTokenHeader, 
                             player->sessionToken);
    sendContent             ("./charsheet.html", 
                             HTTP_FLAG_TEXT_HTML, 
                             remotehost, 
                             sessionTokenHeader);

    pthread_mutex_unlock(&game->threadlock);
    return 0;
}

/*
 * ==============================================================
 * ======= Message handling functions ===========================
 * ==============================================================
 */
static void pingHandler(char *data, ssize_t dataSize, Host remotehost)
{
    printf("Ping incoming!");
    Opcode responseOpcode = 0x00;
    char   responseBuffer[sizeof(responseOpcode)
                          + WEBSOCKET_HEADER_SIZE_MAX] = { 0 };
    int packetSize =
    encodeWebsocketMessage (responseBuffer,
                            (char*)&responseOpcode,
                            sizeof(responseOpcode));
    sendDataTCP            (responseBuffer,
                            packetSize,
                            remotehost);
}

/*
 * Returns an object of the corresponding NetObjType,
 * or NULL if: 
 * - The netID is invalid
 * - A valid netID resolves to an object of the wrong type
 */
static void *resolveNetIDToObj(const NetID netID, enum NetObjType type)
{
    if (netID >= NETIDS_MAX) {
        return NULL;
    }
    pthread_mutex_lock(&netIDmutex);
    const void *retObj     = netIDs[netID];
    if (retObj == NULL) {
        pthread_mutex_unlock(&netIDmutex);
        return NULL;
    }
    bool  isCorrectType = ((netID < netIDRanges[type]) && (netID >= netIDRanges[type-1]));
    void *ret           = (void*)(((unsigned long long)retObj) * isCorrectType);
    pthread_mutex_unlock(&netIDmutex);
    return ret;    
}

/*
 * Returns the NetID it finds free,
 * or -1 on failure
 */
static NetID createNetID (enum NetObjType type, 
                          void *object)
{
    NetID i = netIDRanges[type-1];
    while(netIDs[i] != NULL) {
        i++;
        if (i >= netIDRanges[type]) {
            return -1;
        }
    }
    return i;
}
/*
 * Expects a NetID smaller than NETID_MAX
 *
 * Currently just sets the pointer at
 * that NetID to NULL.
 * Do this to prevent the object from being
 * resolved from a NetID e.g. if it's being deleted.
 */
static void clearNetID (const NetID netID)
{

}

static void moveObjOnMapHandler(char *data, ssize_t dataSize, Host remotehost)
{
    struct MoveObjOnMapData *moveData = (struct MoveObjOnMapData*)data; 

    // Here we respond to the clients,
    // telling them all who moved and where.
    Opcode responseOpcode = 0x01;
    char   gameResponseData[(sizeof(*moveData) 
                            + sizeof(responseOpcode))] = { 0 };
    memcpy (gameResponseData, 
            &responseOpcode, 
            sizeof(responseOpcode));
    memcpy (&(gameResponseData[sizeof(responseOpcode)]), 
            moveData, 
            sizeof(*moveData));


    char multicastBuffer   [sizeof(*moveData) 
                            + sizeof(responseOpcode)
                            + WEBSOCKET_HEADER_SIZE_MAX] = { 0 };
    int packetSize =
    encodeWebsocketMessage (multicastBuffer, 
                            gameResponseData, 
                            sizeof(*moveData) 
                            + sizeof(responseOpcode));
    multicastTCP           (multicastBuffer, 
                            packetSize, 
                            0);
}
static void endTurnHandler(char *data, ssize_t dataSize, Host remotehost)
{

}
// Player chose a response to an encounter
static void respondToEventHandler(char *data, ssize_t dataSize, Host remotehost)
{

}
// Player manually used one of their resources
static void useResourceHandler(char *data, ssize_t dataSize, Host remotehost)
{

}

// Client calls this on connect,
// this should either:
// 1. Set an atomic lock on the entire game state while the client connects (easy)
// 2. Allow the game to continue during the connection and apply
//    any state changes that happened since (harder)
static void getGameStateHandler(char *data, ssize_t dataSize, Host remotehost)
{

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
void baseUseResourceHandler (enum ResourceID resource, 
                             struct Player *user, 
                             struct Player *target)
{
    // Implement code for using specific resources here
}
void baseGiveResourceHandler (enum ResourceID resource, 
                              struct Player *target, 
                              int count)
{
    // Implement code for receiving resources here, such
    // as adding it to the player's inventory or adding 
    // a passive effect
}
void baseTakeResourceHandler (enum ResourceID resource, 
                              struct Player *target, 
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
static UseResourceHandler useResourceHandlers[RESOURCE_COUNT] = {
    baseUseResourceHandler
};
// Handlers for when a resource is given to a player
static GiveResourceHandler giveResourceHandlers[RESOURCE_COUNT] = {
    baseGiveResourceHandler
};
// Handlers for when a resource is taken from a player
static TakeResourceHandler takeResourceHandlers[RESOURCE_COUNT] = {
    baseTakeResourceHandler
};
