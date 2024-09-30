#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "bbnetlib.h"
#include "game_logic.h"
#include "helpers.h"
#include "html_server.h"
#include "packet_handlers.h"

Host localhost = NULL;

int main(void)
{
    printf("\nWelcome to the test server!");
    printf("\n-----------------------------------\n");
#ifdef DEBUG
    printf("RUNNING SLOW DEBUG VERSION\n");
    checkDataSizes();
#endif
    enableTLS();
    localhost = createHost("0.0.0.0", 7676);

    createAllowedFileTable();

    // TODO: Make a web interface for creating and
    // joining multiple games.
    struct GameConfig gameConfig = {.name           = "test game",
                                    .password       = "hello",
                                    .maxPlayerCount = 4,
                                    .minPlayerCount = 2};

    createGame(&gameConfig);

    /* All incoming TCP packets
     * are given to "masterHandler()"
     * in "packet_handlers.c"
     */
    listenForTCP(localhost, masterHandler);
    return 0;
}
