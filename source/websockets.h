#ifndef BB_WEBSOCKETS
#define BB_WEBSOCKETS
#include "bbnetlib.h"

#define WEBSOCKET_HEADER_SIZE_MAX 8 

void  sendWebSocketResponse   (char *httpString, ssize_t packetSize, Host remotehost);

int   decodeWebsocketMessage  (char *outData, char *inData, ssize_t dataSize);
// returns size of entire websocket packet including header
int   writeWebsocketHeader  (char inOutData[static WEBSOCKET_HEADER_SIZE_MAX], ssize_t dataSize);
#endif
