#ifndef BB_PACKET_HANDLERS
#define BB_PACKET_HANDLERS

#include "bbnetlib.h"

#define MAX_PACKET_SIZE  1024
/* Netlib gives us numbered caches
 * to store hosts for multicasting.
 * This constant defines the one we
 * use for all caching.
 */
#define HOST_CACHE_INDEX 0

enum Handler{
    HANDLER_DISCONNECT,
    HANDLER_HTTP,
    HANDLER_WEBSOCK,
    HANDLER_COUNT
};

void masterHandler       (char *restrict data, ssize_t packetSize, Host remotehost);

void buildFileTable      (void);

int  getCurrentHostCache (void);

#endif
