#ifndef BB_PACKET_HANDLERS
#define BB_PACKET_HANDLERS

#include "bbnetlib.h"

#define MAX_PACKET_SIZE 1024

typedef enum {
    HANDLER_HTTP,
    HANDLER_WEBSOCK,
    HANDLER_COUNT
} HandlerEnum;

void masterHandler      (char *data, ssize_t packetSize, Host remotehost);

#endif
