#ifndef BB_PACKET_HANDLERS
#define BB_PACKET_HANDLERS

#include "bbnetlib.h"

typedef enum {
    HANDLER_HTML,
    HANDLER_WEBSOCK,
    HANDLER_ESTABLISHED,
    HANDLER_COUNT
} HandlerEnum;

void masterHandler      (char *data, ssize_t packetSize, Host remotehost);

#endif
