#include "auth.h"
#include "html_server.h"
#include <stdlib.h>
#include <string.h>

static int isPlayerPasswordValid(const struct Player *restrict player,
                                 const char *password);
static struct Player *tryGetPlayerFromPlayername(struct Game *game,
                                                 const char *playername);
static void generateSessionToken(struct Player *player, struct Game *game);
static void buildSessionTokenHeader(char outHeader[static HEADER_LENGTH],
                                    SessionToken token);

static int isPlayerPasswordValid(const struct Player *restrict player,
                                 const char *password)
{
    int passwordCheck = 0;
    passwordCheck =
        strncmp(password, player->credentials.password, MAX_CREDENTIAL_LEN);
    return passwordCheck == 0;
}
/*
 * returns the index of the player in the game
 */
static struct Player *tryGetPlayerFromPlayername(struct Game *game,
                                                 const char *playername)
{
    int playerIndex = 0;
    int playerFound = -1;

    pthread_mutex_lock(game->threadlock);
    for (playerIndex = 0; playerIndex < game->playerCount; playerIndex++) {
        playerFound = strncmp(playername,
                              game->players[playerIndex].credentials.name,
                              MAX_CREDENTIAL_LEN);
        if (playerFound == 0) {
            pthread_mutex_unlock(game->threadlock);
            return &game->players[playerIndex];
        }
    }
    pthread_mutex_unlock(game->threadlock);
    return NULL;
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
int tryPlayerLogin(struct Game *restrict game,
                   struct PlayerCredentials *restrict credentials,
                   struct host *remotehost)
{
    struct Player *player                           = NULL;
    char sessionTokenHeader[CUSTOM_HEADERS_MAX_LEN] = {0};
    if (isEmptyString(credentials->name)) {
        return -1;
    }
    player = tryGetPlayerFromPlayername(game, credentials->name);
    pthread_mutex_lock(game->threadlock);
    if (player != NULL) {
        if (isPlayerPasswordValid(player, credentials->password)) {
            generateSessionToken(player, game);
            buildSessionTokenHeader(sessionTokenHeader, player->sessionToken);
            sendContent("./game.html",
                        HTTP_FLAG_TEXT_HTML,
                        remotehost,
                        sessionTokenHeader);
            pthread_mutex_unlock(game->threadlock);
            return 0;
        }
        else {
            // Player exists, but the password was wrong
            pthread_mutex_unlock(game->threadlock);
            return -1;
        }
    }
    // Player was not found in game redirect them to character creation
    // And create a player
    player = createPlayer(game, credentials);
    generateSessionToken(player, game);
    buildSessionTokenHeader(sessionTokenHeader, player->sessionToken);
    sendContent("./charsheet.html",
                HTTP_FLAG_TEXT_HTML,
                remotehost,
                sessionTokenHeader);

    pthread_mutex_unlock(game->threadlock);
    return 0;
}

/*
 * We pass in the game because the
 * session token needs to be unique
 * on a per game basis.
 * Caller handles concurrency.
 */
static void generateSessionToken(struct Player *restrict player,
                                 struct Game *restrict game)
{
    long long int nonce = getRandomInt();
    int i               = 0;
    // Make sure the token is unique, and not 0.
    while ((tryGetPlayerFromToken(nonce, game) != NULL) ||
           (nonce == INVALID_SESSION_TOKEN)) {
        nonce = getRandomInt();
        i++;
        if (i > TOKEN_GEN_LIMIT) {
            fprintf(stderr, "Too many token collisions, exiting...\n");
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
    char headerBase[HEADER_LENGTH]  = "Set-Cookie: sessionToken=";
    char tokenString[HEADER_LENGTH] = {0};

    sprintf(tokenString, "%lld\n", token);
    strncat(headerBase, tokenString, HEADER_LENGTH - strlen(headerBase));
    memcpy(outHeader, headerBase, HEADER_LENGTH);
}

/*
 * Parses an int64 token out of a HTTP
 * message and returns it, or 0 on failure.
 */
SessionToken getTokenFromHTTP(char *http, int httpLength)
{
    const char cookieName[HEADER_LENGTH] = "sessionToken=";
    int startIndex     = stringSearch(http, cookieName, httpLength);
    SessionToken token = 0;

    if (startIndex >= 0) {
        startIndex += strnlen(cookieName, HEADER_LENGTH);
        // TODO: This might overflow with specifically
        // malformed packets
        token = strtoll(&http[startIndex], NULL, 10);
    }
    return token;
}

/*
 * Returns 0 on success and -1 on failure
 */
int tryGameLogin(struct Game *restrict game, const char *password)
{
    int match = 0;
    pthread_mutex_lock(game->threadlock);
    match = strncmp(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock(game->threadlock);
    match = -1 * (match != 0);
    return match;
}
