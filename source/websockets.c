#include <stdlib.h>
#include <string.h>

#include "websockets.h"
#include "error_handling.h"

char *getWebSocketResponse(char *httpString)
{
    char *response     = NULL;
    char *tempResponse = 
        "HTTP/1.1 101 Switching Protocols\n"
        "Upgrade: websocket\n"
        "Connection: Upgrade\n"
        "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\n"
        "Sec-WebSocket-Protocol: chat\n\n";

    response = (char*)calloc(strlen(tempResponse), sizeof(char));
    if (response == NULL) {
        printError(BB_ERR_CALLOC);
        exit(1);
    }

    return response;
}
