#ifndef BB_HOST_CUSTOM_ATTRIBUTES
#define BB_HOST_CUSTOM_ATTRIBUTES

#include "packet_handlers.h"
#include "game_logic.h"

// Store all custom data you need to attach to a host 
// in this struct
typedef struct HostCustomAttributes {
    HandlerEnum handler; // Which handler should be called when receiving a 
                         // packet from this host
    Player     *player;  // Which player this host controls
}HostCustomAttributes;

#endif
