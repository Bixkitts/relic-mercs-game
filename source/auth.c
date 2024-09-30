#include "auth.h"
#include "html_server.h"
#include <stdlib.h>
#include <string.h>

static int is_player_password_valid(const struct player *restrict player,
                                    const char *password);
static struct player *try_get_player_from_playername(struct game *game,
                                                     const char *playername);
static void generate_session_token(struct player *player, struct game *game);
static void build_session_token_header(char out_header[static HEADER_LENGTH],
                                       session_token_t token);

static int is_player_password_valid(const struct player *restrict player,
                                    const char *password)
{
    int password_check = 0;
    password_check =
        strncmp(password, player->credentials.password, MAX_CREDENTIAL_LEN);
    return password_check == 0;
}
/*
 * returns the index of the player in the game
 */
static struct player *try_get_player_from_playername(struct game *game,
                                                     const char *playername)
{
    int player_index = 0;
    int player_found = -1;

    pthread_mutex_lock(game->threadlock);
    for (player_index = 0; player_index < game->player_count; player_index++) {
        player_found = strncmp(playername,
                               game->players[player_index].credentials.name,
                               MAX_CREDENTIAL_LEN);
        if (player_found == 0) {
            pthread_mutex_unlock(game->threadlock);
            return &game->players[player_index];
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
int try_player_login(struct game *restrict game,
                     struct player_credentials *restrict credentials,
                     struct host *remotehost)
{
    struct player *player                             = NULL;
    char session_token_header[CUSTOM_HEADERS_MAX_LEN] = {0};
    if (is_empty_string(credentials->name)) {
        return -1;
    }
    player = try_get_player_from_playername(game, credentials->name);
    pthread_mutex_lock(game->threadlock);
    if (player != NULL) {
        if (is_player_password_valid(player, credentials->password)) {
            generate_session_token(player, game);
            build_session_token_header(session_token_header,
                                       player->session_token);
            send_content("./game.html",
                         HTTP_FLAG_TEXT_HTML,
                         remotehost,
                         session_token_header);
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
    player = create_player(game, credentials);
    generate_session_token(player, game);
    build_session_token_header(session_token_header, player->session_token);
    send_content("./charsheet.html",
                 HTTP_FLAG_TEXT_HTML,
                 remotehost,
                 session_token_header);

    pthread_mutex_unlock(game->threadlock);
    return 0;
}

/*
 * We pass in the game because the
 * session token needs to be unique
 * on a per game basis.
 * Caller handles concurrency.
 */
static void generate_session_token(struct player *restrict player,
                                   struct game *restrict game)
{
    long long int nonce = get_random_int();
    int i               = 0;
    // Make sure the token is unique, and not 0.
    while ((try_get_player_from_token(nonce, game) != NULL) ||
           (nonce == INVALID_SESSION_TOKEN)) {
        nonce = get_random_int();
        i++;
        if (i > TOKEN_GEN_LIMIT) {
            fprintf(stderr, "Too many token collisions, exiting...\n");
            exit(1);
        }
    }
    player->session_token = nonce;
}

/*
 * Builds the custom Cookie header that
 * sends the session token to the client.
 */
static void build_session_token_header(char out_header[static HEADER_LENGTH],
                                       session_token_t token)
{
    char header_base[HEADER_LENGTH]  = "Set-Cookie: sessionToken=";
    char token_string[HEADER_LENGTH] = {0};

    sprintf(token_string, "%lld\n", token);
    strncat(header_base, token_string, HEADER_LENGTH - strlen(header_base));
    memcpy(out_header, header_base, HEADER_LENGTH);
}

/*
 * Parses an int64 token out of a HTTP
 * message and returns it, or 0 on failure.
 */
session_token_t get_token_from_http(char *http, int http_length)
{
    const char cookie_name[HEADER_LENGTH] = "sessionToken=";
    int start_index       = string_search(http, cookie_name, http_length);
    session_token_t token = 0;

    if (start_index >= 0) {
        start_index += strnlen(cookie_name, HEADER_LENGTH);
        // TODO: This might overflow with specifically
        // malformed packets
        token = strtoll(&http[start_index], NULL, 10);
    }
    return token;
}

/*
 * Returns 0 on success and -1 on failure
 */
int try_game_login(struct game *restrict game, const char *password)
{
    int match = 0;
    pthread_mutex_lock(game->threadlock);
    match = strncmp(game->password, password, MAX_CREDENTIAL_LEN);
    pthread_mutex_unlock(game->threadlock);
    match = -1 * (match != 0);
    return match;
}
