#ifndef BB_PACKET_HANDLERS
#define BB_PACKET_HANDLERS

#include "bbnetlib.h"

#define MAX_PACKET_SIZE 1024

enum Handler{
    HANDLER_HTTP,
    HANDLER_WEBSOCK,
    HANDLER_COUNT
};

void masterHandler      (char *restrict data, ssize_t packetSize, Host remotehost);

void buildFileTable     (void);

#endif
