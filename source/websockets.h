#ifndef BB_WEBSOCKETS
#define BB_WEBSOCKETS
#include "bbnetlib.h"

void  sendWebSocketResponse   (char *httpString, ssize_t packetSize, Host remotehost);

void  decodeWebsocketMessage  (char *outData, char *inData, ssize_t dataSize);
// returns size of entire websocket packet including header
int   encodeWebsocketMessage  (char *outData, char *inData, ssize_t dataSize);
#endif
