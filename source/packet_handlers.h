#ifndef BB_PACKET_HANDLERS
#define BB_PACKET_HANDLERS

#include "bbnetlib.h"

#define MAX_PACKET_SIZE 1024

enum Handler {
    HANDLER_DISCONNECT,
    HANDLER_HTTP,
    HANDLER_WEBSOCK,
    HANDLER_COUNT
};

#define HANDLER_DEFAULT HANDLER_HTTP

void masterHandler(char *restrict data, ssize_t packetSize, struct host *remotehost);

void buildFileTable(void);

int getCurrentHostCache(void);

int getCurrentHostCache(void);
#endif
