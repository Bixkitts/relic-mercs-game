#ifndef BB_PACKET_HANDLERS
#define BB_PACKET_HANDLERS

#include "bbnetlib.h"

#define MAX_PACKET_SIZE 1024

enum handler {
    HANDLER_DISCONNECT,
    HANDLER_HTTP,
    HANDLER_WEBSOCK,
    HANDLER_COUNT
};

#define HANDLER_DEFAULT HANDLER_HTTP

void master_handler(char *restrict data,
                    ssize_t packet_size,
                    struct host *remotehost);

void build_file_table(void);

int get_current_host_cache(void);

#endif
