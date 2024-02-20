#include <stdio.h>
#include <unistd.h>

#include "bbnetlib.h"
#include "helpers.h"
#include "packet_handlers.h"

Host localhost = NULL;


int main(void)
{
    printf("\nWelcome to the test server!");
    printf("\n-----------------------------------\n");
    localhost = createHost("0.0.0.0", 80);

    listenForTCP(localhost, masterHandler);
    return 0;
}
