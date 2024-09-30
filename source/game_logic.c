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

#define MAX_RESPONSE_HEADER_SIZE WEBSOCKET_HEADER_SIZE_MAX + sizeof(Opcode)

// This is coupled with enum PlayerBackground
// and also coupled on the clientside
static const char playerBackgroundStrings[PLAYER_BACKGROUND_COUNT]
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
static const char playerGenderStrings[GENDER_COUNT][HTMLFORM_FIELD_MAX_LEN] =
    {"Male", "Female"};

pthread_mutex_t netObjMutexes[MAX_NETOBJS] = {0};

const char testGameName[MAX_CREDENTIAL_LEN] = "test game";

/*
 * Handler type definitions
 */
typedef void (*GameMessageHandler)(char *data,
                                   ssize_t dataSize,
                                   Host remotehost);
typedef void (*UseResourceHandler)(enum ResourceID resource,
                                   struct Player *user,
                                   struct Player *target);
typedef void (*GiveResourceHandler)(enum ResourceID resource,
                                    struct Player *target,
                                    int count);
typedef void (*TakeResourceHandler)(enum ResourceID resource,
                                    struct Player *target,
                                    int count);

/*
 * Central global list of games
 */
struct GameSlot {
    atomic_int inUse;
    struct Game game;
    pthread_mutex_t gameMutex;
};
static struct GameSlot gameList[MAX_GAMES] = {0};
atomic_int gameCount                       = 0;

/*
 * Returns a pointer to the corresponding
 * game from the global list, or NULL
 * if no name matched
 */
struct Game *getGameFromName(const char name[static MAX_CREDENTIAL_LEN])
{
    int cmp = -1;
    for (int i = 0; i < MAX_GAMES; i++) {
        cmp = strncmp(gameList[i].game.name, name, MAX_CREDENTIAL_LEN);
        if (cmp == 0) {
            return &gameList[i].game;
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
static inline int isGameMessageValidLength(Opcode opcode, ssize_t messageSize);
static void genPlayerStartPos(struct Coordinates *outCoords);
/*
 * Handlers for incoming messages from the websocket connection
 */
static void pingHandler(char *data, ssize_t dataSize, Host remotehost);
static void movePlayerHandler(char *data, ssize_t dataSize, Host remotehost);
static void playerConnectHandler(char *data, ssize_t dataSize, Host remotehost);

/*
 * Primary interpreter for incoming websocket messages
 * carrying valid gameplay opcodes.
 * Player disconnects are handled in packet_handlers.c,
 * and are not seen by the websocket layer.
 */
#define MESSAGE_HANDLER_COUNT 3
static GameMessageHandler gameMessageHandlers[MESSAGE_HANDLER_COUNT] = {
    pingHandler,
    movePlayerHandler,
    playerConnectHandler,
};
#define EMPTY_OPCODE 0 // Some opcodes just give the server no data
static int requestSizes[MESSAGE_HANDLER_COUNT] = {
    EMPTY_OPCODE,
    sizeof(struct MovePlayerReq),
    sizeof(struct PlayerConnectReq),
};
static int responseSizes[MESSAGE_HANDLER_COUNT] = {
    EMPTY_OPCODE,
    sizeof(struct MovePlayerRes),
    sizeof(struct PlayerConnectRes),
};

/*
 * There's an opcode, and then
 * serialised state data.
 * This state data should match a
 * corresponding data structure perfectly,
 * or be rejected.
 */
static inline int isGameMessageValidLength(Opcode opcode, ssize_t messageSize)
{
    return messageSize == requestSizes[opcode];
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
    Opcode opcode = 0;
    // memcpy because of pointer aliasing
    memcpy(&opcode, data, sizeof(opcode));
    if (opcode >= MESSAGE_HANDLER_COUNT) {
#ifdef DEBUG
        fprintf(stderr, "\nBad websocket Opcode.\n");
#endif
        return;
    }
#ifdef DEBUG
    printBufferInHex(data, dataSize);
#endif

    if (isGameMessageValidLength(opcode, dataSize - sizeof(Opcode))) {
        gameMessageHandlers[opcode](&data[sizeof(Opcode)],
                                    dataSize,
                                    remotehost);
    };
}

static inline int getFreeGame()
{
    int expected = 0;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (atomic_compare_exchange_strong(&gameList[i].inUse, &expected, 1)) {
            return i;
        }
    }
    return -1;
}

struct Game *createGame(struct GameConfig *config)
{
    int gameIndex = getFreeGame();
    if (gameIndex == -1) {
        return NULL;
    }
    struct Game *game = &gameList[gameIndex].game;

    game->threadlock = &gameList[gameIndex].gameMutex;

    strncpy(game->password, config->password, MAX_CREDENTIAL_LEN);
    strncpy(game->name, config->name, MAX_CREDENTIAL_LEN);
    game->maxPlayerCount = config->maxPlayerCount;
    game->minPlayerCount = config->minPlayerCount;

    atomic_store(&game->playerCount, 0);
    atomic_fetch_add(&gameCount, 1);
    return game;
}

void deleteGame(struct Game *game)
{
    pthread_mutex_t *lock = game->threadlock;
    pthread_mutex_lock(lock);
    // TODO:
    // Before we nuke the game,
    // we need to lock and tell all the clients
    // that the game is deleted and make
    // sure they shutdown
    atomic_store(&game->playerCount, 0);
    memset(game, 0, sizeof(*game));
    pthread_mutex_unlock(lock);
    atomic_fetch_sub(&gameCount, 1);
}

static void genPlayerStartPos(struct Coordinates *outCoords)
{
    outCoords->x = 0.0f;
    outCoords->y = 0.0f;
    outCoords->z = 0.0f;
}
/*
 * This function assumes that the player was redirected to
 * character creation and creates a character at the next free
 * index in the game.
 * Caller handles concurrency.
 */
struct Player *createPlayer(struct Game *game,
                            struct PlayerCredentials *credentials)
{
    struct Player *newPlayer = &game->players[game->playerCount];
    newPlayer->netID = createNetID(NET_TYPE_PLAYER, game, (void *)newPlayer);
    newPlayer->threadlock = getMutexFromNetID(game, newPlayer->netID);
    newPlayer->game       = game;
    memcpy(&newPlayer->credentials, credentials, sizeof(*credentials));
    genPlayerStartPos(&newPlayer->coords);
    atomic_fetch_add(&game->playerCount, 1);
    return newPlayer;
}

void deletePlayer(struct Player *restrict player)
{
    pthread_mutex_t *lock = player->threadlock;
    pthread_mutex_lock(lock);
    clearNetID(player->game, player->netID);
    atomic_fetch_sub(&player->game->playerCount, 1);
    player->game->playerCount--;
    memset(player, 0, sizeof(*player));
    pthread_mutex_unlock(lock);
}

void setPlayerCharSheet(struct Player *player,
                        const struct CharacterSheet *charsheet)
{
    pthread_mutex_lock(player->threadlock);
    memcpy(&player->charSheet, charsheet, sizeof(*charsheet));
    pthread_mutex_unlock(player->threadlock);
}

/*
 * Returns -1 on failure,
 * you should tell the client about malformed data.
 */
int initCharsheetFromForm(struct Player *player, const struct HTMLForm *form)
{
    pthread_mutex_lock(player->threadlock);
    struct CharacterSheet *sheet = &player->charSheet;
    if (form->fieldCount < FORM_CHARSHEET_FIELD_COUNT) {
        goto exit_error;
    }
    while (sheet->background < PLAYER_BACKGROUND_COUNT) {
        if (strncmp(playerBackgroundStrings[sheet->background],
                    form->fields[FORM_CHARSHEET_BACKGROUND],
                    HTMLFORM_FIELD_MAX_LEN) == 0) {
            break;
        }
        sheet->background++;
    }
    while (sheet->gender < GENDER_COUNT) {
        if (strncmp(playerGenderStrings[sheet->gender],
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

    if (validateNewCharsheet(sheet) != 0) {
        memset(sheet, 0, sizeof(*sheet));
        goto exit_error;
    }
    sheet->isValid = true;
    pthread_mutex_unlock(player->threadlock);
    return 0;
exit_error:
    pthread_mutex_unlock(player->threadlock);
    return -1;
}

/*
 * This just checks the "isValid" flag
 * in a thread safe manner,
 * see "validateNewCharsheet()"
 * for vibe-checking hackers
 */
bool isCharsheetValid(const struct Player *restrict player)
{
    bool result = 0;
    pthread_mutex_lock(player->threadlock);
    result = player->charSheet.isValid;
    pthread_mutex_unlock(player->threadlock);
    return result;
}

void setGamePassword(struct Game *restrict game,
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
struct Player *tryGetPlayerFromToken(SessionToken token,
                                     struct Game *restrict game)
{
    if (token == INVALID_SESSION_TOKEN) {
        return NULL;
    }
    for (int i = 0; i < game->playerCount; i++) {
        if (token == game->players[i].sessionToken) {
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
enum ResponseOpcodes {
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
static int initHandlerResponseBuffer(void *responseBuffer, Opcode code)
{
    int headerSize           = 0;
    ssize_t responseDataSize = responseSizes[code];

    headerSize =
        writeWebsocketHeader(responseBuffer, sizeof(code) + responseDataSize);
    memcpy(&responseBuffer[headerSize], &code, sizeof(code));
    return headerSize + sizeof(code);
}

static void pingHandler(char *data, ssize_t dataSize, Host remotehost)
{
#ifdef DEBUG
    printf("Ping incoming!");
#endif
    const Opcode responseOpcode                   = OPCODE_PING;
    char responseBuffer[MAX_RESPONSE_HEADER_SIZE] = {0};
    int packetSize = initHandlerResponseBuffer(responseBuffer, responseOpcode);
    sendDataTCP(responseBuffer, (ssize_t)packetSize, remotehost);
}

static void movePlayerHandler(char *data, ssize_t dataSize, Host remotehost)
{
    struct MovePlayerRes *responseData   = NULL;
    const struct MovePlayerReq *moveData = (struct MovePlayerReq *)data;
    const Opcode responseOpcode          = OPCODE_PLAYER_MOVE;
    int headerSize                       = 0;
    struct Player *hostPlayer            = getPlayerFromHost(remotehost);

    char responseBuffer[MAX_RESPONSE_HEADER_SIZE + sizeof(*responseData)] = {0};
    headerSize   = initHandlerResponseBuffer(responseBuffer, responseOpcode);
    responseData = (struct MovePlayerRes *)&responseBuffer[headerSize];

    validatePlayerMoveCoords(moveData, &responseData->coords);
    responseData->playerNetID = hostPlayer->netID;

    hostPlayer->coords.x = responseData->coords.xCoord;
    hostPlayer->coords.y = responseData->coords.yCoord;

    int packetSize = headerSize + sizeof(*responseData);
    multicastTCP(responseBuffer, packetSize, 0);
}

/*
 * Somebody has sent the connection opcode,
 * but the current player taking a turn is *nobody*.
 * Returns 0 on success, -1 if we still
 * can't start the game because there aren't enough
 * players.
 * Caller handles game threadlock.
 */
static int tryStartGame(struct Game *game)
{
    if (atomic_load(&game->playerCount) >= game->minPlayerCount) {
        // TODO: handle turn order more
        // gracefully than first come first serve.
        game->currentTurn = game->players[0].netID;
        return 0;
    }
    return -1;
}
/*
 * The client is attempting to fetch the player netIDs
 * so it can interpret messages about player state
 * changes
 */
static void playerConnectHandler(char *data, ssize_t dataSize, Host remotehost)
{
    // TODO:
    // When somebody connects, they could be connecting to
    // an ongoing game when someone, or themselves, are
    // in the middle of an encounter or other dialog.
    // This will need to be communicated.
    struct PlayerConnectRes *responseData = NULL;
    // Currently unused
    // ---------------------
    // const struct
    // PlayerConnectReq  *playerData     = (struct PlayerConnectReq*)data;
    const Opcode responseOpcode = OPCODE_PLAYER_CONNECT;
    struct Player *hostPlayer   = getPlayerFromHost(remotehost);
    struct Game *game           = hostPlayer->game;
    if (game == NULL) {
        return;
    }
    const ssize_t namelen = sizeof(game->players[0].credentials.name);

    char responseBuffer[MAX_RESPONSE_HEADER_SIZE +
                        sizeof(struct PlayerConnectRes)] = {0};

    int headerSize = initHandlerResponseBuffer(responseBuffer, responseOpcode);
    responseData   = (struct PlayerConnectRes *)&responseBuffer[headerSize];

    for (int i = 0; i < MAX_PLAYERS_IN_GAME; i++) {
        if (game->players[i].netID == NULL_NET_ID) {
            continue;
        }
        pthread_mutex_lock(game->players[i].threadlock);
        NetID id                 = game->players[i].netID;
        responseData->players[i] = id;
        memcpy(&responseData->playerNames[i],
               game->players[i].credentials.name,
               namelen);
        memcpy(&responseData->playerCoords[i],
               &game->players[i].coords,
               sizeof(game->players[i].coords));
        pthread_mutex_unlock(game->players[i].threadlock);
        if (hostPlayer->netID == id) {
            responseData->playerIndex = (int8_t)i;
        }
    }
    // It's *nobody's* turn?
    // This means the game hasn't started yet.
    if (game->currentTurn == NULL_NET_ID) {
        tryStartGame(game);
    }
    responseData->currentTurn = game->currentTurn;

    int packetSize = headerSize + sizeof(*responseData);
    multicastTCP(responseBuffer, packetSize, 0);
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
void baseUseResourceHandler(enum ResourceID resource,
                            struct Player *user,
                            struct Player *target)
{
    // Implement code for using specific resources here
}
void baseGiveResourceHandler(enum ResourceID resource,
                             struct Player *target,
                             int count)
{
    // Implement code for receiving resources here, such
    // as adding it to the player's inventory or adding
    // a passive effect
}
void baseTakeResourceHandler(enum ResourceID resource,
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
    baseUseResourceHandler};
// Handlers for when a resource is given to a player
static GiveResourceHandler giveResourceHandlers[RESOURCE_COUNT] = {
    baseGiveResourceHandler};
// Handlers for when a resource is taken from a player
static TakeResourceHandler takeResourceHandlers[RESOURCE_COUNT] = {
    baseTakeResourceHandler};
