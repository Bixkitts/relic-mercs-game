#ifndef BB_HOST_CUSTOM_ATTRIBUTES
#define BB_HOST_CUSTOM_ATTRIBUTES

#include "packet_handlers.h"
#include "game_logic.h"

// Store all custom data you need to attach to a host 
// in this struct
struct HostCustomAttributes {
    enum Handler  handler;  // Which handler should be called when receiving a 
                           // packet from this host
    struct Player *player;  // Which player this host controls
};

static inline struct Player *getPlayerFromHost(Host remotehost)
{
    struct HostCustomAttributes *attr = getHostCustomAttr(remotehost);
    return attr->player;
}

#endif
