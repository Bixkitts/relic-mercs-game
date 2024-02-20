#ifndef BB_WEBSOCKETS
#define BB_WEBSOCKETS
#include "bbnetlib.h"

void sendWebSocketResponse(char *httpString, ssize_t packetSize, Host remotehost);

#endif
