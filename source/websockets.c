#include <stdlib.h>
#include <string.h>

#include "websockets.h"
#include "error_handling.h"
#include "helpers.h"

#define WEBSOCK_HEADERS_LEN 512

static void extractAcceptCode(char *outString, char *httpString, ssize_t packetSize)
{
    const char *pattern = "Sec-WebSocket-Accept: ";
    int index  = stringSearch(httpString, pattern, packetSize);
    index += strlen(pattern); // size of above pattern
}
void sendWebSocketResponse(char *httpString, ssize_t packetSize, Host remotehost)
{
    char  response[WEBSOCK_HEADERS_LEN]  = { 0 };
    // We append the calculated hash to this
    // Then build the response
    char *tempResponse = 
        "HTTP/1.1 101 Switching Protocols\n"
        "Upgrade: websocket\n"
        "Connection: Upgrade\n"
        "Sec-WebSocket-Accept: ";
    char *finResponse =
        "Sec-WebSocket-Protocol: chat\n\n";

    strcpy(response, tempResponse);


}
