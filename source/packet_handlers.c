#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "auth.h"
#include "bbnetlib.h"
#include "error_handling.h"
#include "file_handling.h"
#include "game_logic.h"
#include "helpers.h"
#include "host_custom_attributes.h"
#include "html_server.h"
#include "packet_handlers.h"
#include "websockets.h"

enum credential_form_fields {
    FORM_CREDENTIAL_PLAYERNAME,
    FORM_CREDENTIAL_PLAYERPASSWORD,
    FORM_CREDENTIAL_GAMEPASSWORD,
    FORM_CREDENTIAL_FIELD_COUNT
};
typedef void (*packet_handler_t)(char *data,
                                 ssize_t packet_size,
                                 struct host *remotehost);

static inline enum handler initial_handler_check(struct host *remotehost);

static void disconnect_handler(char *data,
                               ssize_t packet_size,
                               struct host *remotehost);
static void http_handler(char *data,
                         ssize_t packet_size,
                         struct host *remotehost);
static void websock_handler(char *data,
                            ssize_t packet_size,
                            struct host *remotehost);

static void login_handler(char *restrict data,
                          ssize_t packet_size,
                          struct host *remotehost);
static void charsheet_handler(char *restrict data,
                              ssize_t packet_size,
                              struct host *remotehost);
static void post_handler(char *restrict data,
                         ssize_t packet_size,
                         struct host *remotehost);
static void http_get_handler(char *restrict get_request,
                        ssize_t packet_size,
                        struct host *remotehost);

/* disconnectHandler needs to be at index 0
 * because we use pointer math to handle
 * empty packets (TCP disconnections).
 * This is coupled with enum Handler
 * in packet_handlers.h
 */
static packet_handler_t handlers[HANDLER_COUNT] = {disconnect_handler,
                                                   http_handler,
                                                   websock_handler};

/*
 * Called from masterHandler,
 * checks all incoming packets
 * and figures out which handler to use
 * i.e. what application-level protocol.
 * This depends on the "handlers[]" array
 * in this file,
 * and enum Handler in packet_handlers.h.
 */
static inline enum handler initial_handler_check(struct host *remotehost)
{
    struct host_custom_attr *custom_attr = NULL;
    if (!get_host_custom_attr(remotehost)) {
        custom_attr = calloc(1, sizeof(*custom_attr));
        if (!custom_attr) {
            print_error(BB_ERR_CALLOC);
            exit(1);
        }
        custom_attr->handler = HANDLER_DEFAULT;
        set_host_custom_attr(remotehost, (void *)custom_attr);
    }
    else {
        custom_attr =
            (struct host_custom_attr *)get_host_custom_attr(remotehost);
    }
    return custom_attr->handler;
}

/*
 * This function captures and handles
 * every single incoming TCP packet
 * in the entire server.
 */
void master_handler(char *restrict data,
                    ssize_t packet_size,
                    struct host *remotehost)
{
    const enum handler handler = initial_handler_check(remotehost);
#ifdef DEBUG
    if (packet_size > 0) {
        printf("\nReceived data:");
        for (int i = 0; i < packet_size; i++) {
            printf("%c", data[i]);
        }
        printf("\n");
    }
    else if (packet_size == 0) {
        printf("\nClient disconnected\n");
    }
#endif

    // Pointer math handles client disconnects
    // and calls disconnectHandler()
    handlers[handler * (packet_size > 0)](data, packet_size, remotehost);

    return;
}

/*
 * Handler for HTTP GET requests
 */
static void http_get_handler(char *restrict get_request,
                             ssize_t packet_size,
                             struct host *remotehost)
{
    assert(get_request && remotehost);
    const int starting_index = strnlen("GET /", 5);
    if (packet_size <= starting_index) return;

    char requested_resource[MAX_FILENAME_LEN] = {0};
    const struct char_slice filename = slice_string_to(get_request,
                                                       starting_index,
                                                       packet_size,
                                                       ' ');
    char *file_table_entry = NULL;

    struct host_custom_attr *custom_attr =
        (struct host_custom_attr *)get_host_custom_attr(remotehost);

    if (filename.len < 0 || filename.len > MAX_FILENAME_LEN) {
        return;
    }

    memcpy(requested_resource, filename.start, filename.len);

    struct game     *game   = get_game_from_name(test_game_name);
    session_token_t  token  = get_token_from_http(get_request, packet_size);
    struct player   *player = try_get_player_from_token(token, game);

    /* Direct the remotehost to the login, character creation
     * or game depending on their session token.
     */
    if (string_search(get_request, "GET / ", 10) >= 0) {
        if (!player) {
            send_content("./login.html", HTTP_FLAG_TEXT_HTML, remotehost, NULL);
        }
        else if (!is_charsheet_valid(player)) {
            send_content("./charsheet.html",
                         HTTP_FLAG_TEXT_HTML,
                         remotehost,
                         NULL);
        }
        else if (string_search(get_request, "Sec-WebSocket-Key", packet_size) >= 0) {
            send_web_socket_response(get_request, packet_size, remotehost);
            struct host_custom_attr *host_attr =
                (struct host_custom_attr *)get_host_custom_attr(remotehost);
            host_attr->player    = player;
            custom_attr->handler = HANDLER_WEBSOCK;
            cache_host(remotehost, get_current_host_cache());
            return;
        }
        else {
            send_content("./game.html", HTTP_FLAG_TEXT_HTML, remotehost, NULL);
        }
        return;
    }
    // Unauthenticated users are allowed the stylesheet, and login script
    else if (string_search(get_request, "GET /styles.css", 16) >= 0) {
        send_content("./styles.css", HTTP_FLAG_TEXT_CSS, remotehost, NULL);
        return;
    }
    else if (string_search(get_request, "GET /login.js", 14) >= 0) {
        send_content("./login.js", HTTP_FLAG_TEXT_JAVASCRIPT, remotehost, NULL);
        return;
    }
    /*
     * For any other url than a blank one
     * (or the styles.css and login.js),
     * the remotehost needs to be authenticated
     * otherwise we're not sending anything at all.
     */
    if (!player) {
        send_forbidden_packet(remotehost);
        return;
    }
    else if (is_file_allowed(requested_resource, &file_table_entry)) {
        send_content(file_table_entry,
                     get_content_type_enum_from_filename(file_table_entry),
                     remotehost,
                     NULL);
        return;
    }
    else if (string_search(get_request, "GET /index.js", 12) >= 0) {
        send_content("./index.js", HTTP_FLAG_TEXT_JAVASCRIPT, remotehost, NULL);
        return;
    }
    send_forbidden_packet(remotehost);
}

static void login_handler(char *restrict data,
                          ssize_t packet_size,
                          struct host *remotehost)
{
    // Read the Submitted Player Name, Player Password and Game Password
    // and link the remotehost to a specific player object based on that.
    struct player_credentials credentials           = {0};
    int credential_index                            = 0;
    const char first_form_field[MAX_CREDENTIAL_LEN] = "playerName=";
    struct html_form form                           = {0};

    // Where the credentials start, as expected by parse_html_form().
    credential_index =
        string_search(data, first_form_field, packet_size);

    parse_html_form(&data[credential_index],
                    &form,
                    packet_size - credential_index);
    if (form.field_count < FORM_CREDENTIAL_FIELD_COUNT) {
        send_forbidden_packet(remotehost); // placeholder
        return;
    }
    if (try_game_login(get_game_from_name(test_game_name),
                       form.fields[FORM_CREDENTIAL_GAMEPASSWORD]) != 0) {
        send_bad_request_packet(remotehost);
        return;
    };
    strncpy(credentials.name,
            form.fields[FORM_CREDENTIAL_PLAYERNAME],
            MAX_CREDENTIAL_LEN);
    strncpy(credentials.password,
            form.fields[FORM_CREDENTIAL_PLAYERPASSWORD],
            MAX_CREDENTIAL_LEN);
    if (try_player_login(get_game_from_name(test_game_name),
                         &credentials,
                         remotehost) < 0) {
        send_bad_request_packet(remotehost);
        return;
    }
    return;
}

static void charsheet_handler(char *restrict data,
                              ssize_t packet_size,
                              struct host *remotehost)
{
    session_token_t token = get_token_from_http(data, packet_size);
    struct player *player =
        try_get_player_from_token(token, get_game_from_name(test_game_name));
    struct html_form form = {0};

    const char first_form_field[HTMLFORM_FIELD_MAX_LEN] = "playerBackground=";

    if (!player) {
        // token was invalid, handle that
        send_forbidden_packet(remotehost);
        return;
    }
    int html_form_index = string_search(data, first_form_field, packet_size);
    if (html_form_index < 0) {
        // TODO: Malformed form data, let the client know
        send_forbidden_packet(remotehost); // placeholder
        return;
    }
    parse_html_form(&data[html_form_index],
                    &form,
                    packet_size - html_form_index);
    if (init_charsheet_from_form(player, &form) != 0) {
        // The client needs to know about malformed data
        send_forbidden_packet(remotehost); // placeholder
        return;
    }
    send_content("./game.html", HTTP_FLAG_TEXT_HTML, remotehost, NULL);
}

static void post_handler(char *restrict data,
                         ssize_t packet_size,
                         struct host *remotehost)
{
    if (string_search(data, "login", 12) >= 0) {
        login_handler(data, packet_size, remotehost);
    }
    else if (string_search(data, "charsheet", 16) >= 0) {
        charsheet_handler(data, packet_size, remotehost);
    }
}

static void http_handler(char *restrict data,
                         ssize_t packet_size,
                         struct host *remotehost)
{
    if (packet_size < 10) {
        return;
    }
    if (string_search(data, "GET /", 8) >= 0) {
        http_get_handler(data, packet_size, remotehost);
    }
    else if (string_search(data, "POST /", 8) >= 0) {
        post_handler(data, packet_size, remotehost);
    }
    else {
        send_forbidden_packet(remotehost);
    }
}

/*
 * Global caching state.
 * Affected by disconnections.
 */
pthread_mutex_t caching_mutex = PTHREAD_MUTEX_INITIALIZER;

// Netlib gives us numbered caches for
// storing hosts that connect
int8_t current_host_cache = 0;
int8_t last_host_cache    = 0;

static void disconnect_handler(char *data,
                               ssize_t packet_size,
                               struct host *remotehost)
{
    struct host_custom_attr *attr = get_host_custom_attr(remotehost);
    if (attr->handler == HANDLER_WEBSOCK) {
        uncache_host(remotehost, get_current_host_cache());
        // TODO: When someone disconnects,
        // the game will need to pause and alert everyone
        // of the disconnect and ask whether to
        // continue or wait.
    }
}
/*
 * When a user disconnects,
 * all OTHER users who are still
 * connected get written to another
 * multicast cache and the former
 * one is cleared.
 * This function returns the up-to-date
 * multicast cache.
 */
int get_current_host_cache(void)
{
    pthread_mutex_lock(&caching_mutex);
    int ret = (int)current_host_cache;
    pthread_mutex_unlock(&caching_mutex);
    return ret;
}

static void websock_handler(char *restrict data,
                            ssize_t packet_size,
                            struct host *remotehost)
{
    const int min_websocket_packet_len = 8;
    if (packet_size < min_websocket_packet_len) {
        fprintf(stderr, "\nToo short websocket packet received.\n");
        return;
    }
    // After this part we can freely dereference the
    // first 8 bytes for opcodes and such.
    char decoded_data[MAX_PACKET_SIZE] = {0};
    int decoded_data_length            = 0;

    decoded_data_length =
        decode_websocket_message(decoded_data, data, packet_size);
    handle_game_message(decoded_data, decoded_data_length, remotehost);
}
