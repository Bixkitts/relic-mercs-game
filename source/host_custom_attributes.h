#ifndef BB_HOST_CUSTOM_ATTRIBUTES
#define BB_HOST_CUSTOM_ATTRIBUTES

#include "game_logic.h"
#include "packet_handlers.h"

/*
 * This struct stores all custom data we want
 * to save as part of a remote host connection.
 * Most notably, the application level protocol
 * in use by the TCP connection in "handler".
 */
struct host_custom_attr {
    enum handler handler;  // Which handler should be called when receiving a
                           // packet from this host
    struct player *player; // Which player this host controls
};

static inline struct player *get_player_from_host(struct host *remotehost)
{
    struct host_custom_attr *attr = get_host_custom_attr(remotehost);
    return attr->player;
}

#endif
