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
#endif
    enableTLS    ();
    localhost = createHost("0.0.0.0", 443);

    createAllowedFileTable();

    // TODO: Make a web interface for creating and
    // joining multiple games.
    GameConfig gameConfig = {
        .password       = "hello",
        .maxPlayerCount = 4
    };
    Game *mainGame = getTestGame();
    createGame(&mainGame, &gameConfig);

    listenForTCP (localhost, masterHandler);
    return 0;
}
