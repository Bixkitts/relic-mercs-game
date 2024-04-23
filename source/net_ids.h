#ifndef BB_NET_IDS
#define BB_NET_IDS

/*
 * Types of objects or states
 * that will need to be synced,
 * partially or fully serialised
 * over the network
 */
enum NetObjType {
    NET_TYPE_NULL,
    NET_TYPE_PLAYER,
    NET_TYPE_GAME,
    NET_TYPE_COUNT
};
/*
 * A NetID is a unique numberic identifier for
 * a specific object that can be sent
 * over the network as an integer to refer
 * to the specific object.
 */
typedef long long NetID;

void *resolveNetIDToObj (const NetID netID,
                         enum NetObjType type);
NetID createNetID       (enum NetObjType);
void  clearNetID        (const NetID netID);


#endif
