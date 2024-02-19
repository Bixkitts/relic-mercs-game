#include <stdio.h>
#include <unistd.h>

#include "bbnetlib.h"
#include "helpers.h"
Host localhost = NULL;

void testPacketHandler(char *data, ssize_t packetSize, Host remotehost)
{
    printf("\nReceived data:");
    for (int i = 0; i < packetSize; i++) {
        printf("%c", data[i]);
    }
    printf("\n");
    return;
}

int main(void)
{
    printf("\nWelcome to the test server!");
    printf("\n-----------------------------------\n");
    localhost = createHost("0.0.0.0", 80);

    listenForTCP(localhost, testPacketHandler);
    return 0;
}
