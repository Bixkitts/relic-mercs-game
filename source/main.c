#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bbnetlib.h"
#include "helpers.h"
#include "packet_handlers.h"
#include "html_server.h"
#include "game_logic.h"

Host localhost = NULL;


int main(void)
{
    printf("\nWelcome to the test server!");
    printf("\n-----------------------------------\n");
#ifdef DEBUG
    printf("RUNNING SLOW DEBUG VERSION\n");
    checkDataSizes();
#endif
    enableTLS    ();
    localhost = createHost("0.0.0.0", 7676);

    createAllowedFileTable();

    // TODO: Make a web interface for creating and
    // joining multiple games.
    struct GameConfig gameConfig = {
        .name           = "test game",
        .password       = "hello",
        .maxPlayerCount = 4,
        .minPlayerCount = 2
    };

    createGame(&gameConfig);

    listenForTCP (localhost, masterHandler);
    return 0;
}
