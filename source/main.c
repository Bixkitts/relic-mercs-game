#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "bbnetlib.h"
#include "game_logic.h"
#include "helpers.h"
#include "html_server.h"
#include "packet_handlers.h"

struct host *localhost = NULL;

int main(void)
{
    printf("\nWelcome to the test server!");
    printf("\n-----------------------------------\n");
#ifdef DEBUG
    printf("RUNNING SLOW DEBUG VERSION\n");
    check_data_sizes();
#endif
    enable_tls();
    localhost = create_host("0.0.0.0", 7676);

    create_allowed_file_table();

    // TODO: Make a web interface for creating and
    // joining multiple games.
    struct game_config game_config = {.name             = "test game",
                                      .password         = "hello",
                                      .max_player_count = 4,
                                      .min_player_count = 2};

    create_game(&game_config);

    /* All incoming TCP packets
     * are given to "masterHandler()"
     * in "packet_handlers.c"
     */
    listen_for_tcp(localhost, master_handler);
    return 0;
}
